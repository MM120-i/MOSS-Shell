#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <limits.h>
#include <errno.h>

#include "builtin.h"
#include "logging.h"

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

private char *expand_ls_path(const char *path)
{
    if (path[0] != '~')
        return NULL;

    char *home = getenv("HOME");

    if (!home)
        return NULL;

    if (path[1] == '\0')
    {
        return strdup(home);
    }
    else if (path[1] == '/')
    {
        size_t homeLen = strlen(home);
        size_t pathLen = strlen(path + 1);
        char *expanded = malloc(homeLen + pathLen + 1);

        if (!expanded)
            return NULL;

        strcpy(expanded, home);
        strcat(expanded, path + 1);
        return expanded;
    }
    else
    {
        struct passwd *pw = getpwnam(path + 1);

        if (!pw)
            return NULL;

        return strdup(pw->pw_dir);
    }
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
        char *expanded = expand_ls_path(dirPath);

        if (!expanded)
        {
            SAFE_ERROR(ERR_CATEGORY_PATH, "ls: cannot expand path '%s'", dirPath);
            return 1;
        }

        dirPath = expanded;
    }

    DIR *dir = opendir(dirPath);

    if (!dir)
    {
        SAFE_ERROR(ERR_CATEGORY_PATH, "ls: cannot access '%s'", dirPath);
        if (dirPath != targetDir && strcmp(dirPath, ".") != 0)
            free(dirPath);

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
    {
        if (dirPath != targetDir && strcmp(dirPath, ".") != 0)
            free(dirPath);
        return 1;
    }

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

    if (dirPath != targetDir && strcmp(dirPath, ".") != 0)
        free(dirPath);

    return 1;
}
