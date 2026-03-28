#pragma once

#include <termios.h>

#define private static

void moss_input_init();
void moss_input_restore();
char *moss_input_readline(const char *);