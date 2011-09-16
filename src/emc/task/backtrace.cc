// based on http://stackoverflow.com/questions/4636456/stack-trace-for-c-using-gcc/4732119#4732119
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

extern int done;
extern int emcOperatorError(int id, const char *fmt, ...);

void backtrace(int signo)
{
    char pid_buf[30];
    char name_buf[512];
    char filename[512];

    signal(signo, SIG_IGN); // prevent multiple invocations on same signal
    sprintf(pid_buf, "%d", getpid());
    sprintf(filename,"/tmp/backtrace.%s", pid_buf);

    name_buf[readlink("/proc/self/exe", name_buf, 511)]=0;
    int child_pid = fork();
    if (!child_pid) {
	fclose(stderr);
	stderr = freopen(filename, "a", stderr);
        dup2(2,1); // child: redirect output to stderr
        fprintf(stdout,"stack trace for %s pid=%s signal=%d\n",name_buf,pid_buf, signo);
        execlp("gdb", "gdb", "--batch", "-n", "-ex", "thread", "-ex", "bt", name_buf, pid_buf, NULL);
	emcOperatorError(0, "backtrace for %s (pid %s signal %d): gdb failed to start", name_buf, pid_buf, signo);
        abort(); /* If gdb failed to start */
    } else {
	int status;
        waitpid(child_pid, &status,0);
	if (signo == SIGUSR1) {  // continue running after backtrace
	    signal(SIGUSR1, backtrace);
	    fprintf(stderr, "%s continuing status=%d\n", name_buf, status);
	} else {
	    // this takes emcmodule.cc:EMC_COMMAND_TIMEOUT seconds to display:
	    if (status == 0) // backtrace succeeded
		emcOperatorError(0, "%s (pid %d) died on signal %d, backtrace stored in %s",
				 name_buf, getpid(), signo, filename);
	    fprintf(stderr, "%s exiting\n", name_buf);
	    done = 1;  // signal task to exit main loop
	}
    }
}
