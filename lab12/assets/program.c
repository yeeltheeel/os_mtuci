#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#define BLOCK_MIN_KIB (4UL) 
#define BLOCK_MAX_KIB (64UL)
#define DELAY_MIN_NS (1000000L)
#define DELAY_WIDTH_NS (9000000L)

typedef struct Block {
	void *ptr;
	struct Block *next;
} Block;

static Block *g_head = NULL;
static volatile sig_atomic_t g_stop = 0;

static int list_add(void *p) {
	Block *b = malloc(sizeof(Block));
	if (!b) 
		return -1;
	b->ptr = p;
	b->next = g_head;
	g_head = b;
	return 0;
}

static void list_remove(void *p) {
	Block **cur = &g_head;
	while (*cur) {
		if ((*cur)->ptr == p) {
			Block *dead = *cur;
			*cur = dead->next;
			free(dead);
			return;
		}
		cur = &(*cur)->next;
	}
}

static void list_free_all(void) {
	Block *cur = g_head;
	while (cur) {
		Block *next = cur->next;
		free(cur->ptr);
		free(cur);
		cur = next;
	}
	g_head = NULL;
}

static void on_signal(int sig) {
	(void)sig;
	g_stop = 1;
}

static void delay_ns(long ns) {
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = ns;
	while (nanosleep(&ts, &ts) == -1 && errno == EINTR) {
		if (g_stop) 
			return;
	}
}

static void touch_block(unsigned char *p, size_t sz) {
	memset(p, 0xAA, sz);
}

static size_t rand_size(void) {
	return (size_t)(BLOCK_MIN_KIB * 1024 + rand() % (BLOCK_MAX_KIB * 1024 - BLOCK_MIN_KIB * 1024));
}

// p1

static void run_mode1(void) {
	void *prev = NULL;
	size_t prev_sz = 0;
	(void)prev_sz;

	while (!g_stop) {
		size_t sz = rand_size();
		unsigned char *cur = malloc(sz);
		if (!cur) {
			perror("malloc");
			break;
		}
		touch_block(cur, sz);
		list_add(cur);

		if (prev) {
			list_remove(prev);
			free(prev);
		}
		prev = cur;
		prev_sz = sz;

		delay_ns(DELAY_MIN_NS + rand() % DELAY_WIDTH_NS);
	}

	if (prev) {
		list_remove(prev);
		free(prev);
		prev = NULL;
	}
	list_free_all();
}

// p2

static void run_mode2(void) {
	unsigned long counter = 0;

	while (!g_stop) {
		size_t sz = rand_size();
		unsigned char *cur = malloc(sz);
		if (!cur) {
			perror("malloc");
			break;
		}
		touch_block(cur, sz);
		list_add(cur);

		counter++;
		if (counter % 5 == 0) {
			list_remove(cur);
			free(cur);
		}

		delay_ns(DELAY_MIN_NS + rand() % DELAY_WIDTH_NS);
	}
	list_free_all();
}

// main

int main(int argc, char *argv[]){
	if (argc < 2) {
		fprintf(stderr, "usage: %s {p1|p2}\n", argv[0]);
		return 1;
	}

	int mode;
	if (strcmp(argv[1], "p1") == 0)
		mode = 1;
	else if (strcmp(argv[1], "p2") == 0)
		mode = 2;
	else { 
		fprintf(stderr, "unknown mode: %s\n", argv[1]); 
		return 1; 
	}

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = on_signal;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGINT,  &sa, NULL); // ctrl+c
	sigaction(SIGTERM, &sa, NULL); // kill

	srand((unsigned)time(NULL) ^ (unsigned)getpid());
	fprintf(stderr, "[pid=%d] mode=%s, waiting for signal to stop\n", (int)getpid(), argv[1]);

	if (mode == 1) 
		run_mode1();
	else
		run_mode2();

	list_free_all();

	fprintf(stderr, "[pid=%d] finished, memory freed\n", (int)getpid());
	return 0;
}