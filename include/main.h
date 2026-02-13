#pragma once

#define TOK_INITIAL_BUFFER 64
#define TOK_MAX_TOKEN_LEN 4096
#define TOK_DELIMETER " \t\r\n\a"

void moss_loop();
char *moss_read_line();
char **moss_split_line(char *);
int moss_execute(char **);
int moss_launch(char **);