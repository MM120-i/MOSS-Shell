#pragma once

#define TOK_BUFFERSIZE 64
#define TOK_DELIMETER " \t\r\n\a"

void moss_loop();
char *moss_read_line();
char **moss_split_line(char *);
int moss_execute(char **);
int moss_launch(char **);