#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "../include/history.h"

// example: add 'ls' -> add 'ls' again -> expect false
private void test_duplicate_immediate(void **state)
{
}

// example: add 'ls', 'cd' -> add 'ls' -> expect true (different from last)
private void test_duplicate_after_other(void **state)
{
}

// example: add 'ls' -> add 'cd' -> expect true
private void test_not_duplicate(void **state)
{
}

// example: add 'ls', 'ls' '' -> first duplicate returns false, empty also returns false
private void test_empty_after_duplicate(void **state)
{
}

// example: fill to 1000 -> add duplicate -> still works correctly
private void test_duplicate_at_capacity(void **state)
{
}

// TODO: add more test cases:

int main()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_duplicate_immediate),
        cmocka_unit_test(test_duplicate_after_other),
        cmocka_unit_test(test_not_duplicate),
        cmocka_unit_test(test_empty_after_duplicate),
        cmocka_unit_test(test_duplicate_at_capacity),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}