#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "include/main.h"

int main(int argc, char **argv)
{
    // load config files later

    lsh_loop();
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
void lsh_loop(void)
{
    char *line;
    char **args;
    int status;

    do
    {
        printf("> ");
        line = lsh_read_line();
        args = lsh_split_line(line);
        status = lsh_execute(args);

        free(line);
        free(args);
    } while (status);
}

// starting with a block then dynamically allocating more space if needed
char *lsh_read_line(void)
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
char **lsh_split_line(char *line)
{
    int bufferSize = LSH_TOK_BUFFERSIZE, position = 0;
    char **tokens = (char **)malloc(bufferSize * sizeof(char *));
    char *token;

    if (!tokens)
        goto allocationFailed;

    token = strtok(line, LSH_TOK_DELIMETER);

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

        token = strtok(NULL, LSH_TOK_DELIMETER);
    }

    tokens[position] = NULL;
    return tokens;

allocationFailed:
    fprintf(stderr, "lsh: allocation failed\n");
    free(tokens);
    exit(EXIT_FAILURE);
}

int lsh_launch(char **args)
{
    pid_t pid;
    int status;
    pid = fork();

    if (pid == 0)
    {
        if (execvp(args[0], args) == -1)
            perror("lsh");

        exit(EXIT_FAILURE);
    }
    else if (pid < 0)
    {
        perror("lsh");
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

int lsh_execute(char **args)
{
    return 0;
}