#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "include/main.h"
#include "include/builtin.h"

int main(int argc, char **argv)
{
    // load config files later

    // MOSS is just a name of the shell
    moss_loop();
    // printf("hello world");

    // do shutdown/cleanup

    return EXIT_SUCCESS;
}

/**
 * we r gonna be reading, parsing, and executing
 * read: read the command from stdin
 * parsing: spearate the command string into program arguments
 * execute: run the parsed command
 */
void moss_loop(void)
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
char *moss_read_line(void)
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
            perror("readline");
            exit(EXIT_FAILURE);
        }
    }

    return line;
}

// tokenizing the commands by white spaces as delimeters
char **moss_split_line(char *line)
{
    int bufferSize = TOK_BUFFERSIZE, position = 0;
    char **tokens = (char **)malloc(bufferSize * sizeof(char *));
    char *token;

    if (!tokens)
        goto allocationFailed;

    token = strtok(line, TOK_DELIMETER);

    while (token != NULL)
    {
        tokens[position] = token;
        position++;

        if (position >= bufferSize)
        {
            bufferSize *= 2;
            char **newTokens = realloc(tokens, bufferSize * sizeof(char *));

            if (!newTokens)
                goto allocationFailed;

            tokens = newTokens;
        }

        token = strtok(NULL, TOK_DELIMETER);
    }

    tokens[position] = NULL;
    return tokens;

allocationFailed:
    fprintf(stderr, "MOSS: allocation failed\n");
    free(tokens);
    exit(EXIT_FAILURE);
}

int moss_launch(char **args)
{
    pid_t pid;
    int status;
    pid = fork();

    if (pid == 0)
    {
        if (execvp(args[0], args) == -1)
            perror("MOSS");

        exit(EXIT_FAILURE);
    }
    else if (pid < 0)
    {
        perror("MOSS");
    }
    else
    {
        do
        {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

int moss_execute(char **args)
{
    if (!args[0])
        return 1;

    for (size_t i = 0; i < NUM_BUILTINS; i++)
        if (strcmp(args[0], builtins[i].name) == 0)
            return (*builtins[i].func)(args);

    return moss_launch(args);
}