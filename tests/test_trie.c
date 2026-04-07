#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "../include/datastructures/trie.h"

static void test_trie_create(void **state)
{
    (void)state;

    Trie *trie = trie_create();
    assert_non_null(trie);
    trie_destroy(trie);
}

static void test_trie_insert_single(void **state)
{
    (void)state;

    Trie *trie = trie_create();
    int result = trie_insert(trie, "hello");
    assert_int_equal(result, 0);

    assert_true(trie_search(trie, "hello"));
    trie_destroy(trie);
}

static void test_trie_insert_multiple(void **state)
{
    (void)state;

    Trie *trie = trie_create();

    trie_insert(trie, "hello");
    trie_insert(trie, "world");
    trie_insert(trie, "help");

    assert_true(trie_search(trie, "hello"));
    assert_true(trie_search(trie, "world"));
    assert_true(trie_search(trie, "help"));
    assert_false(trie_search(trie, "hell"));
    assert_false(trie_search(trie, "xyz"));

    trie_destroy(trie);
}

static void test_trie_search_not_found(void **state)
{
    (void)state;

    Trie *trie = trie_create();
    trie_insert(trie, "hello");

    assert_false(trie_search(trie, "hell"));
    assert_false(trie_search(trie, "hella"));
    assert_false(trie_search(trie, "world"));

    trie_destroy(trie);
}

static void test_trie_prefix_search(void **state)
{
    (void)state;

    Trie *trie = trie_create();

    trie_insert(trie, "hello");
    trie_insert(trie, "help");
    trie_insert(trie, "helicopter");
    trie_insert(trie, "world");

    char **results = NULL;
    size_t count = 0;

    trie_prefix_search(trie, "hel", &results, &count);

    assert_int_equal(count, 3);

    assert_non_null(results);
    assert_string_equal(results[0], "hello");
    assert_string_equal(results[1], "help");
    assert_string_equal(results[2], "helicopter");

    for (size_t i = 0; i < count; i++)
    {
        free(results[i]);
    }
    free(results);

    trie_destroy(trie);
}

static void test_trie_prefix_search_no_results(void **state)
{
    (void)state;

    Trie *trie = trie_create();
    trie_insert(trie, "hello");

    char **results = NULL;
    size_t count = 0;

    trie_prefix_search(trie, "xyz", &results, &count);

    assert_int_equal(count, 0);
    assert_null(results);

    trie_destroy(trie);
}

static void test_trie_prefix_search_empty_prefix(void **state)
{
    (void)state;

    Trie *trie = trie_create();
    trie_insert(trie, "hello");
    trie_insert(trie, "world");

    char **results = NULL;
    size_t count = 0;

    trie_prefix_search(trie, "", &results, &count);

    assert_int_equal(count, 0);
    assert_null(results);

    trie_destroy(trie);
}

static void test_trie_case_sensitivity(void **state)
{
    (void)state;

    Trie *trie = trie_create();
    trie_insert(trie, "Hello");

    assert_true(trie_search(trie, "Hello"));
    assert_false(trie_search(trie, "hello"));
    assert_false(trie_search(trie, "HELLO"));

    trie_destroy(trie);
}

static void test_trie_prefix_count(void **state)
{
    (void)state;

    Trie *trie = trie_create();

    trie_insert(trie, "hello");
    trie_insert(trie, "help");
    trie_insert(trie, "helicopter");
    trie_insert(trie, "world");

    assert_int_equal(trie_prefix_count(trie, "hel"), 3);
    assert_int_equal(trie_prefix_count(trie, "hello"), 1);
    assert_int_equal(trie_prefix_count(trie, "xyz"), 0);
    assert_int_equal(trie_prefix_count(trie, ""), 4);

    trie_destroy(trie);
}

int main()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_trie_create),
        cmocka_unit_test(test_trie_insert_single),
        cmocka_unit_test(test_trie_insert_multiple),
        cmocka_unit_test(test_trie_search_not_found),
        cmocka_unit_test(test_trie_prefix_search),
        cmocka_unit_test(test_trie_prefix_search_no_results),
        cmocka_unit_test(test_trie_prefix_search_empty_prefix),
        cmocka_unit_test(test_trie_case_sensitivity),
        cmocka_unit_test(test_trie_prefix_count),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
