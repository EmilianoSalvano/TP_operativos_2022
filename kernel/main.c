/*
 * main.c
 *
 *  Created on: 19 jun. 2022
 *      Author: utnso
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>

#include "include/main.h"
#include "include/kernel.h"
#include "../shared/include/utils.h"


int main(int argc, char **argv) {


	int log_level = atoi(argv[1]);

	kernel_execute(log_level, argv[2]);

	pthread_exit(NULL);
}
