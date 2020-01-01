#include <stdio.h>
#include "wrapped.h"
#include "myshell.h"
#include "job.h"


int main()
{
    test_shell();
    test_job();

    init_jobs();

    if(Signal(SIGCHLD, sigchild_handler) == SIG_ERR)
        unix_error("signal child handler error");
    if(Signal(SIGINT, sigint_handler) == SIG_ERR)
    unix_error("signal int handler error");
    if(Signal(SIGTSTP, sigstop_handler) == SIG_ERR)
        unix_error("signal stop handler error");

    char cmdline[MAXLINE];
    while(1){
        printf("> ");
        Fgets(cmdline, MAXLINE, stdin);
        if(feof(stdin))
            exit(0);

        eval(cmdline);
    }
}