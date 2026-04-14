/**
 * @note This feature is not integrated with the app yet.
 */

#include <stdlib.h>
#include <string.h>

#include "include/autocomplete.h"
#include "include/logging.h"
#include "include/pathscanner.h"
#include "include/datastructures/trie.h"

private Trie *completionTrie = NULL;
private char **currentSuggestions = NULL;
private size_t currentCount = 0;
private size_t currentIndex = 0;

int autocomplete_init()
{
    completionTrie = trie_create();

    if (!completionTrie)
    {
        SAFE_ERROR(ERR_CATEGORY_MEMORY, "Failed to create a trie");
        return -1;
    }

    if (path_scanner_populate_trie(completionTrie) == -1)
        LOG_WARN("Failed to populate trie from PATH");

    return 0;
}

void autocomplete_destroy()
{
    if (completionTrie)
    {
        trie_destroy(completionTrie);
        completionTrie = NULL;
    }

    if (currentSuggestions)
    {
        for (size_t i = 0; i < currentCount; i++)
            free(currentSuggestions[i]);

        free(currentSuggestions);
        currentSuggestions = NULL;
    }

    currentCount = 0;
    currentIndex = 0;
}

int autocomplete_get_completions(const char *prefix, char ***results, size_t *count)
{
    if (!prefix || !results || !count)
        return -1;

    if (!completionTrie)
    {
        *results = NULL;
        *count = 0;
        return -1;
    }

    if (currentSuggestions)
    {
        for (size_t i = 0; i < currentCount; i++)
            free(currentSuggestions[i]);

        free(currentSuggestions);
        currentSuggestions = NULL;
    }

    currentIndex = 0;

    trie_prefix_search(completionTrie, prefix, &currentSuggestions, &currentCount);

    if (currentCount > 0 && currentSuggestions)
    {
        *results = malloc(currentCount * sizeof(char *));

        if (!*results)
        {
            SAFE_ERROR(ERR_CATEGORY_MEMORY, "Memory allocation failed");
            *count = 0;
            return -1;
        }

        for (size_t i = 0; i < currentCount; i++)
        {
            (*results)[i] = strdup(currentSuggestions[i]);

            if (!((*results)[i]))
            {
                SAFE_ERROR(ERR_CATEGORY_MEMORY, "Strdup allocation failed");
                return -1;
            }
        }

        *count = currentCount;
    }
    else
    {
        *results = NULL;
        *count = 0;
    }

    return 0;
}

void autocomplete_free_results(char **results, size_t count)
{
    if (!results)
        return;

    for (size_t i = 0; i < count; i++)
        free(results[i]);

    free(results);
}

size_t autocomplete_get_suggestion_count()
{
    return currentCount;
}

const char *autocomplete_get_suggestion(size_t index)
{
    if (!currentSuggestions || currentCount == 0)
        return NULL;

    if (index >= currentCount)
        return NULL;

    return currentSuggestions[index];
}

void autocomplete_reset_cycle()
{
    currentIndex = 0;
}

int autocomplete_cycle_next()
{
    if (currentCount == 0)
        return -1;

    currentIndex = (currentIndex + 1) % currentCount;

    return 0;
}

/**
 * Context aware
 * completion_context_getPrefix();
 */