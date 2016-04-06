#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#define ERR_EXIT(...) err_exit_(__LINE__, "%d: " __VA_ARGS__)
template <typename... Args> void err_exit_(int lin_, char const* fmt, Args... a)
{
    fprintf(stderr, fmt, lin_, a...);
    exit(127);
}
#define ERR_MSG(...) err_msg_(__LINE__, "%d: " __VA_ARGS__)
template <typename... Args> void err_msg_(int lin_, char const* fmt, Args... a)
{
    fprintf(stderr, fmt, lin_, a...);
    //fflush(stderr);
}

struct Proc { const char* path; pid_t pid; time_t ts; };

static int Fork(Proc* proc, char const* arg1)
{
    switch (pid_t pid = fork()) {
        case 0:  /* Child - reads from pipe */
            //close(fildes[1]);                       /* Write end is unused */
            //nbytes = read(fildes[0], buf, BSIZE);   /* Get data from pipe */
            ///* At this point, a further read would see end of file ... */
            //close(fildes[0]);                       /* Finished with pipe */
            //exit(EXIT_SUCCESS);
            execlp(proc->path, proc->path, arg1, NULL);
            ERR_MSG("execlp");
            return 5;
            break;

        default:  /* Parent - writes to pipe */
            if (pid < 0)  { //case −1: /* Handle error */
                ERR_MSG("fork");
                return 10;
            }

            //close(fildes[0]);                       /* Read end is unused */
            //write(fildes[1], "Hello world\n", 12);  /* Write data on pipe */
            //close(fildes[1]);                       /* Child will see EOF */
            //exit(EXIT_SUCCESS);
            proc->pid = pid;
            proc->ts = time(0);
    }
    return 0;
}

int main(int argc, char* const argv[])
{
    //mkfifo; //signal; //pipe; //sigaction; wait;
    daemon(0,1);

    Proc procs[2] = {{"/opt/bin/mdserver"},{"/opt/bin/ctpmd"}};
    Proc *proc;
    char const* fifo = (argc>1 ? argv[1] : "/tmp/mys_fifo_");
    if (argc >= 3)
        procs[0].path = argv[2];
    if (argc >= 4)
        procs[1].path = argv[3];

    remove(fifo);
    mkfifo(fifo, 0600);
    Fork(&procs[0], fifo);
    Fork(&procs[1], fifo);

    while (1) {
        pid_t pid = wait(NULL);
        if (pid > 0) {
            remove(fifo);

            if (pid == procs[0].pid)
                proc = &procs[0];
            else if (pid == procs[1].pid)
                proc = &procs[1];
            else
                ERR_EXIT("pid %d", pid);

            if (time(0) - proc->ts < 1)
                sleep(1);
            mkfifo(fifo, 0600);
            int ws = Fork(proc, fifo);
            if (ws > 0)
                sleep(ws);
        }
    }
}

