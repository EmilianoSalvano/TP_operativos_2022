#include <stdlib.h>
#include <stdio.h>
#include "include/main.h"
#include "include/cpu.h"
#include <pthread.h>
#include "../shared/include/utils.h"

int main(int argc, char **argv) {

	int log_level = atoi(argv[1]);

	cpu_iniciar(log_level, argv[2]);
	return 0;
}


