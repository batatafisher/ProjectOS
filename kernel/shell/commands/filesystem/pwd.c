#include "../commands.h"
#include <kernel/shell.h>

const char *pwd(char *args[], int argc) { return shell_get_cwd(); }
