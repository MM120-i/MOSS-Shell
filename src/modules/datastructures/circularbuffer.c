#include <string.h>

#include "include/datastructures/circularbuffer.h"
#include "include/logging.h"

CircularBuffer *cb_create(size_t capacity)
{
    CircularBuffer *cb = (CircularBuffer *)malloc(sizeof(CircularBuffer));

    if (!cb)
    {
        SAFE_ERROR(ERR_CATEGORY_MEMORY, "Failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    cb->items = (char **)malloc(capacity * sizeof(char *));

    if (!cb->items)
    {
        SAFE_ERROR(ERR_CATEGORY_MEMORY, "Failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    cb->capacity = capacity;
    cb->count = 0;
    cb->head = 0;

    return cb;
}

int cb_add(CircularBuffer *cb, const char *item)
{
    if (cb->count == cb->capacity)
        free(cb->items[cb->head]);
    else
        cb->count++;

    cb->items[cb->head] = strdup(item);
    cb->head = (cb->head + 1) % cb->capacity;

    return 0;
}

const char *cb_get(const CircularBuffer *cb, size_t index)
{
    if (index >= cb->count)
        return NULL;

    size_t tail = (cb->head + cb->capacity - cb->count) % cb->capacity;
    size_t actual = (tail + index) % cb->capacity;

    return cb->items[actual];
}

void cb_destroy(CircularBuffer *cb)
{
    if (!cb)
        return;

    for (size_t i = 0; i < cb->count; i++)
    {
        size_t index = (cb->head + cb->capacity - cb->count + i) % cb->capacity;
        free(cb->items[index]);
    }

    free(cb->items);
    free(cb);
}

size_t cb_count(const CircularBuffer *cb)
{
    return cb ? cb->count : 0;
}

size_t cb_capacity(const CircularBuffer *cb)
{
    return cb ? cb->capacity : 0;
}

bool cb_isFull(const CircularBuffer *cb)
{
    return cb ? (cb->count == cb->capacity) : false;
}

void cb_clear(CircularBuffer *cb)
{
    if (!cb)
        return;

    for (size_t i = 0; i < cb->count; i++)
    {
        size_t index = (cb->head + cb->capacity - cb->count + i) % cb->capacity;
        free(cb->items[index]);
        cb->items[index] = NULL;
    }

    cb->count = 0;
    cb->head = 0;
}