#include <assert.h>
#include "wrapped.h"
#include "myshell.h"
#include "job.h"


int parseline(char* buf, char** argv)
{
    char *delim;  //delimiter
    int argc;
    int bg; //background job?


    buf[strlen(buf) - 1] = ' ';
    while(*buf && (*buf == ' '))
        buf++;
    
    argc = 0;
    while((delim = strchr(buf, ' '))){
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while(*buf && (*buf == ' '))
            buf++;
    }

    argv[argc] = NULL;

    if(argc == 0){
        return 1;
    }

    /* end with '&'? */
    if((bg = (*argv[argc-1] == '&')) != 0){
        argv[--argc] = NULL;
    }

    return bg;
}

int builtin_command(char **argv)
{
    if(!strcmp(argv[0], "quit") || !strcmp(argv[0], "exit"))
        exit(0);

    if(!strcmp(argv[0], "&"))
        return 1;

    if(!strcmp(argv[0], "jobs")){
        print_jobs();
        return 1;
    }

    //fg command: fg %jid or fg pid
    if(!strcmp(argv[0], "fg")){
        int id;
        if((id = parse_id(argv[1])) != -1 && argv[2] == NULL){
            sigset_t mask_one, prev_one;

            /*block signal sigchild*/
            Sigemptyset(&mask_one);
            Sigaddset(&mask_one, SIGCHLD);
            Sigprocmask(SIG_BLOCK, &mask_one, &prev_one);

            pid_t pid = id;
            if(argv[1][0] == '%'){   //if param is %jid
                JobPtr jp = find_job_by_jid(id);
                pid = jp->pid;
            }
            Kill(pid, SIGCONT);
            set_fg_pid(pid);
            while(get_fg_pid()){
                Sigsuspend(&prev_one);
            }
            
            /* unblock signal sigchild*/
            Sigprocmask(SIG_SETMASK, &prev_one, NULL);
        }
        else{
            printf("format error, e.g. fg %%12 || fg 1498\n");
        }
        return 1;
    }

    //bg: bg %jid or bg pid
    if(!strcmp(argv[0], "bg")){
        int id;
        if((id = parse_id(argv[1])) != -1 && argv[2] == NULL){
            pid_t pid = id;
            if(argv[1][0] == '%'){
                JobPtr jp = find_job_by_jid(id);
                pid = jp->pid;
            }
            Kill(pid, SIGCONT);
        }
        else{
            printf("format error, e.g. bg %%12 or bg 1498\n");
        }
        return 1;
    }
    return 0;
}

void eval(char *cmdline)
{
    char *argv[MAXARGS];
    char buf[MAXLINE];
    int bg;
    pid_t pid;

    strcpy(buf, cmdline);
    bg = parseline(buf, argv);
    if(argv[0] == NULL){
        return;
    }

    /*if the command is not a builtin command*/
    if(!builtin_command(argv)){
        sigset_t mask_one, prev_one;
        Sigemptyset(&mask_one);
        Sigaddset(&mask_one, SIGCHLD);

        Sigprocmask(SIG_BLOCK, &mask_one, &prev_one);
        if((pid = Fork()) == 0){
            Sigprocmask(SIG_SETMASK, &prev_one, NULL);

            Setpgid(0, 0);

            if(execve(argv[0], argv, environ) < 0){
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
        }

        sigset_t mask_all, prev_all;
        Sigfillset(&mask_all);

        Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
        Jid new_jid = new_job(pid, cmdline, !bg);
        Sigprocmask(SIG_SETMASK, &prev_all, NULL);

        if(!bg){
            set_fg_pid(pid);
            while(get_fg_pid())
                Sigsuspend(&prev_one);
        }
        else{
            printf("[%d] %d %s \t %s\n", new_jid, pid, "Running", cmdline);
        }
        Sigprocmask(SIG_SETMASK, &prev_one, NULL);
    }

    return;
}

static int is_number_str(char* s)
{
    int len = strlen(s);
    for(int i = 0; i < len; i++){
        if(!isdigit(s[i]))
            return 0;
    }
    return 1;
}

/* convert jid or pid to a integer*/
int parse_id(char* s)
{
    int error = -1;
    if(!s){
        return error;
    }

    if(s[0] == '%'){   //jid
        if(!is_number_str(s+1))
            return error;
        return atoi(s+1);
    }

    if(is_number_str(s))  //pid
        return atoi(s);

    return error;
}

void test_shell()
{
    assert(-1 == parse_id("ns"));
    assert( -1 == parse_id("%%"));
    assert(0 == parse_id("%0"));
    assert(98 == parse_id("%98"));
    assert(98 == parse_id("98"));
}
