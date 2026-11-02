#define _GNU_SOURCE
#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

void *thread_task(void *arg){
	(void)arg;
	time_t now = time(NULL);
	struct tm tm_buf;
	struct tm *tm = localtime_r(&now, &tm_buf);

	printf(
		"child with uid=%d, tid=%d and ppid=%d [%02d:%02d:%02d]\n", 
		getuid(), 
		gettid(),
		getppid(),
		tm->tm_hour, tm->tm_min, tm->tm_sec
	);
	fflush(stdout);

	return NULL;
}

int main(void){
	pthread_t child_thread[2] = {0, 0};

	time_t now = time(NULL);
	struct tm tm_buf;
	struct tm *tm = localtime_r(&now, &tm_buf);

	printf(
		"parent with uid=%d, tid=%d and ppid=%d [%02d:%02d:%02d]\n", 
		getuid(),
		gettid(),
		getppid(),
		tm->tm_hour, tm->tm_min, tm->tm_sec
	);
	fflush(stdout);

	int status;
	for (int i = 0; i < 2; i++){
		status = pthread_create(&child_thread[i], NULL, thread_task, "child task");
		if (status != 0){
			errno = status;
            perror("pthread_create");
            exit(1);
		}
	}

	for (int i = 0; i < 2; i++){
		pthread_join(child_thread[i], NULL);
	}

	return 0;
}
