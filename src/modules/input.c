#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>

#include "include/input.h"
#include "include/logging.h"
#include "include/history.h"

private struct termios orig_termios;
private bool raw_mode_enabled = false;
private bool is_first_prompt = true;

void moss_input_init()
{
    if (!isatty(STDIN_FILENO))
    {
        LOG_WARN("Not a TTY, skipping raw mode");
        return;
    }

    tcgetattr(STDIN_FILENO, &orig_termios);

    struct termios raw = orig_termios;

    raw.c_lflag &= ~(ICANON | ECHO | ECHOCTL | ECHOE | ECHOK | ECHOKE | ICRNL | INLCR | INPCK | ISTRIP);
    raw.c_oflag &= ~(OPOST);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    raw_mode_enabled = true;
}

void moss_input_restore()
{
    if (raw_mode_enabled)
    {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
        raw_mode_enabled = false;
    }
}

private void input_redraw_line(const char *prompt, char *buffer, size_t cursor, size_t len, size_t bufferSize)
{
    char output[512];
    int pos = 0;

    pos += snprintf(output + pos, sizeof(output) - pos, "\r\033[K%s%s", prompt, buffer);

    if (cursor < len)
        pos += snprintf(output + pos, sizeof(output) - pos, "\033[%zuD", len - cursor);

    write(STDOUT_FILENO, output, pos);
}

private void input_handle_backspace(char *buffer, size_t *cursor, size_t *len, const char *prompt, size_t bufferSize)
{
    if (*len > 0 && *cursor > 0)
    {
        for (size_t i = *cursor - 1; i < *len - 1; i++)
            buffer[i] = buffer[i + 1];

        (*len)--;
        (*cursor)--;
        buffer[*len] = '\0';

        input_redraw_line(prompt, buffer, *cursor, *len, bufferSize);
    }
}

private void input_handle_ctrl_c(char *buffer, size_t *cursor, size_t *len, int *historyView, char **historyBackup, const char *prompt)
{
    write(STDOUT_FILENO, "^C\n", 3);
    buffer[0] = '\0';
    *len = 0;
    *cursor = 0;
    *historyView = -1;
    history_reset_cursor();
    free(*historyBackup);
    *historyBackup = NULL;
    write(STDOUT_FILENO, prompt, strlen(prompt));
}

private int input_handle_ctrl_d(size_t len, char *buffer, char *historyBackup)
{
    if (len == 0)
    {
        write(STDOUT_FILENO, "exit\n", 5);
        free(buffer);
        free(historyBackup);
        return 1;
    }

    return 0;
}

private void input_handle_arrow_up(char *buffer, int *historyView, char **historyBackup, size_t *cursor, size_t *len, size_t bufferSize)
{
    int count = history_count();

    if (*historyView == -1 || *historyView == count)
    {
        *historyBackup = strndup(buffer, *len);
        *historyView = history_prev();
    }
    else
    {
        *historyView = history_prev();
    }

    if (*historyView >= 0)
    {
        const char *cmd = history_get(*historyView);

        if (cmd)
        {
            strncpy(buffer, cmd, bufferSize - 1);
            buffer[bufferSize - 1] = '\0';
            *len = strlen(buffer);
            *cursor = *len;
        }
    }
}

private void input_handle_arrow_down(char *buffer, int *historyView, char **historyBackup, size_t *cursor, size_t *len, size_t bufferSize)
{
    if (*historyView >= 0 || *historyView == history_count())
    {
        *historyView = history_next();

        if (*historyView >= 0 && *historyView < history_count())
        {
            const char *cmd = history_get(*historyView);

            if (cmd)
            {
                strncpy(buffer, cmd, bufferSize - 1);
                buffer[bufferSize - 1] = '\0';
                *len = strlen(buffer);
                *cursor = *len;
            }
        }
        else
        {
            if (*historyBackup)
            {
                strncpy(buffer, *historyBackup, bufferSize - 1);
                buffer[bufferSize - 1] = '\0';
                *len = strlen(buffer);
                *cursor = *len;
                free(*historyBackup);
                *historyBackup = NULL;
            }
            else
            {
                buffer[0] = '\0';
                *len = 0;
                *cursor = 0;
            }

            *historyView = -1;
        }
    }
}

private void input_handle_arrow_left(size_t *cursor, size_t len)
{
    if (*cursor > 0)
        (*cursor)--;
}

private void input_handle_arrow_right(size_t *cursor, size_t len)
{
    if (*cursor < len)
        (*cursor)++;
}

char *moss_input_readline(const char *prompt)
{
    if (!raw_mode_enabled)
    {
        char *line = NULL;
        size_t bufSize = 0;
        printf("\r%s", prompt);
        fflush(stdout);

        if (getline(&line, &bufSize, stdin) == -1)
        {
            free(line);
            return NULL;
        }

        size_t len = strlen(line);

        if (len > 0 && line[len - 1] == '\n')
            line[len - 1] = '\0';

        return line;
    }

    char *buffer = (char *)malloc(INPUT_BUFFER_SIZE);

    if (!buffer)
    {
        SAFE_ERROR(ERR_CATEGORY_MEMORY, "Memory allocation failed");
        return NULL;
    }

    size_t bufferSize = INPUT_BUFFER_SIZE;
    size_t cursor = 0;
    size_t len = 0;

    int historyView = -1;
    char *historyBackup = NULL;
    int cursorMoved = 0;
    int currentHistoryCount = history_count();

    if (currentHistoryCount > 0)
        historyView = history_count();

    if (!is_first_prompt)
        write(STDOUT_FILENO, "\r\n", 2);

    is_first_prompt = false;
    write(STDOUT_FILENO, prompt, strlen(prompt));

    while (true)
    {
        char c;

        if (read(STDIN_FILENO, &c, 1) <= 0)
        {
            free(buffer);
            free(historyBackup);
            return NULL;
        }

        if (c == NEWLINE || c == CARRIAGE_RETURN)
        {
            write(STDOUT_FILENO, "\r\n", 2);
            buffer[len] = '\0';

            if (len > 0)
                history_add(buffer);

            free(historyBackup);
            return buffer;
        }

        if (c == BACKSPACE || c == BACKSPACE_ALT)
        {
            input_handle_backspace(buffer, &cursor, &len, prompt, bufferSize);
            continue;
        }

        if (c == CTRL_C)
        {
            input_handle_ctrl_c(buffer, &cursor, &len, &historyView, &historyBackup, prompt);
            continue;
        }

        if (c == CTRL_D)
        {
            if (input_handle_ctrl_d(len, buffer, historyBackup))
                return NULL;

            continue;
        }

        if (c == ESCAPE)
        {
            char seq[3];
            read(STDIN_FILENO, &seq[0], 1);
            read(STDIN_FILENO, &seq[1], 1);

            if (seq[0] == '[')
            {
                switch (seq[1])
                {
                case UP:
                    input_handle_arrow_up(buffer, &historyView, &historyBackup, &cursor, &len, bufferSize);
                    break;

                case DOWN:
                    input_handle_arrow_down(buffer, &historyView, &historyBackup, &cursor, &len, bufferSize);
                    break;

                case LEFT:
                    input_handle_arrow_left(&cursor, len);
                    cursorMoved = 1;
                    break;

                case RIGHT:
                    input_handle_arrow_right(&cursor, len);
                    cursorMoved = 1;
                    break;
                }
            }

            input_redraw_line(prompt, buffer, cursor, len, bufferSize);
            continue;
        }

        if (c >= 32 && c < 127)
        {
            if (len + 1 >= bufferSize)
            {
                bufferSize *= 2;
                char *newBuffer = realloc(buffer, bufferSize);

                if (!newBuffer)
                {
                    free(buffer);
                    free(historyBackup);
                    SAFE_ERROR(ERR_CATEGORY_MEMORY, "Memory realloc failed");
                    return NULL;
                }

                buffer = newBuffer;
            }

            if (cursorMoved)
            {
                cursor = len;
                cursorMoved = 0;
            }

            for (size_t i = len; i > cursor; i--)
                buffer[i] = buffer[i - 1];

            buffer[cursor] = c;
            len++;
            cursor++;
            buffer[len] = '\0';

            putchar(c);
            fflush(stdout);
        }
    }

    return NULL;
}

// ========================================= MOCK I/O Types for Unit testing =========================================

private void input_redraw_line_with_io(const char *prompt, char *buffer, size_t cursor, size_t len, size_t bufferSize, moss_write_fn_t write_fn)
{
    char output[512];
    int pos = 0;

    pos += snprintf(output + pos, sizeof(output) - pos, "\r%s%s", prompt, buffer);

    for (size_t i = len; i < bufferSize - 1 && pos < (int)sizeof(output) - 1; i++)
        output[pos++] = ' ';

    pos += snprintf(output + pos, sizeof(output) - pos, "\r%s%s", prompt, buffer);

    for (size_t i = 0; i < cursor && pos < (int)sizeof(output) - 1; i++)
        output[pos++] = '\b';

    output[pos] = '\0';
    write_fn(STDOUT_FILENO, output, pos);
}

private void input_handle_backspace_with_io(char *buffer, size_t *cursor, size_t *len, const char *prompt, size_t bufferSize, moss_write_fn_t write_fn)
{
    if (*len > 0 && *cursor > 0)
    {
        for (size_t i = *cursor - 1; i < *len - 1; i++)
            buffer[i] = buffer[i + 1];

        (*len)--;
        (*cursor)--;
        buffer[*len] = '\0';

        input_redraw_line_with_io(prompt, buffer, *cursor, *len, bufferSize, write_fn);
    }
}

private void input_handle_ctrl_c_with_io(char *buffer, size_t *cursor, size_t *len, int *historyView, char **historyBackup, const char *prompt, moss_write_fn_t write_fn)
{
    char output[32];
    int pos = 0;
    pos += snprintf(output + pos, sizeof(output) - pos, "^C\n");
    pos += snprintf(output + pos, sizeof(output) - pos, "%s", prompt);
    output[pos] = '\0';
    write_fn(STDOUT_FILENO, output, pos);

    buffer[0] = '\0';
    *len = 0;
    *cursor = 0;
    *historyView = -1;
    history_reset_cursor();
    free(*historyBackup);
    *historyBackup = NULL;
}

private int input_handle_ctrl_d_with_io(size_t len, char *buffer, char *historyBackup, moss_write_fn_t write_fn)
{
    if (len == 0)
    {
        char output[32];
        snprintf(output, sizeof(output), "exit\n");
        write_fn(STDOUT_FILENO, output, strlen(output));
        free(buffer);
        free(historyBackup);
        return 1;
    }

    return 0;
}

private void input_handle_arrow_up_with_io(char *buffer, int *historyView, char **historyBackup, size_t *cursor, size_t *len, size_t bufferSize)
{
    int count = history_count();

    if (*historyView == -1 || *historyView == count)
    {
        *historyBackup = strndup(buffer, *len);
        *historyView = history_prev();
    }
    else
    {
        *historyView = history_prev();
    }

    if (*historyView >= 0)
    {
        const char *cmd = history_get(*historyView);

        if (cmd)
        {
            strncpy(buffer, cmd, bufferSize - 1);
            buffer[bufferSize - 1] = '\0';
            *len = strlen(buffer);
            *cursor = *len;
        }
    }
}

private void input_handle_arrow_down_with_io(char *buffer, int *historyView, char **historyBackup, size_t *cursor, size_t *len, size_t bufferSize)
{
    if (*historyView >= 0 || *historyView == history_count())
    {
        int prevView = *historyView;
        *historyView = history_next();

        if (*historyView >= 0 && *historyView < history_count())
        {
            const char *cmd = history_get(*historyView);

            if (cmd)
            {
                strncpy(buffer, cmd, bufferSize - 1);
                buffer[bufferSize - 1] = '\0';
                *len = strlen(buffer);
                *cursor = *len;
            }
        }
        else if (prevView >= 0 && prevView < history_count())
        {
            const char *cmd = history_get(prevView);

            if (cmd)
            {
                strncpy(buffer, cmd, bufferSize - 1);
                buffer[bufferSize - 1] = '\0';
                *len = strlen(buffer);
                *cursor = *len;
            }

            *historyView = prevView;
        }
        else
        {
            if (*historyBackup)
            {
                strncpy(buffer, *historyBackup, bufferSize - 1);
                buffer[bufferSize - 1] = '\0';
                *len = strlen(buffer);
                *cursor = *len;
                free(*historyBackup);
                *historyBackup = NULL;
            }
            else
            {
                buffer[0] = '\0';
                *len = 0;
                *cursor = 0;
            }

            *historyView = -1;
        }
    }
}

char *moss_input_readline_with_io(const char *prompt, moss_read_fn_t read_fn, moss_write_fn_t write_fn)
{
    char *buffer = (char *)malloc(INPUT_BUFFER_SIZE);

    if (!buffer)
    {
        SAFE_ERROR(ERR_CATEGORY_MEMORY, "Memory allocation failed");
        return NULL;
    }

    size_t bufferSize = INPUT_BUFFER_SIZE;
    size_t cursor = 0;
    size_t len = 0;

    int historyView = -1;
    char *historyBackup = NULL;
    int cursorMoved = 0;
    int currentHistoryCount = history_count();

    if (currentHistoryCount > 0)
        historyView = history_count();

    char output[64];
    snprintf(output, sizeof(output), "%s", prompt);
    write_fn(STDOUT_FILENO, output, strlen(output));

    while (true)
    {
        char c;

        if (read_fn(STDIN_FILENO, &c, 1) <= 0)
        {
            free(buffer);
            free(historyBackup);
            return NULL;
        }

        if (c == NEWLINE || c == CARRIAGE_RETURN)
        {
            snprintf(output, sizeof(output), "\n");
            write_fn(STDOUT_FILENO, output, strlen(output));
            buffer[len] = '\0';

            if (len > 0)
                history_add(buffer);

            free(historyBackup);
            return buffer;
        }

        if (c == BACKSPACE || c == BACKSPACE_ALT)
        {
            input_handle_backspace_with_io(buffer, &cursor, &len, prompt, bufferSize, write_fn);
            continue;
        }

        if (c == CTRL_C)
        {
            input_handle_ctrl_c_with_io(buffer, &cursor, &len, &historyView, &historyBackup, prompt, write_fn);
            continue;
        }

        if (c == CTRL_D)
        {
            if (input_handle_ctrl_d_with_io(len, buffer, historyBackup, write_fn))
                return NULL;
            continue;
        }

        if (c == ESCAPE)
        {
            char seq[3];
            read_fn(STDIN_FILENO, &seq[0], 1);
            read_fn(STDIN_FILENO, &seq[1], 1);

            if (seq[0] == '[')
            {
                switch (seq[1])
                {
                case UP:
                    input_handle_arrow_up_with_io(buffer, &historyView, &historyBackup, &cursor, &len, bufferSize);
                    break;

                case DOWN:
                    input_handle_arrow_down_with_io(buffer, &historyView, &historyBackup, &cursor, &len, bufferSize);
                    break;

                case LEFT:
                    if (cursor > 0)
                        cursor--;

                    cursorMoved = 1;
                    break;

                case RIGHT:
                    if (cursor < len)
                        cursor++;

                    cursorMoved = 1;
                    break;
                }
            }

            input_redraw_line_with_io(prompt, buffer, cursor, len, bufferSize, write_fn);
            continue;
        }

        if (c >= 32 && c < 127)
        {
            if (len + 1 >= bufferSize)
            {
                bufferSize *= 2;
                char *newBuffer = realloc(buffer, bufferSize);

                if (!newBuffer)
                {
                    free(buffer);
                    free(historyBackup);
                    SAFE_ERROR(ERR_CATEGORY_MEMORY, "Memory realloc failed");
                    return NULL;
                }

                buffer = newBuffer;
            }

            if (cursorMoved)
            {
                cursor = len;
                cursorMoved = 0;
            }

            for (size_t i = len; i > cursor; i--)
                buffer[i] = buffer[i - 1];

            buffer[cursor] = c;
            len++;
            cursor++;
            buffer[len] = '\0';

            write_fn(STDOUT_FILENO, &c, 1);
        }
    }

    return NULL;
}
