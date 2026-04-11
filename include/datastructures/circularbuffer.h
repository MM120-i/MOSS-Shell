#pragma once

#include <stdbool.h>
#include <stdlib.h>

#define private static

typedef struct CircularBuffer CircularBuffer;

struct CircularBuffer
{
    char **items;
    size_t capacity;
    size_t count;
    size_t head;
};

CircularBuffer *cb_create(size_t);
void cb_destroy(CircularBuffer *);
int cb_add(CircularBuffer *, const char *);
const char *cb_get(const CircularBuffer *, size_t);
size_t cb_count(const CircularBuffer *);
size_t cb_capacity(const CircularBuffer *);
bool cb_isFull(const CircularBuffer *);
void cb_clear(CircularBuffer *);