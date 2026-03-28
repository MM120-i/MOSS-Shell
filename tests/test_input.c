#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "include/input.h"
#include "include/history.h"
#include "include/signals.h"

private char *mock_input_buffer = NULL;
private size_t mock_input_pos = 0;
private size_t mock_input_len = 0;
private char *mock_output_buffer = NULL;
private size_t mock_output_pos = 0;
private size_t mock_output_cap = 0;

private void reset_mock_buffers(void)
{
    mock_input_pos = 0;
    mock_output_pos = 0;

    if (mock_output_buffer)
        mock_output_buffer[0] = '\0';
}

private ssize_t mock_read_fn(int fd, void *buf, size_t count)
{
    (void)fd;
    (void)count;

    if (!mock_input_buffer || mock_input_pos >= mock_input_len)
        return 0;

    char *c = (char *)buf;
    *c = mock_input_buffer[mock_input_pos++];

    return 1;
}

private int mock_write_fn(int fd, const void *buf, size_t count)
{
    (void)fd;

    if (!mock_output_buffer)
    {
        mock_output_cap = 4096;
        mock_output_buffer = malloc(mock_output_cap);

        if (!mock_output_buffer)
            return -1;

        mock_output_buffer[0] = '\0';
    }

    if (mock_output_pos + count + 1 > mock_output_cap)
    {
        size_t new_cap = mock_output_cap * 2;
        char *new_buf = realloc(mock_output_buffer, new_cap);

        if (!new_buf)
            return -1;

        mock_output_buffer = new_buf;
        mock_output_cap = new_cap;
    }

    memcpy(mock_output_buffer + mock_output_pos, buf, count);
    mock_output_pos += count;
    mock_output_buffer[mock_output_pos] = '\0';

    return count;
}

private void setup_input(const char *input)
{
    mock_input_buffer = strdup(input);
    mock_input_len = strlen(input);
    mock_input_pos = 0;
}

private void cleanup_input(void)
{
    free(mock_input_buffer);
    mock_input_buffer = NULL;
    free(mock_output_buffer);
    mock_output_buffer = NULL;
    mock_output_cap = 0;
}

private void test_simple_input(void **state)
{
    (void)state;

    setup_input("hello\n");
    reset_mock_buffers();

    history_init();

    char *result = moss_input_readline_with_io("> ", mock_read_fn, mock_write_fn);

    assert_non_null(result);
    assert_string_equal(result, "hello");

    free(result);
    cleanup_input();
    history_destroy();
}

private void test_empty_input(void **state)
{
    (void)state;

    setup_input("\n");
    reset_mock_buffers();

    history_init();

    char *result = moss_input_readline_with_io("> ", mock_read_fn, mock_write_fn);

    assert_non_null(result);
    assert_string_equal(result, "");

    free(result);
    cleanup_input();
    history_destroy();
}

private void test_backspace_at_start(void **state)
{
    (void)state;

    setup_input("abc\x1b[D\x1b[D\x1b[D\x7f\n");
    reset_mock_buffers();

    history_init();

    char *result = moss_input_readline_with_io("> ", mock_read_fn, mock_write_fn);

    assert_non_null(result);
    assert_string_equal(result, "ab");

    free(result);
    cleanup_input();
    history_destroy();
}

private void test_backspace_at_end(void **state)
{
    (void)state;

    setup_input("hello\x7f\n");
    reset_mock_buffers();

    history_init();

    char *result = moss_input_readline_with_io("> ", mock_read_fn, mock_write_fn);

    assert_non_null(result);
    assert_string_equal(result, "hell");

    free(result);
    cleanup_input();
    history_destroy();
}

private void test_multiple_backspaces(void **state)
{
    (void)state;

    setup_input("hello\x7f\x7f\x7f\n");
    reset_mock_buffers();

    history_init();

    char *result = moss_input_readline_with_io("> ", mock_read_fn, mock_write_fn);

    assert_non_null(result);
    assert_string_equal(result, "he");

    free(result);
    cleanup_input();
    history_destroy();
}

private void test_ctrl_c_clears_line(void **state)
{
    (void)state;

    setup_input("hello\x03world\n");
    reset_mock_buffers();

    history_init();

    char *result = moss_input_readline_with_io("> ", mock_read_fn, mock_write_fn);

    assert_non_null(result);
    assert_string_equal(result, "world");

    free(result);
    cleanup_input();
    history_destroy();
}

private void test_ctrl_d_on_empty_returns_null(void **state)
{
    (void)state;

    setup_input("\x04");
    reset_mock_buffers();

    history_init();

    char *result = moss_input_readline_with_io("> ", mock_read_fn, mock_write_fn);

    assert_null(result);

    cleanup_input();
    history_destroy();
}

private void test_ctrl_d_with_content_ignores(void **state)
{
    (void)state;

    setup_input("hello\x04\n");
    reset_mock_buffers();

    history_init();

    char *result = moss_input_readline_with_io("> ", mock_read_fn, mock_write_fn);

    assert_non_null(result);
    assert_string_equal(result, "hello");

    free(result);
    cleanup_input();
    history_destroy();
}

private void test_arrow_up_navigates_history(void **state)
{
    (void)state;

    setup_input("\x1b[A\n");
    reset_mock_buffers();

    history_init();
    history_add("first_cmd");
    history_add("second_cmd");
    history_add("third_cmd");

    char *result = moss_input_readline_with_io("> ", mock_read_fn, mock_write_fn);

    assert_non_null(result);
    assert_string_equal(result, "third_cmd");

    free(result);
    cleanup_input();
    history_destroy();
}

private void test_arrow_down_navigates_history(void **state)
{
    (void)state;

    setup_input("\x1b[A\x1b[B\n");
    reset_mock_buffers();

    history_init();
    history_add("first_cmd");
    history_add("second_cmd");

    char *result = moss_input_readline_with_io("> ", mock_read_fn, mock_write_fn);

    assert_non_null(result);
    assert_string_equal(result, "second_cmd");

    free(result);
    cleanup_input();
    history_destroy();
}

private void test_arrow_up_then_down(void **state)
{
    (void)state;

    setup_input("\x1b[A\x1b[B\n");
    reset_mock_buffers();

    history_init();
    history_add("cmd1");
    history_add("cmd2");
    history_add("cmd3");

    char *result = moss_input_readline_with_io("> ", mock_read_fn, mock_write_fn);

    assert_non_null(result);
    assert_string_equal(result, "cmd3");

    free(result);
    cleanup_input();
    history_destroy();
}

private void test_arrow_up_at_oldest_stays(void **state)
{
    (void)state;

    setup_input("\x1b[A\x1b[A\x1b[A\x1b[A\n");
    reset_mock_buffers();

    history_init();
    history_add("cmd1");
    history_add("cmd2");
    history_add("cmd3");

    char *result = moss_input_readline_with_io("> ", mock_read_fn, mock_write_fn);

    assert_non_null(result);
    assert_string_equal(result, "cmd1");

    free(result);
    cleanup_input();
    history_destroy();
}

private void test_arrow_down_at_newest_returns_empty(void **state)
{
    (void)state;

    setup_input("\x1b[B\n");
    reset_mock_buffers();

    history_init();
    history_add("cmd1");
    history_add("cmd2");

    char *result = moss_input_readline_with_io("> ", mock_read_fn, mock_write_fn);

    assert_non_null(result);
    assert_string_equal(result, "");

    free(result);
    cleanup_input();
    history_destroy();
}

private void test_arrow_left_moves_cursor(void **state)
{
    (void)state;

    setup_input("abcd\x1b[D\n");
    reset_mock_buffers();

    history_init();

    char *result = moss_input_readline_with_io("> ", mock_read_fn, mock_write_fn);

    assert_non_null(result);
    assert_string_equal(result, "abcd");

    free(result);
    cleanup_input();
    history_destroy();
}

private void test_arrow_right_moves_cursor(void **state)
{
    (void)state;

    setup_input("ab\x1b[Ccd\n");
    reset_mock_buffers();

    history_init();

    char *result = moss_input_readline_with_io("> ", mock_read_fn, mock_write_fn);

    assert_non_null(result);
    assert_string_equal(result, "abcd");

    free(result);
    cleanup_input();
    history_destroy();
}

private void test_input_added_to_history_on_enter(void **state)
{
    (void)state;

    setup_input("test_cmd\n");
    reset_mock_buffers();

    history_init();

    char *result = moss_input_readline_with_io("> ", mock_read_fn, mock_write_fn);

    assert_non_null(result);
    assert_int_equal(history_count(), 1);
    assert_string_equal(history_get(0), "test_cmd");

    free(result);
    cleanup_input();
    history_destroy();
}

private void test_empty_input_not_added_to_history(void **state)
{
    (void)state;

    setup_input("\n");
    reset_mock_buffers();

    history_init();
    history_add("previous_cmd");

    char *result = moss_input_readline_with_io("> ", mock_read_fn, mock_write_fn);

    assert_non_null(result);
    assert_int_equal(history_count(), 1);

    free(result);
    cleanup_input();
    history_destroy();
}

private void test_long_input_buffer_growth(void **state)
{
    (void)state;

    char long_input[600];
    memset(long_input, 'a', 500);
    long_input[500] = '\n';
    long_input[501] = '\0';

    setup_input(long_input);
    reset_mock_buffers();

    history_init();

    char *result = moss_input_readline_with_io("> ", mock_read_fn, mock_write_fn);

    assert_non_null(result);
    assert_int_equal(strlen(result), 500);

    free(result);
    cleanup_input();
    history_destroy();
}

private void test_carriage_return_as_newline(void **state)
{
    (void)state;

    setup_input("test\r");
    reset_mock_buffers();

    history_init();

    char *result = moss_input_readline_with_io("> ", mock_read_fn, mock_write_fn);

    assert_non_null(result);
    assert_string_equal(result, "test");

    free(result);
    cleanup_input();
    history_destroy();
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_simple_input),
        cmocka_unit_test(test_empty_input),
        cmocka_unit_test(test_backspace_at_start),
        cmocka_unit_test(test_backspace_at_end),
        cmocka_unit_test(test_multiple_backspaces),
        cmocka_unit_test(test_ctrl_c_clears_line),
        cmocka_unit_test(test_ctrl_d_on_empty_returns_null),
        cmocka_unit_test(test_ctrl_d_with_content_ignores),
        cmocka_unit_test(test_arrow_up_navigates_history),
        cmocka_unit_test(test_arrow_down_navigates_history),
        cmocka_unit_test(test_arrow_up_then_down),
        cmocka_unit_test(test_arrow_up_at_oldest_stays),
        cmocka_unit_test(test_arrow_down_at_newest_returns_empty),
        cmocka_unit_test(test_arrow_left_moves_cursor),
        cmocka_unit_test(test_arrow_right_moves_cursor),
        cmocka_unit_test(test_input_added_to_history_on_enter),
        cmocka_unit_test(test_empty_input_not_added_to_history),
        cmocka_unit_test(test_long_input_buffer_growth),
        cmocka_unit_test(test_carriage_return_as_newline),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
