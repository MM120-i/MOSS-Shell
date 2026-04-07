#pragma once

#include <stdbool.h>

#define private static
#define NODE_SIZE 256

typedef struct Trie Trie;

typedef struct TrieNode
{
    struct TrieNode *children[NODE_SIZE];
    bool isEndofWord;
} TrieNode;

struct Trie
{
    TrieNode *root;
};

Trie *trie_create();
void trie_destroy(Trie *);
int trie_insert(Trie *, const char *);
bool trie_search(Trie *, const char *);
void trie_prefix_search(Trie *, const char *, char ***, size_t *);
size_t trie_prefix_count(Trie *, const char *);
