#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

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
    {"ls", moss_ls},
};

const int NUM_BUILTINS = sizeof(builtins) / sizeof(struct builtin);
private char *moss_oldpwd = NULL;

private int doChdir(void *ctx)
{
    ChdirContext *chdirCtx = (ChdirContext *)ctx;
    return chdir(chdirCtx->path);
}

private void strmode(mode_t mode, char *buf)
{

    buf[0] = S_ISDIR(mode) ? 'd' : S_ISLNK(mode) ? 'l'
                                                 : '-';
    buf[1] = (mode & S_IRUSR) ? 'r' : '-';
    buf[2] = (mode & S_IWUSR) ? 'w' : '-';
    buf[3] = (mode & S_IXUSR) ? 'x' : '-';
    buf[4] = (mode & S_IRGRP) ? 'r' : '-';
    buf[5] = (mode & S_IWGRP) ? 'w' : '-';
    buf[6] = (mode & S_IXGRP) ? 'x' : '-';
    buf[7] = (mode & S_IROTH) ? 'r' : '-';
    buf[8] = (mode & S_IWOTH) ? 'w' : '-';
    buf[9] = (mode & S_IXOTH) ? 'x' : '-';
    buf[10] = '\0';
}

private int compareStrings(const void *a, const void *b)
{
    const char *strA = *(const char **)a;
    const char *strB = *(const char **)b;

    return strcmp(strA, strB);
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
            char *expanded = (char *)malloc(totalLength);

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

    if (args[1] && strcmp(args[1], "-") == 0)
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

int moss_ls(char **args)
{
    bool showAll = false, longFormat = false;
    char *targetDir = NULL;

    for (size_t i = 1; args[i]; i++)
    {
        if (args[i][0] == '-' && args[i][1] != '\0')
        {
            for (size_t j = 1; args[i][j]; j++)
            {
                if (args[i][j] == 'a')
                    showAll = true;
                else if (args[i][j] == 'l')
                    longFormat = true;
                else
                {
                    SAFE_ERROR(ERR_CATEGORY_INPUT, "ls: Invalid option -%c", args[i][j]);
                    return 1;
                }
            }
        }
        else
        {
            targetDir = args[i];
        }
    }

    char *dirPath = targetDir ? targetDir : ".";

    if (dirPath[0] == '~')
    {
        char *home = getenv("HOME");

        if (!home)
        {
            SAFE_ERROR(ERR_CATEGORY_PATH, "ls: HOME not set");
            return 1;
        }

        if (dirPath[1] == '\0')
        {
            dirPath = home;
        }
        else if (dirPath[1] == '/')
        {
            size_t homeLen = strlen(home);
            size_t pathLen = strlen(dirPath + 1);
            char *expanded = malloc(homeLen + pathLen + 1);

            if (!expanded)
            {
                SAFE_ERROR(ERR_CATEGORY_MEMORY, "ls: allocation failed");
                return 1;
            }

            strcpy(expanded, home);
            strcat(expanded, dirPath + 1);
            dirPath = expanded;
        }
        else
        {
            struct passwd *pw = getpwnam(dirPath + 1);

            if (!pw)
            {
                SAFE_ERROR(ERR_CATEGORY_PATH, "ls: unknown user '%s'", dirPath + 1);
                return 1;
            }

            dirPath = pw->pw_dir;
        }
    }

    DIR *dir = opendir(dirPath);

    if (!dir)
    {
        SAFE_ERROR(ERR_CATEGORY_PATH, "ls: cannot access '%s'", dirPath);
        return 1;
    }

    struct dirent *entry;
    char **entries = NULL;
    int numberOfFiles = 0;

    while ((entry = readdir(dir)) != NULL)
    {
        if (!showAll && entry->d_name[0] == '.')
            continue;

        entries = realloc(entries, sizeof(char *) * (numberOfFiles + 1));
        entries[numberOfFiles] = strdup(entry->d_name);
        numberOfFiles++;
    }

    closedir(dir);

    if (numberOfFiles == 0)
        return 1;

    qsort(entries, numberOfFiles, sizeof(char *), compareStrings);

    if (longFormat)
    {
        struct stat st;
        char fullPath[PATH_MAX];

        for (size_t i = 0; i < numberOfFiles; i++)
        {
            if (strcmp(dirPath, ".") == 0)
                snprintf(fullPath, sizeof(fullPath), "%s", entries[i]);
            else
                snprintf(fullPath, sizeof(fullPath), "%s/%s", dirPath, entries[i]);

            if (lstat(fullPath, &st) == -1)
                continue;

            char permission[11];
            strmode(st.st_mode, permission);
            struct passwd *pw = getpwuid(st.st_uid);
            struct group *gr = getgrgid(st.st_gid);
            char date[64];
            struct tm *tmInfo = localtime(&st.st_mtime);
            strftime(date, sizeof(date), "%b %d %H:%M", tmInfo);
            char colorCode[8] = "";

            if (S_ISDIR(st.st_mode))
                strcpy(colorCode, "\033[34m");
            else if (st.st_mode & S_IXUSR)
                strcpy(colorCode, "\033[32m");
            else if (S_ISLNK(st.st_mode))
                strcpy(colorCode, "\033[36m");

            printf("%s %2ld %-8s %-8s %8ld %s %s%s\033[0m\n",
                   permission, (long)st.st_nlink,
                   pw ? pw->pw_name : "unknown",
                   gr ? gr->gr_name : "unknown",
                   (long)st.st_size,
                   date,
                   colorCode,
                   entries[i]);
        }
    }
    else
    {
        for (size_t i = 0; i < numberOfFiles; i++)
        {
            struct stat st;
            char fullPath[PATH_MAX];

            if (strcmp(dirPath, ".") == 0)
                snprintf(fullPath, sizeof(fullPath), "%s", entries[i]);
            else
                snprintf(fullPath, sizeof(fullPath), "%s/%s", dirPath, entries[i]);

            if (lstat(fullPath, &st) == -1)
            {
                printf("%s ", entries[i]);
                continue;
            }

            char colorCode[8] = "";

            if (S_ISDIR(st.st_mode))
                strcpy(colorCode, "\033[34m");
            else if (st.st_mode & S_IXUSR)
                strcpy(colorCode, "\033[32m");
            else if (S_ISLNK(st.st_mode))
                strcpy(colorCode, "\033[36m");

            printf("%s%s\033[0m ", colorCode, entries[i]);
        }

        printf("\n");
    }

    for (size_t i = 0; i < numberOfFiles; i++)
        free(entries[i]);

    free(entries);

    return 1;
}