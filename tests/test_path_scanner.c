#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>

#include "../include/pathscanner.h"

private void test_path_get_from_env(void **state)
{
    (void)state;

    char *path = path_scanner_get_path();
    assert_non_null(path);
    free(path);
}

// Test 2: Get PATH returns NULL when not set (edge case)
// TODO
private void test_path_get_null(void **state)
{
    (void)state;
    // This'll need mocking, kinda hard ngl
}

// Test 3: Scan single valid directory
private void test_scan_single_directory(void **state)
{
    (void)state;

    char **results = NULL;
    size_t count = 0;
    int result = path_scanner_scan_directory("/usr/bin", &results, &count);

    assert_int_equal(result, 0);
    assert_true(count > 0);

    bool has_ls = false;

    for (size_t i = 0; i < count; i++)
        if (strcmp(results[i], "ls") == 0)
            has_ls = true;

    assert_true(has_ls);

    for (size_t i = 0; i < count; i++)
        free(results[i]);

    free(results);
}
// Test 4: Scan non-existent directory (should handle gracefully)
private void test_scan_nonexistent_directory(void **state)
{
    (void)state;

    char **results = NULL;
    size_t count = 0;
    int result = path_scanner_scan_directory("/nonexistent/path", &results, &count);

    assert_int_equal(result, -1);
    assert_null(results);
    assert_int_equal(count, 0);
}

// Test 5: Scan all PATH directories
private void test_scan_all_path(void **state)
{
    (void)state;

    char **results = NULL;
    size_t count = 0;
    int result = path_scanner_scan_all(&results, &count);

    assert_int_equal(result, 0);
    assert_true(count > 0);

    bool has_ls = false, has_gcc = false, has_make = false;

    for (size_t i = 0; i < count; i++)
    {
        if (strcmp(results[i], "ls") == 0)
            has_ls = true;

        if (strcmp(results[i], "gcc") == 0)
            has_gcc = true;

        if (strcmp(results[i], "make") == 0)
            has_make = true;
    }

    assert_true(has_ls);
    assert_true(has_gcc);
    assert_true(has_make);

    for (size_t i = 0; i < count; i++)
        free(results[i]);

    free(results);
}

// Test 6: No duplicates in results
private void test_no_duplicates(void **state)
{
    (void)state;

    char **results = NULL;
    size_t count = 0;

    path_scanner_scan_all(&results, &count);

    for (size_t i = 0; i < count; i++)
        for (size_t j = i + 1; j < count; j++)
            assert_string_not_equal(results[i], results[j]);

    for (size_t i = 0; i < count; i++)
        free(results[i]);

    free(results);
}

// Test 7: Get all builtins
private void test_get_builtins(void **state)
{
    (void)state;

    char **results = NULL;
    size_t count = 0;
    int result = builtins_get_all(&results, &count);

    assert_int_equal(result, 0);
    assert_true(count > 0);

    bool has_cd = false, has_echo = false, has_exit = false;

    for (size_t i = 0; i < count; i++)
    {
        if (strcmp(results[i], "cd") == 0)
            has_cd = true;

        if (strcmp(results[i], "echo") == 0)
            has_echo = true;

        if (strcmp(results[i], "exit") == 0)
            has_exit = true;
    }

    assert_true(has_cd);
    assert_true(has_echo);
    assert_true(has_exit);

    for (size_t i = 0; i < count; i++)
        free(results[i]);

    free(results);
}

// Test 8: Builtins list is stable (always same order)
private void test_builtins_stable(void **state)
{
    (void)state;

    char **results1 = NULL, **results2 = NULL;
    size_t count1 = 0, count2 = 0;

    builtins_get_all(&results1, &count1);
    builtins_get_all(&results2, &count2);

    assert_int_equal(count1, count2);

    for (size_t i = 0; i < count1; i++)
        assert_string_equal(results1[i], results2[i]);

    for (size_t i = 0; i < count1; i++)
        free(results1[i]);

    free(results1);

    for (size_t i = 0; i < count2; i++)
        free(results2[i]);

    free(results2);
}

// Test 9: Populate trie with PATH and builtins
private void test_populate_trie_with_commands(void **state)
{
    (void)state;

    Trie *trie = trie_create();
    assert_non_null(trie);
    int result = path_scanner_populate_trie(trie);

    assert_int_equal(result, 0);
    assert_true(trie_search(trie, "ls"));
    assert_true(trie_search(trie, "gcc"));
    assert_true(trie_search(trie, "cd"));
    assert_true(trie_search(trie, "echo"));
    assert_false(trie_search(trie, "xyznonexistent"));

    trie_destroy(trie);
}

// Test 10: Prefix search works on populated trie
private void test_trie_prefix_with_path(void **state)
{
    (void)state;

    Trie *trie = trie_create();
    path_scanner_populate_trie(trie);
    char **results = NULL;
    size_t count = 0;

    trie_prefix_search(trie, "ls", &results, &count);

    assert_true(count > 0);
    assert_non_null(results);

    bool found_ls = false;

    for (size_t i = 0; i < count; i++)
        if (strcmp(results[i], "ls") == 0)
            found_ls = true;

    assert_true(found_ls);

    for (size_t i = 0; i < count; i++)
        free(results[i]);

    free(results);
    trie_destroy(trie);
}

int main()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_path_get_from_env),
        cmocka_unit_test(test_scan_single_directory),
        cmocka_unit_test(test_scan_nonexistent_directory),
        cmocka_unit_test(test_scan_all_path),
        cmocka_unit_test(test_no_duplicates),
        cmocka_unit_test(test_get_builtins),
        cmocka_unit_test(test_builtins_stable),
        cmocka_unit_test(test_populate_trie_with_commands),
        cmocka_unit_test(test_trie_prefix_with_path),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);

    return 0;
}
