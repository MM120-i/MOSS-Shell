#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "include/parser.h"
#include "include/main.h"
#include "include/logging.h"

private bool isEnvVarChar(char);

private bool isEnvVarChar(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

char *expandEnvVar(const char *token)
{
    if (!token)
    {
        SAFE_ERROR(ERR_CATEGORY_INPUT, "MOSS: Input is empty");
        return NULL;
    }

    if (strchr(token, '$') == NULL)
        return strdup(token);

    size_t resultSize = 64;
    char *result = (char *)malloc(resultSize);

    if (!result)
    {
        SAFE_ERROR(ERR_CATEGORY_MEMORY, "MOSS: Memory allocation failed");
        return NULL;
    }

    result[0] = '\0';
    const size_t len = strlen(token);
    size_t resultPosition = 0;

    for (size_t i = 0; i < len; i++)
    {
        if (token[i] != '$')
        {
            result[resultPosition++] = token[i];
            result[resultPosition] = '\0';
        }
        else
        {
            if (i + 1 >= len)
            {
                result[resultPosition++] = '$';
                result[resultPosition] = '\0';
            }
            else if (token[i + 1] == '$')
            {
                char pidStr[32];
                snprintf(pidStr, sizeof(pidStr), "%d", getpid());
                size_t pidLen = strlen(pidStr);

                while (resultPosition + pidLen + 1 > resultSize)
                {
                    resultSize *= 2;
                    char *newResult = realloc(result, resultSize);

                    if (!newResult)
                    {
                        free(result);
                        SAFE_ERROR(ERR_CATEGORY_MEMORY, "MOSS: Memory allocation failed for");
                        return NULL;
                    }

                    result = newResult;
                }

                strcat(result, pidStr);
                resultPosition += pidLen;
                i++;
            }
            else if (token[i + 1] == '{')
            {
                size_t start = i + 2;
                size_t end = start;

                while (end < len && token[end] != '}')
                    end++;

                if (end == len)
                {
                    result[resultPosition++] = '$';
                    result[resultPosition++] = '{';
                    result[resultPosition] = '\0';
                    i = len - 1;
                }
                else
                {
                    const size_t varLen = end - start;
                    char *varName = (char *)malloc(varLen + 1);

                    if (!varName)
                    {
                        free(result);
                        SAFE_ERROR(ERR_CATEGORY_MEMORY, "MOSS: Memory allocation failed");
                        return NULL;
                    }

                    strncpy(varName, token + start, varLen);
                    varName[varLen] = '\0';
                    char *value = getenv(varName);

                    if (value)
                    {
                        const size_t valLen = strlen(value);

                        while (resultPosition + valLen + 1 > resultSize)
                        {
                            resultSize *= 2;
                            char *newResult = realloc(result, resultSize);

                            if (!newResult)
                            {
                                free(result);
                                free(varName);
                                SAFE_ERROR(ERR_CATEGORY_MEMORY, "MOSS: Memory allocation failed");
                                return NULL;
                            }

                            result = newResult;
                        }

                        strcat(result, value);
                        resultPosition += valLen;
                    }

                    free(varName);
                    i = end;
                }
            }
            else if (!isEnvVarChar(token[i + 1]))
            {
                result[resultPosition++] = '$';
                result[resultPosition] = '\0';
            }
            else
            {
                size_t start = i + 1;
                size_t end = start;

                while (end < len && isEnvVarChar(token[end]))
                    end++;

                const size_t varLen = end - start;
                char *varName = (char *)malloc(varLen + 1);

                if (!varName)
                {
                    free(result);
                    SAFE_ERROR(ERR_CATEGORY_MEMORY, "MOSS: Memory allocation failed");
                    return NULL;
                }

                strncpy(varName, token + start, varLen);
                varName[varLen] = '\0';
                char *value = getenv(varName);

                if (value)
                {
                    const size_t valLen = strlen(value);

                    while (resultPosition + valLen + 1 > resultSize)
                    {
                        resultSize *= 2;
                        char *newResult = realloc(result, resultSize);

                        if (!newResult)
                        {
                            free(result);
                            free(varName);
                            SAFE_ERROR(ERR_CATEGORY_MEMORY, "MOSS: Memory allocation failed");
                            return NULL;
                        }

                        result = newResult;
                    }

                    strcat(result, value);
                    resultPosition += valLen;
                }

                free(varName);
                i = end - 1;
            }
        }
    }

    return result;
}

char *strip_quotes(char *token)
{
    if (!token)
        return NULL;

    size_t len = strlen(token);

    if (len < 2)
        return token;

    if ((token[0] == '"' && token[len - 1] == '"') || (token[0] == '\'' && token[len - 1] == '\''))
    {
        token[len - 1] = '\0';
        return token + 1;
    }

    return token;
}

bool isSafe(const char *token)
{
    if (!token || *token == '\0')
        return 0;

    return strcspn(token, DANGEROUS_CHARS) == strlen(token);
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

        tokens[position++] = strdup(token);
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

// strip_comment()