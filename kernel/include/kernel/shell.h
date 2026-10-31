#ifndef _KERNEL_SHELL_H
#define _KERNEL_SHELL_H

#include <kernel/mmu.h>

#define CD_PATH_MAX 512

void shell_buffer(char c);
void init_shell();
void run_shell();

int shell_set_cwd(char *path);
const char *shell_get_cwd(void);
char *get_full_path(char *path);

extern volatile int shell_command_ready;

#endif
