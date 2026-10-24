#ifndef _KERNEL_SHELL_H
#define _KERNEL_SHELL_H

#define CD_PATH_MAX 512

void shell_buffer(char c);
void init_shell();
char *shell(char *command);
void run_shell();

int shell_set_cwd(const char *path);
const char *shell_get_cwd(void);
char *get_full_path(const char *path);

extern volatile int shell_command_ready;

#endif
