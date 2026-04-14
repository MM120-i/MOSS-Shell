#pragma once

#include <stdbool.h>
#include <stddef.h>

#define private static

typedef struct Trie trie;

int autocomplete_init();
void autocomplete_destroy();
int autocomplete_get_completions(const char *, char ***, size_t *);
void autocomplete_free_results(char **, size_t);
size_t autocomplete_get_suggestion_count();
const char *autocomplete_get_suggestion(size_t);
void autocomplete_reset_cycle();
int autocomplete_cycle_next();
// completion_context_getPrefix();