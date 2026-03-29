#pragma once

#define private static
#define INPUT_BUFFER_SIZE 256

enum KeyArrow
{
    UP = 'A',
    DOWN = 'B',
    LEFT = 'C',
    RIGHT = 'D',
};

enum SpecialKey
{
    BACKSPACE = 127,
    BACKSPACE_ALT = 8,
    CTRL_C = 3,
    CTRL_D = 4,
    CTRL_Z = 26,
    ESCAPE = 27,
    CARRIAGE_RETURN = '\r',
    NEWLINE = '\n',
};

typedef ssize_t (*moss_read_fn_t)(int, void *, size_t);
typedef int (*moss_write_fn_t)(int, const void *, size_t);

void moss_input_init();
void moss_input_restore();
char *moss_input_readline(const char *);
char *moss_input_readline_with_io(const char *, moss_read_fn_t, moss_write_fn_t);