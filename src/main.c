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
#include "src/modules/builtins/builtin.h"
#include "include/signals.h"
#include "include/logging.h"
#include "include/retry.h"
#include "include/parser.h"
#include "include/pipeline.h"
#include "include/history.h"
#include "include/input.h"

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
    history_init();
    history_load(DEFAULT_HISTORY_FILE);
    moss_input_init();

    moss_loop();

    moss_input_restore();
    history_save(DEFAULT_HISTORY_FILE);
    history_destroy();
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
        line = moss_input_readline("> ");

        if (!line)
            break;

        strip_comment(line);

        if (line == NULL || line[0] == '\0')
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

        moss_input_restore();
        status = moss_execute(args);
        moss_input_init();

        size_t len = strlen(line);

        if (len > 0 && line[len - 1] == '\n')
            line[len - 1] = '\0';

        if (line[0] != '\0')
            history_add(line);

        free(line);
        free(args);
    } while (status);
}