#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]){
	if (argc < 2) {
		fprintf(stderr, "usage: %s {p1|p2}\n", argv[0]);
		return 1;
	}

	if (strcmp(argv[1], "p1") == 0)
		;
	else if (strcmp(argv[1], "p2") == 0)
		;
	else { 
		fprintf(stderr, "unknown mode\n"); 
		return 1; 
	}

	return 0;
}