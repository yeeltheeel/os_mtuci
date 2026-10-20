#include <stdio.h> 
#include <unistd.h>

int main(){
	pid_t pid;
	pid_t child_pid[2];

	for (int i=0; i < 2; i++){
		pid = fork();
		if (pid == 0){
			prinf(
				"child_uid=%d with parent_uid=%d\n", 
				getuid(), 
				geteuid()
			);
		}
		else if (pid > 0){
			prinf(
				"parent with uid=%d\n", 
				getuid()
			);
		}
		else {
			prinf("error: no child created\n")
		}
	}

	int status = 1;
	while (status){
		for (int i=0; i < 2; i++){
			waitpid(child_pid[i], &status);
		} 
	}

	return 0
}