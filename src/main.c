#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#include "include/main.h"
#include "include/builtin.h"
#include "include/signals.h"
#include "include/logging.h"
#include "include/retry.h"

private bool isSafe(const char *);
private int doFork(void *);

int main(int argc, char **argv)
{
    mossLogSetLevel(LOG_LEVEL_DEBUG);
    moss_init_signals();

    FILE *logFile = fopen("moss.log", "a");

    if (!logFile)
    {
        fprintf(stderr, "Failed to open log file\n");
        return EXIT_FAILURE;
    }

    mossLogSetFile(logFile);
    LOG_INFO("MOSS Shell starting");
    moss_loop();
    LOG_INFO("MOSS Shell shutting down");
    fclose(logFile);

    return EXIT_SUCCESS;
}

/**
 * we r gonna be reading, parsing, and executing
 * read: read the command from stdin
 * parsing: spearate the command string into program arguments
 * execute: run the parsed command
 */
void moss_loop()
{
    char *line;
    char **args;
    int status;

    do
    {
        printf("> ");
        line = moss_read_line();
        args = moss_split_line(line);
        status = moss_execute(args);

        free(line);
        free(args);
    } while (status);
}

// starting with a block then dynamically allocating more space if needed
char *moss_read_line()
{
    char *line = NULL;
    size_t bufSize = 0;

    if (getline(&line, &bufSize, stdin) == -1)
    {
        if (feof(stdin))
        {
            exit(EXIT_SUCCESS);
        }
        else
        {
            SAFE_ERROR(ERR_CATEGORY_INPUT, "failed to read input");
            exit(EXIT_FAILURE);
        }
    }

    return line;
}

// tokenizing the commands by white spaces as delimeters
char **moss_split_line(char *line)
{
    if (!line)
    {
        fprintf(stderr, "MOSS: split_line: NULL input\n");
        return NULL;
    }

    int bufferSize = TOK_INITIAL_BUFFER, position = 0;
    char **tokens = (char **)malloc(bufferSize * sizeof(char *));

    if (!tokens)
    {
        SAFE_ERROR(ERR_CATEGORY_MEMORY, "allocation failed");
        exit(EXIT_FAILURE);
    }

    char *savePtr = NULL;
    char *token = strtok_r(line, TOK_DELIMETER, &savePtr);

    while (token != NULL)
    {
        const size_t tlen = strlen(token);

        if (tlen == 0)
        {
            token = strtok_r(NULL, TOK_DELIMETER, &savePtr);
            continue;
        }

        if (tlen > (size_t)TOK_MAX_TOKEN_LEN)
        {
            SAFE_ERROR(ERR_CATEGORY_PARSE, "token too long");
            free(tokens);
            return NULL;
        }

        if (!isSafe(token))
        {
            SAFE_ERROR(ERR_CATEGORY_PARSE, "invalid characters in token");
            free(tokens);
            return NULL;
        }

        if (position >= bufferSize - 1)
        {
            int newSize = bufferSize * 2;
            char **newTokens = realloc(tokens, (size_t)newSize * sizeof(char *));

            if (!newTokens)
            {
                SAFE_ERROR(ERR_CATEGORY_MEMORY, "allocation failed");
                free(tokens);
                exit(EXIT_FAILURE);
            }

            tokens = newTokens;
            bufferSize = newSize;
        }

        tokens[position++] = token;
        token = strtok_r(NULL, TOK_DELIMETER, &savePtr);
    }

    if (position >= bufferSize)
    {
        char **newTokens = realloc(tokens, (size_t)(bufferSize + 1) * sizeof(char *));

        if (!newTokens)
        {
            SAFE_ERROR(ERR_CATEGORY_MEMORY, "allocation failed");
            free(tokens);
            exit(EXIT_FAILURE);
        }

        tokens = newTokens;
        bufferSize++;
    }

    tokens[position] = NULL;
    return tokens;
}

private int doFork(void *ctx)
{
    LaunchContext *launch = (LaunchContext *)ctx;
    launch->pid = fork();

    return (launch->pid < 0) ? -1 : 0;
}

int moss_launch(char **args)
{
    if (!args || !args[0])
    {
        SAFE_ERROR(ERR_CATEGORY_EXEC, "invalid arguments");
        return 0;
    }

    LaunchContext ctx = {.args = args, .pid = -1};

    RetryConfig retryConfig = {
        .maxRetries = 3,
        .baseDelayms = 100,
        .maxDelayms = 1000,
        .useExponentialBackoff = true};

    RetryContext retryCtx;
    mossRetryInit(&retryCtx, &retryConfig);
    RetryResult result = mossRetryExecute(&retryCtx, doFork, &ctx, mossRetryShouldRetry);

    if (result != RETRY_OK)
    {
        SAFE_ERROR(ERR_CATEGORY_EXEC, "failed to fork process after %d attempts", retryCtx.attemptCount);
        return 0;
    }

    pid_t pid = ctx.pid;
    int status = 0;

    if (pid == 0)
    {
        if (setpgid(0, 0) == -1)
            _exit(126);

        if (execvp(args[0], args) == -1)
            _exit(126);
    }

    if (setpgid(pid, pid) == -1)
        if (errno != EACCES && errno != EINVAL && errno != ESRCH)
            SAFE_ERROR(ERR_CATEGORY_SYSTEM, "failed to set process group");

    moss_foreground_pgid = pid;

    while (true)
    {
        pid_t w = waitpid(pid, &status, WUNTRACED);

        if (w == -1)
        {
            if (errno == EINTR)
                continue;

            SAFE_ERROR(ERR_CATEGORY_SYSTEM, "error waiting for process");
            break;
        }

        if (WIFEXITED(status))
            break;

        if (WIFSIGNALED(status))
            break;

        if (WIFSTOPPED(status))
            break;
    }

    moss_foreground_pgid = 0;

    return 1;
}

int moss_execute(char **args)
{
    if (!args[0])
        return 1;

    for (size_t i = 0; i < NUM_BUILTINS; i++)
        if (strcmp(args[0], builtins[i].name) == 0)
            return (*builtins[i].func)(args);

    SAFE_ERROR(ERR_CATEGORY_EXEC, "command not found: %s", args[0]);
    return 1;
}

private bool isSafe(const char *token)
{
    if (!token || *token == '\0')
        return 0;

    return strcspn(token, DANGEROUS_CHARS) == strlen(token);
}