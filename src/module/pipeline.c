#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>

#include "include/main.h"
#include "include/builtin.h"
#include "include/logging.h"
#include "include/retry.h"
#include "include/signals.h"

private int doFork(void *ctx)
{
    LaunchContext *launch = (LaunchContext *)ctx;
    launch->pid = fork();

    return (launch->pid < 0) ? -1 : 0;
}

private int doPipelineFork(void *ctx)
{
    pid_t *pid = (pid_t *)ctx;
    *pid = fork();

    return (*pid < 0) ? -1 : 0;
}

int moss_execute_pipeline(char **args)
{
    if (strcmp(args[0], "|") == 0)
    {
        SAFE_ERROR(ERR_CATEGORY_PARSE, "Syntax error near unexpected token '|'");
        return 1;
    }

    int argc = 0;

    while (args[argc])
        argc++;

    if (strcmp(args[argc - 1], "|") == 0)
    {
        SAFE_ERROR(ERR_CATEGORY_PARSE, "Syntax error near unexpected token '|'");
        return 1;
    }

    int numCommands = 1;

    for (size_t i = 0; args[i]; i++)
        if (strcmp(args[i], "|") == 0)
            numCommands++;

    char ***commands = (char ***)malloc((numCommands + 1) * sizeof(char **));

    if (!commands)
    {
        SAFE_ERROR(ERR_CATEGORY_MEMORY, "Memory allocation failed");
        return 0;
    }

    int cmdIdx = 0, argIdx = 0;
    commands[cmdIdx] = malloc(64 * sizeof(char *));

    for (size_t i = 0; args[i]; i++)
    {
        if (strcmp(args[i], "|") == 0)
        {
            commands[cmdIdx][argIdx] = NULL;
            cmdIdx++;
            argIdx = 0;
            commands[cmdIdx] = malloc(64 * sizeof(char *));
        }
        else
        {
            commands[cmdIdx][argIdx++] = args[i];
        }
    }

    commands[cmdIdx][argIdx] = NULL;
    signal(SIGPIPE, SIG_IGN);

    int *pipes = (int *)malloc((numCommands - 1) * 2 * sizeof(int));

    if (!pipes)
    {
        SAFE_ERROR(ERR_CATEGORY_MEMORY, "Memory Allocation failed");
        return 0;
    }

    for (size_t i = 0; i < numCommands - 1; i++)
    {
        if (pipe(&pipes[i * 2]) == -1)
        {
            SAFE_ERROR(ERR_CATEGORY_EXEC, "Pipe failed");
            free(pipes);

            for (size_t j = 0; j <= cmdIdx; j++)
                free(commands[j]);

            free(commands);

            return 1;
        }
    }

    pid_t *pids = (pid_t *)malloc(numCommands * sizeof(pid_t));

    for (size_t i = 0; i < numCommands; i++)
    {
        pid_t pid = -1;

        RetryConfig config = {
            .maxRetries = 3,
            .baseDelayms = 100,
            .maxDelayms = 1000,
            .useExponentialBackoff = true,
        };

        RetryContext ctx;
        mossRetryInit(&ctx, &config);
        RetryResult result = mossRetryExecute(&ctx, doPipelineFork, &pid, mossRetryShouldRetry);

        if (result != RETRY_OK)
        {
            SAFE_ERROR(ERR_CATEGORY_EXEC, "Fork failed after %d attempts", ctx.attemptCount);
            pids[i] = -1;
            continue;
        }

        if (pid == 0)
        {
            // child process
            if (i > 0)
                dup2(pipes[(i - 1) * 2], STDIN_FILENO);

            if (i < numCommands - 1)
                dup2(pipes[(i * 2 + 1)], STDOUT_FILENO);

            for (size_t j = 0; j < (numCommands - 1) * 2; j++)
                close(pipes[j]);

            for (size_t k = 0; k < NUM_BUILTINS; k++)
            {
                if (strcmp(commands[i][0], builtins[k].name) == 0)
                {
                    int result = (*builtins[k].func)(commands[i]);
                    fflush(stdout);
                    _exit(result);
                }
            }

            execvp(commands[i][0], commands[i]);
            SAFE_ERROR(ERR_CATEGORY_EXEC, "Command not found: %s", commands[i][0]);
            _exit(127);
        }

        pids[i] = pid;
    }

    for (size_t i = 0; i < (numCommands - 1) * 2; i++)
        close(pipes[i]);

    int status;

    for (size_t i = 0; i < numCommands; i++)
        if (pids[i] > 0)
            waitpid(pids[i], &status, 0);

    free(pipes);
    free(pids);

    for (size_t i = 0; i <= cmdIdx; i++)
        free(commands[i]);

    free(commands);

    return 1;
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
        .useExponentialBackoff = true,
    };

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

    int pipe = 0;

    for (size_t i = 0; args[i]; i++)
        if (strcmp(args[i], "|") == 0)
            pipe++;

    if (pipe > 0)
        return moss_execute_pipeline(args);

    for (size_t i = 0; i < NUM_BUILTINS; i++)
        if (strcmp(args[0], builtins[i].name) == 0)
            return (*builtins[i].func)(args);

    return moss_launch(args);
}