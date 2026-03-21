#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../include/history.h"

// example: add 'ls' -> add 'ls' again -> expect false
private void test_duplicate_immediate(void **state)
{
    (void)state;

    history_init();
    history_add("ls");
    bool result = history_should_add("ls");
    assert_false(result);
    history_destroy();
}

// example: add 'ls', 'cd' -> add 'ls' -> expect true (different from last)
private void test_duplicate_after_other(void **state)
{
    (void)state;

    history_init();
    history_add("ls");
    history_add("cd");
    bool result = history_should_add("ls");
    assert_true(result);
    history_destroy();
}

// example: add 'ls' -> add 'cd' -> expect true
private void test_not_duplicate(void **state)
{
    (void)state;

    history_init();
    history_add("ls");
    bool result = history_should_add("cd");
    assert_true(result);
    history_destroy();
}

// example: add 'ls', 'ls' '' -> first duplicate returns false, empty also returns false
private void test_empty_after_duplicate(void **state)
{
    (void)state;

    history_init();
    history_add("ls");
    history_add("ls");
    bool dupResult = history_should_add("");
    assert_false(dupResult);
    history_add("");
    bool emptyResult = history_should_add("");
    assert_false(emptyResult);
    history_destroy();
}

// example: fill to 1000 -> add duplicate -> still works correctly
private void test_duplicate_at_capacity(void **state)
{
    (void)state;

    history_init();

    for (size_t i = 0; i < DEFAULT_HISTORY_SIZE; i++)
    {
        char cmd[20];
        snprintf(cmd, sizeof(cmd), "cmd_%ld", i);
        history_add(cmd);
    }

    bool result = history_should_add("cmd_999");
    assert_false(result);
    history_destroy();
}

// empty history -> file with only header
private void test_save_empty(void **state)
{
}

// save a few commands -> verify exact file contents
private void test_save_with_entries(void **state)
{
}

// load a valid file -> correct count + content
private void test_load_valid(void **state)
{
}

// load a missing file -> return -1
private void test_load_nonexistent(void **state)
{
}

// loads a corrupted file -> return -1
private void test_load_corrupted(void **state)
{
}

// do a 'roundtrip' so like: Save -> load -> identical history
private void test_save_load_roundtrip(void **state)
{
}

int main()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_duplicate_immediate),
        cmocka_unit_test(test_duplicate_after_other),
        cmocka_unit_test(test_not_duplicate),
        cmocka_unit_test(test_empty_after_duplicate),
        cmocka_unit_test(test_duplicate_at_capacity),

        cmocka_unit_test(test_save_empty),
        cmocka_unit_test(test_save_with_entries),
        cmocka_unit_test(test_load_valid),
        cmocka_unit_test(test_load_nonexistent),
        cmocka_unit_test(test_load_corrupted),
        cmocka_unit_test(test_save_load_roundtrip),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}