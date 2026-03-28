#include <string.h>
#include <stdlib.h>

#include "include/history.h"
#include "include/logging.h"
#include "include/logging.h"
#include "include/datastructures/circularbuffer.h"

private HistoryManager *historyManager = NULL;

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
        cb_destory(historyManager->buffer);
        free(historyManager);
        historyManager = NULL;
    }
}

int history_save(char *filePath)
{
    if (!filePath || !historyManager || !historyManager->buffer)
        return -1;

    FILE *file = fopen(filePath, "w");

    if (!file)
    {
        SAFE_ERROR(ERR_CATEGORY_PATH, "Failed to open file for saving: %s", filePath);
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
    LOG_INFO("History saved: %zu entries", count);

    return 0;
}

int history_load(char *filePath)
{

    if (!filePath)
        return -1;

    FILE *file = fopen(filePath, "r");

    if (!file)
    {
        LOG_ERROR("Failed to open history file: %s", filePath);
        return -1;
    }

    char line[LINE_LENGTH];

    if (!fgets(line, sizeof(line), file))
    {
        fclose(file);
        return -1;
    }

    if (strcmp(line, HISTORY_FILE_HEADER) != 0)
    {
        LOG_ERROR("Invalid history file header");
        fclose(file);
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
    LOG_INFO("History loaded: %d entries", loaded); // delete ths later

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

    int old_cursor = historyManager->cursor;
    historyManager->cursor++;

    if (historyManager->cursor >= (int)count)
    {
        historyManager->cursor = count;
        return -1;
    }

    if (old_cursor == 0)
        return historyManager->cursor;

    return old_cursor;
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