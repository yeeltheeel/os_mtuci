#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <sched.h>

#define MAX_M 10
#define MAX_N 10
#define MAX_K 10
#define NUM_WORKERS 2
#define SOCK_NAME "/tmp/matrix_sock"
#define SHM_NAME "/matrix_shm"

typedef struct { 
	int row, col; 
} Task;

typedef struct { 
	int row, col; 
	double value; 
} Result;

int M, N, K;
double A[MAX_M][MAX_N];
double B[MAX_N][MAX_K];
double C[MAX_M][MAX_K];

double compute_cell(int i, int j) {
	double s = 0.0;
	for (int t = 0; t < N; t++)
		s += A[i][t] * B[t][j];
	return s;
}

void input_matrices(void) {
	printf("Input M N K: ");
	scanf("%d %d %d", &M, &N, &K);
	printf("Matrix A (%dx%d):\n", M, N);
	for (int i = 0; i < M; i++)
		for (int j = 0; j < N; j++)
			scanf("%lf", &A[i][j]);
	printf("Matrix B (%dx%d):\n", N, K);
	for (int i = 0; i < N; i++)
		for (int j = 0; j < K; j++)
			scanf("%lf", &B[i][j]);
}

void print_result(void) {
	printf("Result C (%dx%d):\n", M, K);
	for (int i = 0; i < M; i++) {
		for (int j = 0; j < K; j++)
			printf("%8.2f ", C[i][j]);
		printf("\n");
	}
}

// pipe

void run_pipe(void) {
	int task_pipe[NUM_WORKERS][2];
	int res_pipe[NUM_WORKERS][2];
	pid_t pids[NUM_WORKERS];

	for (int w = 0; w < NUM_WORKERS; w++) {
		if (pipe(task_pipe[w]) < 0 || pipe(res_pipe[w]) < 0) {
			perror("pipe"); 
			exit(1);
		}
	}

	for (int w = 0; w < NUM_WORKERS; w++) {
		pids[w] = fork();
		if (pids[w] == 0) {
			for (int k = 0; k < NUM_WORKERS; k++) {
				close(task_pipe[k][1]); 
				close(res_pipe[k][0]);   
				if (k != w) { 
					close(task_pipe[k][0]);
					close(res_pipe[k][1]);
				}
			}

			read(task_pipe[w][0], A, sizeof(A));
			read(task_pipe[w][0], B, sizeof(B));
			int mn[3];
			read(task_pipe[w][0], mn, sizeof(mn));
			M = mn[0]; 
			N = mn[1]; 
			K = mn[2];

			Task t;
			while (read(task_pipe[w][0], &t, sizeof(Task)) == sizeof(Task)) {
				Result r = { t.row, t.col, compute_cell(t.row, t.col) };
				write(res_pipe[w][1], &r, sizeof(Result));
			}
			close(task_pipe[w][0]);
			close(res_pipe[w][1]);
			exit(0);
		}
	}

	for (int w = 0; w < NUM_WORKERS; w++) {
		close(task_pipe[w][0]);  
		close(res_pipe[w][1]);  
	}

	for (int w = 0; w < NUM_WORKERS; w++) {
		write(task_pipe[w][1], A, sizeof(A));
		write(task_pipe[w][1], B, sizeof(B));
		int mn[3] = {M, N, K};
		write(task_pipe[w][1], mn, sizeof(mn));
	}

	int total = M * K, sent = 0, received = 0;
	while (received < total) {
		for (int w = 0; w < NUM_WORKERS && sent < total; w++) {
			/* */
		}

		fd_set rset; FD_ZERO(&rset);
		int maxfd = -1;
		for (int w = 0; w < NUM_WORKERS; w++) {
			FD_SET(res_pipe[w][0], &rset);
			if (res_pipe[w][0] > maxfd) 
				maxfd = res_pipe[w][0];
		}

		while (sent < total) {
			int w = sent % NUM_WORKERS;
			Task t = { sent / K, sent % K };
			if (write(task_pipe[w][1], &t, sizeof(Task)) != sizeof(Task)) 
				break;
			sent++;
			if (sent >= total) break;
		}

		if (select(maxfd + 1, &rset, NULL, NULL, NULL) < 0) break;
		for (int w = 0; w < NUM_WORKERS; w++) {
			if (FD_ISSET(res_pipe[w][0], &rset)) {
				Result r;
				if (read(res_pipe[w][0], &r, sizeof(Result)) == sizeof(Result)) {
					C[r.row][r.col] = r.value;
					received++;
				}
			}
		}
	}

	for (int w = 0; w < NUM_WORKERS; w++) {
		close(task_pipe[w][1]);
		close(res_pipe[w][0]);
		waitpid(pids[w], NULL, 0);
	}
}

// shm -- shared memory

typedef struct {
	int M, N, K;
	double A[MAX_M][MAX_N];
	double B[MAX_N][MAX_K];
	double C[MAX_M][MAX_K];
	Task tasks[MAX_M * MAX_K];
	int task_ready[MAX_M * MAX_K];
	int result_ready[MAX_M * MAX_K];
	int total_tasks;
	int next_task;
} SharedData;

void run_shm(void) {
	int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
	if (fd < 0) { 
		perror("shm_open"); 
		exit(1); 
	}
	ftruncate(fd, sizeof(SharedData));
	SharedData *sh = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (sh == MAP_FAILED) { 
		perror("mmap"); 
		exit(1); 
	}

	sh->M = M; sh->N = N; sh->K = K;
	memcpy(sh->A, A, sizeof(A));
	memcpy(sh->B, B, sizeof(B));
	sh->total_tasks = M * K;
	sh->next_task = 0;
	memset(sh->task_ready, 0, sizeof(sh->task_ready));
	memset(sh->result_ready, 0, sizeof(sh->result_ready));

	pid_t pids[NUM_WORKERS];
	for (int w = 0; w < NUM_WORKERS; w++) {
		pids[w] = fork();
		if (pids[w] == 0) {
			while (1) {
				int idx = __sync_fetch_and_add(&sh->next_task, 1);
				if (idx >= sh->total_tasks) 
					break;
				Task t = { idx / sh->K, idx % sh->K };
				sh->tasks[idx] = t;
				sh->task_ready[idx] = 1;

				double v = 0.0;
				for (int tt = 0; tt < sh->N; tt++)
					v += sh->A[t.row][tt] * sh->B[tt][t.col];
				sh->C[t.row][t.col] = v;
				sh->result_ready[idx] = 1;
			}
			munmap(sh, sizeof(SharedData));
			exit(0);
		}
	}

	int done = 0;
	while (done < sh->total_tasks) {
		for (int i = 0; i < sh->total_tasks; i++) {
			if (sh->result_ready[i] && !sh->task_ready[i]) {
				/* */
			}
		}
		done = 0;
		for (int i = 0; i < sh->total_tasks; i++)
			if (sh->result_ready[i]) done++;
		sched_yield();
	}
	memcpy(C, sh->C, sizeof(C));

	for (int i = 0; i < NUM_WORKERS; i++) 
		waitpid(pids[i], NULL, 0);
	munmap(sh, sizeof(SharedData));
	shm_unlink(SHM_NAME);
}

// socket

void run_socket(void) {
	int sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sock < 0) { 
		perror("socket"); 
		exit(1); 
	}

	struct sockaddr_un srv;
	srv.sun_family = AF_UNIX;
	strcpy(srv.sun_path, SOCK_NAME);
	unlink(SOCK_NAME);
	if (bind(sock, (struct sockaddr*)&srv, sizeof(srv)) < 0) {
		perror("bind"); 
		exit(1);
	}

	pid_t pids[NUM_WORKERS];
	for (int w = 0; w < NUM_WORKERS; w++) {
		pids[w] = fork();
		if (pids[w] == 0) {
			int cs = socket(AF_UNIX, SOCK_DGRAM, 0);
			struct sockaddr_un cli;
			cli.sun_family = AF_UNIX;
			sprintf(cli.sun_path, "/tmp/matrix_cli_%d", getpid());
			unlink(cli.sun_path);
			bind(cs, (struct sockaddr*)&cli, sizeof(cli));

			int type = 1; 
			sendto(cs, &type, sizeof(int), 0, (struct sockaddr*)&srv, sizeof(srv));
			double bufA[MAX_M][MAX_N], bufB[MAX_N][MAX_K];
			int mn[3];
			recvfrom(cs, bufA, sizeof(bufA), 0, NULL, NULL);
			recvfrom(cs, bufB, sizeof(bufB), 0, NULL, NULL);
			recvfrom(cs, mn, sizeof(mn), 0, NULL, NULL);
			int N_ = mn[1];

			while (1) {
				type = 2; 
				sendto(cs, &type, sizeof(int), 0, (struct sockaddr*)&srv, sizeof(srv));
				Task t;
				recvfrom(cs, &t, sizeof(Task), 0, NULL, NULL);
				if (t.row < 0) 
					break; 
				double v = 0.0;
				for (int tt = 0; tt < N_; tt++)
					v += bufA[t.row][tt] * bufB[tt][t.col];
				type = 4;
				sendto(cs, &type, sizeof(int), 0, (struct sockaddr*)&srv, sizeof(srv));
				Result r = { t.row, t.col, v };
				sendto(cs, &r, sizeof(Result), 0, (struct sockaddr*)&srv, sizeof(srv));
			}
			close(cs);
			unlink(cli.sun_path);
			exit(0);
		}
	}

	int total = M * K, sent = 0, received = 0;
	int client_count = 0;
	struct sockaddr_un clients[NUM_WORKERS];

	while (received < total) {
		int type;
		struct sockaddr_un cli;
		socklen_t len = sizeof(cli);
		ssize_t n = recvfrom(sock, &type, sizeof(int), 0, (struct sockaddr*)&cli, &len);
		if (n != sizeof(int)) 
			continue;

		if (type == 1) {
			sendto(sock, A, sizeof(A), 0, (struct sockaddr*)&cli, len);
			sendto(sock, B, sizeof(B), 0, (struct sockaddr*)&cli, len);
			int mn[3] = {M, N, K};
			sendto(sock, mn, sizeof(mn), 0, (struct sockaddr*)&cli, len);
			if (client_count < NUM_WORKERS) 
				clients[client_count++] = cli;
		} else if (type == 2) {
			Task t = { -1, -1 };
			if (sent < total) { 
				t.row = sent / K; 
				t.col = sent % K; 
				sent++; 
			}
			sendto(sock, &t, sizeof(Task), 0, (struct sockaddr*)&cli, len);
		} else if (type == 4) {
			Result r;
			recvfrom(sock, &r, sizeof(Result), 0, NULL, NULL);
			C[r.row][r.col] = r.value;
			received++;
		}
	}

	for (int i = 0; i < NUM_WORKERS; i++) {
		Task stop = { -1, -1 };
		sendto(sock, &stop, sizeof(Task), 0, (struct sockaddr*)&clients[i], sizeof(clients[i]));
	}
	for (int i = 0; i < NUM_WORKERS; i++) 
		waitpid(pids[i], NULL, 0);
	close(sock);
	unlink(SOCK_NAME);
}

// main

int main(int argc, char *argv[]){
	if (argc < 2) {
		fprintf(stderr, "usage: %s {pipe|shm|socket}\n", argv[0]);
		return 1;
	}
	input_matrices();

	struct timespec t0, t1;
	clock_gettime(CLOCK_MONOTONIC, &t0);

	if (strcmp(argv[1], "pipe") == 0)
		run_pipe();
	else if (strcmp(argv[1], "shm") == 0)
		run_shm();
	else if (strcmp(argv[1], "socket") == 0) 
		run_socket();
	else { 
		fprintf(stderr, "unknown mode\n"); 
		return 1; 
	}

	clock_gettime(CLOCK_MONOTONIC, &t1);
	double dt = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
	printf("elapsed (%s): %.6f s\n", argv[1], dt);
	print_result();

	return 0;
}
