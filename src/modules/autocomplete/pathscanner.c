#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>

#include "include/pathscanner.h"
#include "include/logging.h"
#include "include/datastructures/trie.h"

/**
 * Get PATH from env variable.
 * @returns A copy of PATH that caller must free, or default PATH
 */
char *path_scanner_get_path()
{
    return "";
}

/**
 * Scan a single dir for .exe files
 * @returns 0 on success, -1 otherwise
 */
int path_scanner_scan_directory(const char *dirPath, char ***result, size_t *count)
{
    return 0;
}

/**
 * Scan all PATH dirs
 * @returns 0 on success, otherwise -1
 */
int path_scanner_scan_all(char ***result, size_t *count)
{
    return 0;
}

/**
 * Get all shell builtins commands
 * @returns 0 on success, otherwise -1
 */
int builtins_get_all(char ***results, size_t *count)
{
    return 0;
}

/**
 * Populate trie with all PATH commands and builtins
 * @returns 0 on success, otherwise -1
 */
int path_scanner_populate_trie(Trie *trie)
{
    return 0;
}
