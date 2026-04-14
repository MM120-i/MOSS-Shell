#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>

#include "include/pathscanner.h"
#include "include/logging.h"
#include "include/datastructures/trie.h"
#include "src/modules/builtins/builtin.h"

private bool is_in_array(const char **array, size_t size, const char *str)
{
    for (size_t i = 0; i < size; i++)
        if (strcmp(array[i], str) == 0)
            return true;

    return false;
}

private char *strip_extension(const char *filename)
{
    char *dot = strrchr(filename, '.');

    if (dot && strcmp(dot, ".exe") == 0)
    {
        size_t len = dot - filename;
        char *result = malloc(len + 1);
        strncpy(result, filename, len);
        result[len] = '\0';
        return result;
    }

    return strdup(filename);
}

private bool is_executable(char *filePath)
{
    struct stat st;

    if (stat(filePath, &st) == -1)
        return false;

    bool executable = (st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH);

    return executable && S_ISREG(st.st_mode);
}

/**
 * Get PATH from env variable.
 * @returns A copy of PATH that caller must free, or default PATH
 */
char *path_scanner_get_path()
{
    char *path = getenv("PATH");

    if (path == NULL || path[0] == '\0')
        return strdup("/usr/bin:/bin");

    return strdup(path);
}

/**
 * Scan a single dir for .exe files
 * @returns 0 on success, -1 otherwise
 */
int path_scanner_scan_directory(const char *dirPath, char ***results, size_t *count)
{
    *results = NULL;
    *count = 0;

    if (!dirPath || !results || !count)
        return -1;

    DIR *dir = opendir(dirPath);

    if (!dir)
        return -1;

    size_t capacity = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)))
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char filePath[1024];
        snprintf(filePath, sizeof(filePath), "%s/%s", dirPath, entry->d_name);

        if (is_executable(filePath))
        {
            if (*count >= capacity)
            {
                capacity = (capacity == 0) ? 16 : capacity * 2;
                void *temp = realloc(*results, capacity * sizeof(char *));

                if (!temp)
                {
                    SAFE_ERROR(ERR_CATEGORY_MEMORY, "Realloc failed");
                    return -1;
                }
                else
                {
                    *results = (char **)temp;
                }
            }

            (*results)[*count] = strip_extension(entry->d_name);

            if (!(*results)[*count])
            {
                SAFE_ERROR(ERR_CATEGORY_MEMORY, "Strdup allocation failed");
                return -1;
            }

            (*count)++;
        }
    }

    closedir(dir);

    return 0;
}

/**
 * Scan all PATH dirs
 * @returns 0 on success, otherwise -1
 */
int path_scanner_scan_all(char ***results, size_t *count)
{
    *results = NULL;
    *count = 0;

    char *path = path_scanner_get_path();

    if (!path)
    {
        SAFE_ERROR(ERR_CATEGORY_PATH, "Empty path");
        return -1;
    }

    char *pathCopy = strdup(path);

    if (!pathCopy)
    {
        SAFE_ERROR(ERR_CATEGORY_MEMORY, "Strdup allocation failed");
        return -1;
    }

    char *token = strtok(pathCopy, ":");
    size_t tempCapacity = 0;
    size_t tempCount = 0;
    char **tempResults = NULL;

    while (token != NULL)
    {
        char **dirResults = NULL;
        size_t dirCount = 0;

        if (path_scanner_scan_directory(token, &dirResults, &dirCount) == 0)
        {
            for (size_t i = 0; i < dirCount; i++)
            {
                if (!is_in_array((const char **)tempResults, tempCount, dirResults[i]))
                {
                    if (tempCount >= tempCapacity)
                    {
                        tempCapacity = (tempCapacity == 0) ? 32 : tempCapacity * 2;
                        void *temp = realloc(tempResults, tempCapacity * sizeof(char *));

                        if (!temp)
                        {
                            SAFE_ERROR(ERR_CATEGORY_MEMORY, "Realloc failed");
                            return -1;
                        }
                        else
                        {
                            tempResults = (char **)temp;
                        }
                    }

                    tempResults[tempCount] = strdup(dirResults[i]);

                    if (!(tempResults[tempCount]))
                    {
                        SAFE_ERROR(ERR_CATEGORY_MEMORY, "Strdup allocation failed");
                        return -1;
                    }

                    tempCount++;
                }
                free(dirResults[i]);
            }
            free(dirResults);
        }

        token = strtok(NULL, ":");
    }

    free(pathCopy);
    free(path);
    *results = tempResults;
    *count = tempCount;

    return 0;
}

/**
 * Get all shell builtins commands
 * @returns 0 on success, otherwise -1
 */
int builtins_get_all(char ***results, size_t *count)
{
    if (!results || !count)
        return -1;

    *results = (char **)malloc(NUM_BUILTINS * sizeof(char *));

    if (!results)
    {
        SAFE_ERROR(ERR_CATEGORY_MEMORY, "Memory alloc failed");
        return -1;
    }

    for (size_t i = 0; i < NUM_BUILTINS; i++)
    {
        (*results)[i] = strdup(builtins[i].name);

        if (!((*results)[i]))
        {
            SAFE_ERROR(ERR_CATEGORY_MEMORY, "Strdup allocation failed");
            return -1;
        }
    }

    *count = NUM_BUILTINS;

    return 0;
}

/**
 * Populate trie with all PATH commands and builtins
 * @returns 0 on success, otherwise -1
 */
int path_scanner_populate_trie(Trie *trie)
{
    if (!trie)
        return -1;

    char **commands = NULL;
    size_t commandCount = 0;

    if (path_scanner_scan_all(&commands, &commandCount) == 0)
    {
        for (size_t i = 0; i < commandCount; i++)
        {
            trie_insert(trie, commands[i]);
            free(commands[i]);
        }

        free(commands);
    }

    char **builtinCommands = NULL;
    size_t builtinCount = 0;

    if (builtins_get_all(&builtinCommands, &builtinCount) == 0)
    {
        for (size_t i = 0; i < builtinCount; i++)
        {
            trie_insert(trie, builtinCommands[i]);
            free(builtinCommands[i]);
        }

        free(builtinCommands);
    }

    return 0;
}

/**
 * Scan dirs
 * path_sanner_scan_dirs();
 */