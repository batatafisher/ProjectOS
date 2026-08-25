#ifndef _KERNEL_SHELL_H
#define _KERNEL_SHELL_H

void shell_buffer(char c);
void init_shell();
char *shell();

extern volatile int shell_command_ready;

#endif
