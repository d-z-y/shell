#ifndef __MYSHELL_H__
#define __MYSHELL_H__

#define MAXARGS 128

void eval(char* cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);

int parse_id(char *s);

void test_shell();

#endif