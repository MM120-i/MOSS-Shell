#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "include/history.h"
#include "include/logging.h"
#include "include/logging.h"
#include "include/datastructures/circularbuffer.h"

private HistoryManager *historyManager = NULL;

private char *expand_path(const char *path)
{
    if (!path || path[0] != '~')
        return NULL;

    char *home = getenv("HOME");

    if (!home)
        return NULL;
        
    char *result = (char *)malloc(strlen(home) + strlen(path));

    if (!result)
        return NULL;

    strcpy(result, home);
    strcat(result, path + 1);

    return result;
}

void history_init()
{
    historyManager = (HistoryManager *)malloc(sizeof(HistoryManager));

    if (!historyManager)
    {
        SAFE_ERROR(ERR_CATEGORY_MEMORY, "Allocation failed");
        return;
    }

    historyManager->buffer = cb_create(DEFAULT_HISTORY_SIZE);
    historyManager->cursor = cb_count(historyManager->buffer);
    historyManager->lastCommand = NULL;
}

bool history_should_add(const char *cmd)
{
    if (!cmd || cmd[0] == '\0')
        return false;

    if (cb_count(historyManager->buffer) == 0)
        return true;

    const char *last = cb_get(historyManager->buffer, cb_count(historyManager->buffer) - 1);

    if (last && strcmp(cmd, last) == 0)
        return false;

    return true;
}

void history_add(const char *cmd)
{
    if (history_should_add(cmd))
    {
        cb_add(historyManager->buffer, cmd);
        historyManager->cursor = cb_count(historyManager->buffer);
    }
}

void history_destroy()
{
    if (historyManager)
    {
        cb_destroy(historyManager->buffer);
        free(historyManager);
        historyManager = NULL;
    }
}

int history_save(char *filePath)
{
    if (!filePath || !historyManager || !historyManager->buffer)
        return -1;

    char *expandedPath = NULL;
    const char *pathToUse = filePath;

    if (filePath[0] == '~')
    {
        expandedPath = expand_path(filePath);

        if (!expandedPath)
        {
            SAFE_ERROR(ERR_CATEGORY_PATH, "Failed to expand path: %s", filePath);
            return -1;
        }

        pathToUse = expandedPath;
    }

    FILE *file = fopen(pathToUse, "w");

    if (!file)
    {
        SAFE_ERROR(ERR_CATEGORY_PATH, "Failed to open file for saving: %s", pathToUse);
        free(expandedPath);
        return -1;
    }

    fprintf(file, HISTORY_FILE_HEADER);
    size_t count = cb_count(historyManager->buffer);

    for (size_t i = 0; i < count; i++)
    {
        const char *cmd = cb_get(historyManager->buffer, i);
        if (cmd)
            fprintf(file, "%s\n", cmd);
    }

    fclose(file);
    free(expandedPath);
    LOG_INFO("History saved: %zu entries", count);

    return 0;
}

int history_load(char *filePath)
{
    if (!filePath)
        return -1;

    char *expandedPath = NULL;
    const char *pathToUse = filePath;

    if (filePath[0] == '~')
    {
        expandedPath = expand_path(filePath);
        if (!expandedPath)
        {
            LOG_ERROR("Failed to expand path: %s", filePath);
            return -1;
        }
        pathToUse = expandedPath;
    }

    FILE *file = fopen(pathToUse, "r");

    if (!file)
    {
        LOG_ERROR("Failed to open history file: %s", pathToUse);
        free(expandedPath);
        return -1;
    }

    char line[LINE_LENGTH];

    if (!fgets(line, sizeof(line), file))
    {
        fclose(file);
        free(expandedPath);
        return -1;
    }

    if (strcmp(line, HISTORY_FILE_HEADER) != 0)
    {
        LOG_ERROR("Invalid history file header");
        fclose(file);
        free(expandedPath);
        return -1;
    }

    int loaded = 0;

    while (fgets(line, sizeof(line), file))
    {
        size_t len = strlen(line);

        if (len > 0 && line[len - 1] == '\n')
            line[len - 1] = '\0';

        if (line[0] != '\0')
        {
            history_add(line);
            loaded++;
        }
    }

    fclose(file);
    free(expandedPath);
    LOG_INFO("History loaded: %d entries", loaded);

    return 0;
}

int history_count()
{
    if (!historyManager || !historyManager->buffer)
        return 0;

    return (int)cb_count(historyManager->buffer);
}

const char *history_get(int index)
{
    if (!historyManager || !historyManager->buffer)
        return NULL;

    return cb_get(historyManager->buffer, (size_t)index);
}

int history_next()
{
    if (!historyManager || !historyManager->buffer)
        return -1;

    size_t count = cb_count(historyManager->buffer);

    if (historyManager->cursor >= (int)count)
    {
        historyManager->cursor = count;
        return -1;
    }

    historyManager->cursor++;

    if (historyManager->cursor >= (int)count)
    {
        historyManager->cursor = count;
        return -1;
    }

    return historyManager->cursor;
}

int history_prev()
{
    if (!historyManager || !historyManager->buffer)
        return -1;

    size_t count = cb_count(historyManager->buffer);

    if (count == 0)
        return -1;

    if (historyManager->cursor >= (int)count)
        historyManager->cursor = count - 1;
    else if (historyManager->cursor <= 0)
        historyManager->cursor = 0;
    else
        historyManager->cursor--;

    return historyManager->cursor;
}

void history_reset_cursor()
{
    if (!historyManager || !historyManager->buffer)
        return;

    historyManager->cursor = cb_count(historyManager->buffer);
}

void history_clear()
{
    if (!historyManager || !historyManager->buffer)
        return;

    cb_clear(historyManager->buffer);
}