#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>
#include <pwd.h>

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
private char *moss_oldpwd = NULL;
private bool isEnvVarChar(char);

private bool isEnvVarChar(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

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

            // total length: home + rest of the path + null terminator
            size_t totalLength = strlen(home) + strlen(path + 1) + 1;
            char *expanded = (char *)malloc(totalLength);

            if (!expanded)
            {
                SAFE_ERROR(ERR_CATEGORY_MEMORY, "Alloc failed");
                return NULL;
            }

            // full path: home + rest of the path
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

char *expandEnvVar(const char *token)
{
    if (!token)
    {
        SAFE_ERROR(ERR_CATEGORY_INPUT, "MOSS: Input is empty");
        return NULL;
    }

    if (strchr(token, '$') == NULL)
        return strdup(token);

    size_t resultSize = 64;
    char *result = (char *)malloc(resultSize);

    if (!result)
    {
        SAFE_ERROR(ERR_CATEGORY_MEMORY, "MOSS: Memory allocation failed");
        return NULL;
    }

    result[0] = '\0';
    const size_t len = strlen(token);
    size_t resultPosition = 0;

    for (size_t i = 0; i < len; i++)
    {
        if (token[i] != '$')
        {
            result[resultPosition++] = token[i];
            result[resultPosition] = '\0';
        }
        else
        {
            if (i + 1 >= len)
            {
                result[resultPosition++] = '$';
                result[resultPosition] = '\0';
            }
            else if (token[i + 1] == '$')
            {
                char pidStr[32];
                snprintf(pidStr, sizeof(pidStr), "%d", getpid());
                size_t pidLen = strlen(pidStr);

                while (resultPosition + pidLen + 1 > resultSize)
                {
                    resultSize *= 2;
                    char *newResult = realloc(result, resultSize);

                    if (!newResult)
                    {
                        free(result);
                        SAFE_ERROR(ERR_CATEGORY_MEMORY, "MOSS: Memory allocation failed for");
                        return NULL;
                    }

                    result = newResult;
                }

                strcat(result, pidStr);
                resultPosition += pidLen;
                i++;
            }
            else if (token[i + 1] == '{')
            {
                size_t start = i + 2;
                size_t end = start;

                while (end < len && token[end] != '}')
                    end++;

                if (end == len)
                {
                    result[resultPosition++] = '$';
                    result[resultPosition++] = '{';
                    result[resultPosition] = '\0';
                    i = len - 1;
                }
                else
                {
                    const size_t varLen = end - start;
                    char *varName = (char *)malloc(varLen + 1);

                    if (!varName)
                    {
                        free(result);
                        SAFE_ERROR(ERR_CATEGORY_MEMORY, "MOSS: Memory allocation failed");
                        return NULL;
                    }

                    strncpy(varName, token + start, varLen);
                    varName[varLen] = '\0';
                    char *value = getenv(varName);

                    if (value)
                    {
                        const size_t valLen = strlen(value);

                        while (resultPosition + valLen + 1 > resultSize)
                        {
                            resultSize *= 2;
                            char *newResult = realloc(result, resultSize);

                            if (!newResult)
                            {
                                free(result);
                                free(varName);
                                SAFE_ERROR(ERR_CATEGORY_MEMORY, "MOSS: Memory allocation failed");
                                return NULL;
                            }

                            result = newResult;
                        }

                        strcat(result, value);
                        resultPosition += valLen;
                    }

                    free(varName);
                    i = end;
                }
            }
            else if (!isEnvVarChar(token[i + 1]))
            {
                result[resultPosition++] = '$';
                result[resultPosition] = '\0';
            }
            else
            {
                size_t start = i + 1;
                size_t end = start;

                while (end < len && isEnvVarChar(token[end]))
                    end++;

                const size_t varLen = end - start;
                char *varName = (char *)malloc(varLen + 1);

                if (!varName)
                {
                    free(result);
                    SAFE_ERROR(ERR_CATEGORY_MEMORY, "MOSS: Memory allocation failed");
                    return NULL;
                }

                strncpy(varName, token + start, varLen);
                varName[varLen] = '\0';
                char *value = getenv(varName);

                if (value)
                {
                    const size_t valLen = strlen(value);

                    while (resultPosition + valLen + 1 > resultSize)
                    {
                        resultSize *= 2;
                        char *newResult = realloc(result, resultSize);

                        if (!newResult)
                        {
                            free(result);
                            free(varName);
                            SAFE_ERROR(ERR_CATEGORY_MEMORY, "MOSS: Memory allocation failed");
                            return NULL;
                        }

                        result = newResult;
                    }

                    strcat(result, value);
                    resultPosition += valLen;
                }

                free(varName);
                i = end - 1;
            }
        }
    }

    return result;
}

int moss_cd(char **args)
{
    char *targetPath = NULL;
    char *expandedPath = NULL;
    char currentDir[PATH_MAX];

    if (getcwd(currentDir, sizeof(currentDir)) == NULL)
    {
        SAFE_ERROR(ERR_CATEGORY_PATH, "Failed to get current directory");
        return 1;
    }

    if (!args[1])
    {
        targetPath = getenv("HOME");

        if (!targetPath)
        {
            SAFE_ERROR(ERR_CATEGORY_PATH, "Home directory is not set");
            return 1;
        }

        targetPath = strdup(targetPath);
    }
    else
    {
        expandedPath = expand_path(args[1]);

        if (!expandedPath)
            return 1;

        targetPath = expandedPath;
    }

    if (strcmp(args[1], "-") == 0)
        printf("%s\n", targetPath);

    char resolved[PATH_MAX];

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

    return 1;
}

int moss_help(char **args)
{
    (void)args;
    printf("Type programs names and arguments, and hit enter.\n");
    printf("The following are built in:\n");

    for (size_t i = 0; i < NUM_BUILTINS; i++)
        printf("  %s\n", builtins[i].name);

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

    char *eq = strchr(args[1], '=');

    if (!eq)
    {
        SAFE_ERROR(ERR_CATEGORY_INPUT, "Usage: export NAME-VALUE");
        return 1;
    }

    *eq = '\0';
    const char *name = args[1];
    const char *value = eq + 1;

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