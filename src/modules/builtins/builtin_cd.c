#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <pwd.h>

#include "builtin.h"
#include "logging.h"
#include "retry.h"

private char *moss_oldpwd = NULL;

private int doChdir(void *ctx)
{
    ChdirContext *chdirCtx = (ChdirContext *)ctx;
    return chdir(chdirCtx->path);
}

// caller must free memory tho 🥀
private char *expand_path(const char *path)
{
    if (strcmp(path, "-") == 0)
    {
        if (!moss_oldpwd)
        {
            SAFE_ERROR(ERR_CATEGORY_PATH, "No prev directory");
            return NULL;
        }

        return strdup(moss_oldpwd);
    }

    if (path[0] == '~')
    {
        switch (path[1])
        {
        case '\0':
        {
            char *home = getenv("HOME");

            if (!home)
            {
                SAFE_ERROR(ERR_CATEGORY_PATH, "Home directory not set");
                return NULL;
            }

            return strdup(home);
        }
        case '/':
        {
            char *home = getenv("HOME");

            if (!home)
            {
                SAFE_ERROR(ERR_CATEGORY_PATH, "Home directory not set");
                return NULL;
            }

            size_t totalLength = strlen(home) + strlen(path + 1) + 1;
            char *expanded = (char *)calloc(1, totalLength);

            if (!expanded)
            {
                SAFE_ERROR(ERR_CATEGORY_MEMORY, "Alloc failed");
                return NULL;
            }

            strcpy(expanded, home);
            strcat(expanded, path + 1);

            return expanded;
        }

        default:
        {
            const char *userName = path + 1;
            struct passwd *pw = getpwnam(userName);

            if (!pw)
            {
                SAFE_ERROR(ERR_CATEGORY_PATH, "Unknown user: %s", userName);
                return NULL;
            }

            return strdup(pw->pw_dir);
        }
        }
    }

    return strdup(path);
}

int moss_cd(char **args)
{
    char *targetPath = NULL;
    char *expandedPath = NULL;
    char currentDir[PATH_MAX] = {0};

    if (getcwd(currentDir, sizeof(currentDir)) == NULL)
    {
        SAFE_ERROR(ERR_CATEGORY_PATH, "Failed to get current directory");
        return 1;
    }

    if (!args[1])
    {
        const char *home = getenv("HOME");

        if (!home)
        {
            SAFE_ERROR(ERR_CATEGORY_PATH, "Home directory is not set");
            return 1;
        }

        targetPath = strdup(home);
    }
    else
    {
        expandedPath = expand_path(args[1]);

        if (!expandedPath)
            return 1;

        targetPath = expandedPath;
    }

    if (args[1] && strcmp(args[1], "-") == 0)
        printf("%s\n", targetPath);

    char resolved[PATH_MAX] = {0};

    if (realpath(targetPath, resolved) == NULL)
    {
        SAFE_ERROR(ERR_CATEGORY_PATH, "invalid path: %s", targetPath);
        free(targetPath);
        return 1;
    }

    ChdirContext ctx = {.path = resolved};
    RetryConfig retryConfig = {
        .maxRetries = 3,
        .baseDelayms = 50,
        .maxDelayms = 500,
        .useExponentialBackoff = true,
    };

    RetryContext retryCtx;
    mossRetryInit(&retryCtx, &retryConfig);
    RetryResult result = mossRetryExecute(&retryCtx, doChdir, &ctx, mossRetryShouldRetry);

    if (result != RETRY_OK)
    {
        SAFE_ERROR(ERR_CATEGORY_PATH, "failed to change directory to %s", resolved);
        free(targetPath);
        return 1;
    }

    if (moss_oldpwd)
        free(moss_oldpwd);

    moss_oldpwd = strdup(currentDir);
    setenv("OLDPWD", currentDir, 1);
    setenv("PWD", resolved, 1);
    free(targetPath);

    if (args[2])
        SAFE_ERROR(ERR_CATEGORY_INPUT, "cd: too many arguments");

    return 0;
}
