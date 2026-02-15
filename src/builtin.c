#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>

#include "include/builtin.h"
#include "include/logging.h"
#include "include/retry.h"

extern char **environ;

const struct builtin builtins[] = {
    {"cd", moss_cd},
    {"help", moss_help},
    {"exit", moss_exit},
    {"pwd", moss_pwd},
    {"clear", moss_clear},
    {"echo", moss_echo},
    {"whoami", moss_whoami},
    {"env", moss_env},
    {"export", moss_export},
    {"unset", moss_unset},
    {"type", moss_type},
};

const int NUM_BUILTINS = sizeof(builtins) / sizeof(struct builtin);

private int doChdir(void *ctx)
{
    ChdirContext *chdirCtx = (ChdirContext *)ctx;
    return chdir(chdirCtx->path);
}

int moss_cd(char **args)
{
    if (!args[1])
    {
        char *home = getenv("HOME");

        if (!home)
        {
            SAFE_ERROR(ERR_CATEGORY_PATH, "home directory not set");
            return 1;
        }

        ChdirContext ctx = {.path = home};
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
            SAFE_ERROR(ERR_CATEGORY_PATH, "failed to change to home directory");

        return 1;
    }

    char resolved[PATH_MAX];

    if (realpath(args[1], resolved) == NULL)
    {
        SAFE_ERROR(ERR_CATEGORY_PATH, "invalid path: %s", args[1]);
        return 1;
    }

    ChdirContext ctx = {.path = resolved};
    RetryConfig retryConfig = {
        .maxRetries = 3,
        .baseDelayms = 50,
        .maxDelayms = 500,
        .useExponentialBackoff = true};
    RetryContext retryCtx;
    mossRetryInit(&retryCtx, &retryConfig);

    RetryResult result = mossRetryExecute(&retryCtx, doChdir, &ctx, mossRetryShouldRetry);

    if (result != RETRY_OK)
        SAFE_ERROR(ERR_CATEGORY_PATH, "failed to change directory to %s", resolved);

    if (args[2])
        SAFE_ERROR(ERR_CATEGORY_INPUT, "cd: too many arguments");

    return 1;
}

int moss_help(char **args)
{
    (void)args;
    printf("Type programs names and arguments, and hit enter.\n");
    printf("The following are built in:\n");

    for (size_t i = 0; i < NUM_BUILTINS; i++)
        printf("  %s\n", builtins[i].name);

    printf("Use the 'man' command for more information on other programs.\n");

    return 1;
}

int moss_exit(char **args)
{
    (void)args;
    return 0;
}

int moss_pwd(char **args)
{
    (void)args;

#ifdef PATH_MAX
    size_t bufsize = PATH_MAX;
#else
    size_t bufsize = SIZE;
#endif

    char *cwd = (char *)malloc(bufsize);

    if (!cwd)
    {
        SAFE_ERROR(ERR_CATEGORY_MEMORY, "memory allocation failed");
        return 1;
    }

    if (getcwd(cwd, bufsize) != NULL)
        printf("%s\n", cwd);
    else
    {
        if (errno == ERANGE)
            SAFE_ERROR(ERR_CATEGORY_PATH, "path too long");
        else
            SAFE_ERROR(ERR_CATEGORY_PATH, "failed to get current directory");
    }

    free(cwd);

    return 1;
}

int moss_clear(char **args)
{
    (void)args;
    printf("\033[H\033[J");
    return 1;
}

int moss_echo(char **args)
{
    for (size_t i = 1; args[i] != NULL; i++)
    {
        printf("%s", args[i]);

        if (args[i + 1] != NULL)
            printf(" ");
    }

    printf("\n");
    return 1;
}

int moss_whoami(char **args)
{
    (void)args;
    const char *userName = getenv("USER");

    if (!userName)
        userName = getenv("USERNAME");

    if (userName)
        printf("%s\n", userName);
    else
        SAFE_ERROR(ERR_CATEGORY_SYSTEM, "Could not find the user");

    return 1;
}

int moss_env(char **args)
{
    (void)args;

    for (char **env = environ; *env != NULL; env++)
        printf("%s\n", *env);

    return 1;
}

int moss_export(char **args)
{
    if (!args[1])
    {
        moss_env(args);
        return 1;
    }

    // export PATH=/local/bin:$PATH
    char *eq = strchr(args[1], '=');

    if (!eq)
    {
        SAFE_ERROR(ERR_CATEGORY_INPUT, "Usage: export NAME-VALUE");
        return 1;
    }

    *eq = '\0';
    const char *name = args[1];
    const char *value = eq + 1;

    // whats setenv()
    if (setenv(name, value, 1) != 0)
        SAFE_ERROR(ERR_CATEGORY_SYSTEM, "Failed to set env variable");

    return 1;
}

int moss_unset(char **args)
{
    if (!args[1])
    {
        SAFE_ERROR(ERR_CATEGORY_INPUT, "Usage: unset NAME");
        return 1;
    }

    if (unsetenv(args[1]) != 0)
        SAFE_ERROR(ERR_CATEGORY_SYSTEM, "Failed to unset variable");

    return 1;
}

int moss_type(char **args)
{
    if (!args[1])
    {
        SAFE_ERROR(ERR_CATEGORY_INPUT, "Usage: type COMMAND");
        return 1;
    }

    for (size_t i = 0; i < NUM_BUILTINS; i++)
    {
        if (strcmp(args[1], builtins[i].name) == 0)
        {
            printf("%s is a shell builtin\n", args[1]);
            return 1;
        }
    }

    printf("%s is an external command\n", args[1]);
    return 1;
}