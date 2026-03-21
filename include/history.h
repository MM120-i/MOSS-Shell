#pragma once

#include <stdbool.h>

#define private static
#define DEFAULT_HISTORY_SIZE 1000
#define DEFAULT_HISTORY_FILE "~/.moss_history"
#define HISTORY_FILE_HEADER "# moss shell history\n"

#include "include/datastructures/circularbuffer.h"

typedef struct HistoryManager
{
    CircularBuffer *buffer;
    int cursor;
    char *lastCommand;
} HistoryManager;

void history_init();
void history_add(const char *);
bool history_should_add(const char *);
void history_destroy();
int history_save(char *);