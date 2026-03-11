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
#include "include/parser.h"
#include "include/pipeline.h"

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

void moss_loop()
{
    char *line;
    char **args;
    int status;

    do
    {
        printf("> ");
        line = moss_read_line();
        strip_comment(line);

        if (line[0] == '\0')
        {
            free(line);
            continue;
        }

        args = moss_split_line(line);

        if (!args)
        {
            free(line);
            continue;
        }

        if (args)
        {
            for (size_t i = 0; args[i] != NULL; i++)
            {
                char *stripped = strip_quotes(args[i]);
                char *expanded = expandEnvVar(stripped);

                if (expanded)
                {
                    free(args[i]);
                    args[i] = expanded;
                }
            }
        }

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