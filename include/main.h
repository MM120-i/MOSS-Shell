#pragma once

#define LSH_TOK_BUFFERSIZE 64
#define LSH_TOK_DELIMETER " \t\r\n\a"

void lsh_loop(void);
char *lsh_read_line(void);
char **lsh_split_line(char *);
int lsh_execute(char **);
int lsh_launch(char **);