#include <stdio.h>
#include <stdlib.h>

void lshLoop(void);
char *lsh_read_line(void);
char **lsh_split_line(char *);
int lsh_execute(char **);

int main(int argc, char **argv)
{
    // load config files later

    lshLoop();

    // do shutdown/cleanup

    return EXIT_SUCCESS;
}

/**
 * we r gonna be reading, parsing, and executing
 * read: read the command from stdin
 * parsing: spearate the command string into program arguments
 * execute: run the parsed command
 */
void lshLoop(void)
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
    ssize_t bufSize = 0;

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
}

int lsh_execute(char **args)
{
}