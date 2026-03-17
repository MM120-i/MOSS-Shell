#include "builtin.h"

const struct builtin builtins[] = {
    {"cd", moss_cd},
    {"help", moss_help},
    {"exit", moss_exit},
    {"pwd", moss_pwd},
    {"clear", moss_clear},
    {"echo", moss_echo},
    {"whoami", moss_whoami},
    {"env", moss_env},
    {"export", moss_export},
    {"unset", moss_unset},
    {"type", moss_type},
    {"ls", moss_ls},
};

const int NUM_BUILTINS = sizeof(builtins) / sizeof(struct builtin);
