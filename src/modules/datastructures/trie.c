#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/datastructures/trie.h"
#include "include/logging.h"

private void collect_all_words(TrieNode *node, const char *prefix, char ***results, size_t *count, size_t *capacity)
{
    if (!node)
        return;

    if (node->isEndofWord)
    {
        if (*count >= *capacity)
        {
            *capacity = (*capacity) ? 16 : *capacity * 2;
            *results = (char **)realloc(*results, *capacity * sizeof(char *));
        }

        (*results)[*count] = strdup(prefix);
        (*count)++;
    }

    for (size_t i = 0; i < NODE_SIZE; i++)
    {
        if (node->children[i])
        {
            char newPrefix[1024];
            snprintf(newPrefix, sizeof(newPrefix), "%s%c", prefix, (char)i);
            collect_all_words(node->children[i], newPrefix, results, count, capacity);
        }
    }
}

private size_t count_all_words(TrieNode *node)
{
    if (!node)
        return 0;

    size_t count = node->isEndofWord ? 1 : 0;

    for (size_t i = 0; i < NODE_SIZE; i++)
        count += count_all_words(node->children[i]);

    return count;
}

private size_t count_words_with_prefix(TrieNode *node, const char *prefix)
{
    if (!node)
        return 0;

    TrieNode *curr = node;

    for (size_t i = 0; prefix[i] != '\0'; i++)
    {
        unsigned char c = (unsigned char)prefix[i];

        if (!curr->children[c])
            return 0;

        curr = curr->children[c];
    }

    return count_all_words(curr);
}

private void collect_words_with_prefix(TrieNode *node, const char *prefix, char ***results, size_t *count, size_t *capacity)
{
    if (!node)
        return;

    if (prefix[0] == '\0')
        return;

    TrieNode *curr = node;

    for (size_t i = 0; prefix[i] != '\0'; i++)
    {
        unsigned char c = (unsigned char)prefix[i];

        if (!curr->children[c])
            return;

        curr = curr->children[c];
    }

    collect_all_words(curr, prefix, results, count, capacity);
}

private TrieNode *node_create()
{
    TrieNode *node = (TrieNode *)calloc(1, sizeof(TrieNode));

    if (!node)
    {
        SAFE_ERROR(ERR_CATEGORY_MEMORY, "Allocation failed");
        return NULL;
    }

    return node;
}

private void node_destroy(TrieNode *node)
{
    if (!node)
        return;

    for (size_t i = 0; i < NODE_SIZE; i++)
        if (node->children[i])
            node_destroy(node->children[i]);

    free(node);
}

/**
 * Allocates memory for the trie, and init the root node
 */
Trie *trie_create()
{
    Trie *trie = (Trie *)malloc(sizeof(Trie));

    if (!trie)
    {
        SAFE_ERROR(ERR_CATEGORY_MEMORY, "Allocation failed");
        return NULL;
    }

    trie->root = node_create();

    if (!trie->root)
    {
        free(trie);
        SAFE_ERROR(ERR_CATEGORY_MEMORY, "Allocation failed");
        return NULL;
    }

    return trie;
}

/**
 * Recursively frees all the nodes in the trie
 */
void trie_destroy(Trie *trie)
{
    if (!trie)
        return;

    node_destroy(trie->root);
    free(trie);
}

/**
 * Traverse and create nodes for each char
 * Marks the last node as "end of word"
 */
int trie_insert(Trie *trie, const char *word)
{
    if (!trie || !word)
        return -1;

    TrieNode *curr = trie->root;

    for (size_t i = 0; word[i]; i++)
    {
        unsigned char c = (unsigned char)word[i];

        if (!curr->children[c])
        {
            curr->children[c] = node_create();

            if (!curr->children[c])
                return -1;
        }

        curr = curr->children[c];
    }

    curr->isEndofWord = true;

    return 0;
}

/**
 * Traverse nodes for each char
 * Return true only if word exists and mark it as "end of word", otherwise return false
 */
bool trie_search(Trie *trie, const char *word)
{
    if (!trie || !word)
        return false;

    TrieNode *curr = trie->root;

    for (size_t i = 0; word[i]; i++)
    {
        unsigned char c = (unsigned char)word[i];

        if (!curr->children[c])
            return false;

        curr = curr->children[c];
    }

    return curr->isEndofWord;
}

/**
 * Traverse to the node matching the prefix
 * Collects ALL words from that node (recursively - dfs)
 * Gives us an array of strings and count
 */
void trie_prefix_search(Trie *trie, const char *prefix, char ***results, size_t *count)
{
    if (!trie || !prefix)
        return;

    *results = NULL;
    *count = 0;

    size_t capacity = 0;
    collect_words_with_prefix(trie->root, prefix, results, count, &capacity);
}

/**
 * Same as prefix_search() but just returns the count
 */
size_t trie_prefix_count(Trie *trie, const char *prefix)
{
    if (!trie)
        return 0;

    return count_words_with_prefix(trie->root, prefix);
}