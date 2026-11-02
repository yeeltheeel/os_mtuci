#define _GNU_SOURCE
#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>

int main(void){
	pid_t pid;
	pid_t child_pid[2] = {0, 0};

	for (int i=0; i < 2; i++){
		pid = fork();
		if (pid == 0){
			time_t now = time(NULL);
			struct tm *tm = localtime(&now);

			printf(
				"child with pid=%d and ppid=%d [%02d:%02d:%02d]\n", 
				getpid(), 
				getppid(),
				tm->tm_hour, tm->tm_min, tm->tm_sec
			);
			fflush(stdout);

			if (i == 0){
				// main source is iostat, if not available fallback to /proc/diskstats
				if (system("command -v iostat >/dev/null 2>&1") == 0) {
					system("iostat -x 1 1");
				} else {
					system("cat /proc/diskstats");
				}
			}

			exit(0);
		}
		else if (pid > 0){
			child_pid[i] = pid;
			time_t now = time(NULL);
			struct tm *tm = localtime(&now);

			printf(
				"parent with pid=%d and ppid=%d [%02d:%02d:%02d]\n", 
				getpid(), 
				getppid(),
				tm->tm_hour, tm->tm_min, tm->tm_sec
			);
			fflush(stdout);
		}
		else {
			perror("fork");
			exit(1);
		}
	}

	int status;
	pid_t tmp;
	for (int i = 0; i < 2; i++) {
		do {
			tmp = waitpid(child_pid[i], &status, 0);
		} while (tmp == -1 && errno == EINTR);

		if (tmp == -1) {
			perror("waitpid");
			continue;
		}
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
	}

	return 0;
}
