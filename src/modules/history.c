#include <string.h>
#include <stdlib.h>

#include "include/history.h"
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
    historyManager->cursor = -1;
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
        cb_add(historyManager->buffer, cmd);
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

// todo
int history_save(char *cmd)
{
    return 0;
}

// todo
int history_load(char *cmd)
{
    return 0;
}

// todo
int history_count()
{
    return 0;
}

// todo
const char *history_get(int num)
{
    return "";
}

// todo
int history_next()
{
    return 0;
}

// todo
int history_prev()
{
    return 0;
}

// todo
void history_reset_cursor() {}