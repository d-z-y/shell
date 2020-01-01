#include <stdio.h>
#include <assert.h>
#include "job.h"
#include "wrapped.h"


static volatile sig_atomic_t fg_pid;
static Job jobs[MAXJOBS];

int is_fg_pid(pid_t pid)
{
    return fg_pid == pid;
}

pid_t get_fg_pid()
{
    return fg_pid;
}

void set_fg_pid(pid_t pid)
{
    fg_pid = pid;
}

void sigchild_handler(int sig)
{
    int old_errno = errno;
    int status;
    pid_t pid;

    sigset_t mask_all, pre_all;
    Sigfillset(&mask_all);

    while((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0){

        /*exit normally*/
        if(WIFEXITED(status) || WIFSIGNALED(status)){
            if(is_fg_pid(pid)){
                set_fg_pid(0);
            }
            else{
                Sio_puts("pid ");
                Sio_putl(pid);
                Sio_puts(" terminated\n");
            }
            Sigprocmask(SIG_BLOCK, &mask_all, &pre_all);
            del_job_by_pid(pid);
            Sigprocmask(SIG_SETMASK, &pre_all, NULL);
        }

        /*be stopped */
        if(WIFSTOPPED(status)){
            if(is_fg_pid(pid)){
                set_fg_pid(0);
            }
            Sigprocmask(SIG_BLOCK, &mask_all, &pre_all);
            JobPtr jp = find_job_by_pid(pid);
            set_job_status(jp, Stopped);
            Sigprocmask(SIG_SETMASK, &pre_all, NULL);

            Sio_puts("pid");
            Sio_putl(pid);
            Sio_puts(" be stopped\n");
        }

        if(WIFCONTINUED(status)){
            set_fg_pid(pid);
            Sigprocmask(SIG_BLOCK, &mask_all, &pre_all);
            JobPtr jp = find_job_by_pid(pid);
            set_job_status(jp, Running);
            Sigprocmask(SIG_SETMASK, &pre_all, NULL);

            Sio_puts("pid ");
            Sio_putl(pid);
            Sio_puts(" continue\n");
        }
    }
    errno = old_errno;
}

void sigint_handler(int sig)
{
    if(is_fg_pid(0)){
        Signal(SIGINT, SIG_DFL);
        Kill(getpid(), SIGINT);
    }
    else{
        Kill(get_fg_pid(), SIGINT);
    }
}

void sigstop_handler(int sig)
{
    if(is_fg_pid(0)){
        Signal(SIGTSTP, SIG_DFL);
        Kill(getpid(), SIGTSTP);
    }
    else{
        Kill(get_fg_pid(), SIGTSTP);  //Ctrl+Z
    }
}

JobPtr find_job_by_jid(Jid jid)
{
    return &(jobs[jid]);
}

JobPtr find_job_by_pid(pid_t pid)
{
    for(int i = 0; i < MAXJOBS; i++){
        Job j = jobs[i];
        if(j.using && j.pid == pid){
            return &(jobs[i]);
        }
    }
    return NULL;
}

void set_job_status(JobPtr jp, enum JobStatus status)
{
    if(jp){
        jp->status = status;
    }
}

static int find_spare_jid()
{
    Jid jid = -1;
    for(int i = 0; i < MAXJOBS; i++){
        if(jobs[i].using == 0){
            jid = i;
            break;
        }
    }
    return jid;
}

int new_job(pid_t pid, char* cmdline, int fg)
{
    Jid jid = find_spare_jid();
    if(jid == -1)
        unix_error("No more jid to use");
    
    jobs[jid].jid = jid;
    jobs[jid].pid = pid;
    jobs[jid].status = Running;
    strcpy(jobs[jid].cmdline, cmdline);
    jobs[jid].using = 1;

    return jid;
}

void del_job_by_pid(pid_t pid)
{
    for(int i = 0; i < MAXJOBS; i++){
        if(jobs[i].using && jobs[i].pid == pid){
            jobs[i].using = 0;
        }
    }
}

void print_jobs()
{
    for(int i = 0; i < MAXJOBS; i++){
        Job j = jobs[i];
        if(j.using){
            printf("[%d] %d %s \t %s\n", j.jid, j.pid, 
                    j.status == Running ? "Running" : "Stopped", 
                    j.cmdline);
        }
    }
}

void init_jobs()
{
    memset(jobs, 0, sizeof(jobs));
}

void test_job()
{

}
