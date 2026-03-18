/**
 * So for history im creating a special wrapper that handles:
 * - Duplicate detection
 * - String ownership
 * - Navigation Cursor
 */

#pragma once

#include <stdbool.h>
#include <stdlib.h>

#define private static

typedef struct CircularBuffer CircularBuffer;

// this'll probably be moved to history.h later
typedef struct HistoryManager
{
    CircularBuffer *buffer;
    int cursor;
    char *lastCommand;
} HistoryManager;

struct CircularBuffer
{
    char **items;
    size_t capacity;
    size_t count;
    size_t head;
};

CircularBuffer *cb_create(size_t);
void cb_destory(CircularBuffer *);
int cb_add(CircularBuffer *, const char *);
const char *cb_get(const CircularBuffer *, size_t);
size_t cb_count(const CircularBuffer *);
size_t cb_capacity(const CircularBuffer *);
bool cb_isFull(const CircularBuffer *);
void cb_clear(CircularBuffer *);