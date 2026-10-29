#include "../commands.h"
#include <kernel/shell.h>
#include <stdio.h>

void pwd(char *args[], int argc) { printf("%s\n", shell_get_cwd()); }
