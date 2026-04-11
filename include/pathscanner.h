#pragma once

#define private static

#include "datastructures/trie.h"

char *path_scanner_get_path();
int path_scanner_scan_directory(const char *, char ***, size_t *);
int path_scanner_scan_all(char ***, size_t *);
int builtins_get_all(char ***, size_t *);
int path_scanner_populate_trie(Trie *);