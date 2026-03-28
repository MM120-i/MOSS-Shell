#include <string.h>
#include <stdio.h>

#include "src/modules/builtins/builtin.h"
#include "include/history.h"

int moss_history(char **cmd)
{
    if (!cmd)
        return 1;

    if (cmd[1] != NULL && strcmp(cmd[1], "-c") == 0)
    {
        history_clear();
        return 1;
    }

    int count = history_count();

    if (count == 0)
        return 1;

    for (int i = count - 1; i >= 0; i--)
    {
        const char *entry = history_get(i);

        if (entry)
            printf("    %d  %s\n", count - i, entry);
    }

    return 1;
}