#define _GNU_SOURCE
#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>

#define PMESSAGE "aaaaa"

int main(void){
	pid_t pid, child_pid;

	pid = fork();
	if (pid == 0){
		execl("/bin/echo", "echo", "Hello world!", (char *)NULL);
		perror("execl");
		exit(1);
	}
	else if (pid > 0){
		child_pid = pid;
		printf("%s\n", PMESSAGE);
        fflush(stdout);
	}
	else {
		perror("fork");
		exit(1);
	}

	int status;
	pid_t tmp = waitpid(child_pid, &status, 0);
	if (WIFEXITED(status)){
		printf(
			"reaped child %d, exit=%d\n", 
			tmp, 
			WEXITSTATUS(status)
		);
	}
	else if (WIFSIGNALED(status)){
		printf(
			"reaped child %d, signal=%d\n", 
			tmp, 
			WTERMSIG(status)
		);
	}

	return 0;
}
