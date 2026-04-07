#pragma once

#include <stdbool.h>
#include <stddef.h>

#define private static

typedef struct Trie Trie;

Trie *trie_create();
void trie_destroy(Trie *);
int trie_insert(Trie *, const char *);
bool trie_search(Trie *, const char *);
void trie_prefix_search(Trie *, const char *, char ***, size_t *);
size_t trie_prefix_count(Trie *, const char *);
