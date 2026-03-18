#pragma once

#define private static
#define SIZE 256

extern const int NUM_BUILTINS;

int moss_execute_pipeline(char **);
int moss_launch(char **);
int moss_execute(char **);