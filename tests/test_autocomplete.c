#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "../include/autocomplete.h"

private void test_autocomplete_init(void **state)
{
    (void)state;

    int result = autocomplete_init();
    assert_int_equal(result, 0);
}

private void test_autocomplete_get_completions_single_match(void **state)
{
    (void)state;

    char **results = NULL;
    size_t count = 0;
    int result = autocomplete_get_completions("exi", &results, &count);

    assert_int_equal(result, 0);
    assert_true(count > 0);

    bool has_exit = false;

    for (size_t i = 0; i < count; i++)
        if (strcmp(results[i], "exit") == 0)
            has_exit = true;

    assert_true(has_exit);
    autocomplete_free_results(results, count);
}

private void test_autocomplete_get_completions_multiple_matches(void **state)
{
    (void)state;

    char **results = NULL;
    size_t count = 0;
    int result = autocomplete_get_completions("l", &results, &count);

    assert_int_equal(result, 0);
    assert_true(count > 1);

    autocomplete_free_results(results, count);
}

private void test_autocomplete_get_completions_no_match(void **state)
{
    (void)state;

    char **results = NULL;
    size_t count = 0;
    int result = autocomplete_get_completions("xyznonexistent123", &results, &count);

    assert_int_equal(result, 0);
    assert_int_equal(count, 0);
    assert_null(results);
}

private void test_autocomplete_get_completions_builtins(void **state)
{
    (void)state;

    char **results = NULL;
    size_t count = 0;
    int result = autocomplete_get_completions("cd", &results, &count);

    assert_int_equal(result, 0);
    assert_true(count > 0);

    bool has_cd = false;

    for (size_t i = 0; i < count; i++)
        if (strcmp(results[i], "cd") == 0)
            has_cd = true;

    assert_true(has_cd);

    autocomplete_free_results(results, count);
}

private void test_autocomplete_cycle(void **state)
{
    (void)state;

    autocomplete_get_completions("e", NULL, NULL);

    size_t count = autocomplete_get_suggestion_count();
    assert_true(count >= 1);

    const char *first = autocomplete_get_suggestion(0);
    assert_non_null(first);

    for (size_t i = 0; i < count; i++)
        autocomplete_cycle_next();

    const char *after_cycle = autocomplete_get_suggestion(0);
    assert_string_equal(first, after_cycle);

    autocomplete_reset_cycle();
}

private void test_autocomplete_cycle_wrap_around(void **state)
{
    (void)state;

    autocomplete_get_completions("e", NULL, NULL);

    size_t count = autocomplete_get_suggestion_count();
    assert_true(count >= 1);

    const char *first_before = autocomplete_get_suggestion(0);
    assert_non_null(first_before);

    for (size_t i = 0; i < count + 5; i++)
        autocomplete_cycle_next();

    const char *first_after = autocomplete_get_suggestion(0);
    assert_string_equal(first_before, first_after);

    autocomplete_reset_cycle();
}

private void test_autocomplete_destroy(void **state)
{
    (void)state;

    autocomplete_destroy();

    int result = autocomplete_init();
    assert_int_equal(result, 0);

    autocomplete_destroy();
}

int main()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_autocomplete_init),
        cmocka_unit_test(test_autocomplete_get_completions_single_match),
        cmocka_unit_test(test_autocomplete_get_completions_multiple_matches),
        cmocka_unit_test(test_autocomplete_get_completions_no_match),
        cmocka_unit_test(test_autocomplete_get_completions_builtins),
        cmocka_unit_test(test_autocomplete_cycle),
        cmocka_unit_test(test_autocomplete_cycle_wrap_around),
        cmocka_unit_test(test_autocomplete_destroy),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}