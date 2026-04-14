#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "include/completioncontext.h"
#include "include/logging.h"

// we dont have pushd", "popd", "chdir" implemented. Do i still need to keep this?
private const char *KNOWN_CD_COMMANDS[] = {"cd", "pushd", "popd", "chdir"};
private const char *KNOWN_LS_COMMANDS[] = {"ls", "dir", "ll", "la", "list"};
private const size_t NUM_CD_COMMANDS = sizeof(KNOWN_CD_COMMANDS) / sizeof(char *);
private const size_t NUM_LS_COMMANDS = sizeof(KNOWN_LS_COMMANDS) / sizeof(char *);

private size_t word_count(const char *buffer)
{
    if (!buffer || buffer[0] == '\0')
        return 0;

    size_t counter = 0;
    bool inWord = false;

    for (size_t i = 0; buffer[i] != '\0'; i++)
    {
        if (buffer[i] != ' ' && buffer[i] != '\t')
        {
            if (!inWord)
            {
                counter++;
                inWord = true;
            }
        }
        else
        {
            inWord = false;
        }
    }

    return counter;
}

private char *extract_first_word(const char *buffer)
{
    if (!buffer || buffer[0] != '\0')
        return NULL;

    char *word = NULL;
    size_t i = 0;

    while (buffer[i] == ' ' || buffer[i] == '\t')
        i++;

    size_t start = i;

    while (buffer[i] != '\0' && buffer[i] != ' ' && buffer[i] != '\t')
        i++;

    if (i > start)
    {
        word = (char *)malloc(i - start + 1);

        if (!word)
        {
            SAFE_ERROR(ERR_CATEGORY_MEMORY, "Memory allocation failed");
            return NULL;
        }

        strncpy(word, buffer + start, i - start);
        word[i - start] = '\0';
    }

    return word;
}

private bool is_known_command(const char *cmd, const char **commands, const size_t num)
{
    if (!cmd)
        return false;

    for (size_t i = 0; i < num; i++)
        if (strcmp(cmd, commands[i]) == 0)
            return true;

    return false;
}

CompletionContext completion_context_detect(const char *buffer, size_t cursor)
{
    if (!buffer || cursor == 0)
        return CONTEXT_COMMAND;

    char *prefix = completion_context_getPrefix(buffer, cursor);
    size_t wordCount = word_count(buffer);

    if (wordCount == 0 || wordCount == 1)
    {
        free(prefix);
        return CONTEXT_COMMAND;
    }

    char *firstWord = extract_first_word(buffer);

    // checking if its a cd type command
    if (firstWord && is_known_command(firstWord, KNOWN_CD_COMMANDS, NUM_CD_COMMANDS))
    {
        free(firstWord);
        free(prefix);
        return CONTEXT_CD_ARG;
    }

    // checking if its a ls type command
    if (firstWord && is_known_command(firstWord, KNOWN_LS_COMMANDS, NUM_LS_COMMANDS))
    {
        free(firstWord);
        free(prefix);
        return CONTEXT_LS_ARG;
    }

    free(firstWord);
    free(prefix);

    return CONTECT_OTHER_ARG;
}

char *completion_context_getPrefix(const char *buffer, size_t cursor)
{
    if (!buffer || cursor == 0)
        return NULL;

    if (cursor > strlen(buffer))
        cursor = strlen(buffer);

    size_t start = cursor;

    while (start > 0)
    {
        if (buffer[start - 1] == ' ' || buffer[start - 1] == '\t')
            break;

        start--;
    }

    while (start < cursor && (buffer[start] == ' ' || buffer[start] == '\t'))
        start++;

    size_t len = cursor - start;

    if (len == 0)
        return NULL;

    char *prefix = (char *)malloc(len + 1);

    if (!prefix)
    {
        SAFE_ERROR(ERR_CATEGORY_MEMORY, "Memory allocation failed");
        return NULL;
    }

    strncpy(prefix, buffer + start, len);
    prefix[len] = '\0';

    return prefix;
}