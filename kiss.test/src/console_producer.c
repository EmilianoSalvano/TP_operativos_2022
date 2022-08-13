/*
 * mock.console.c
 *
 *  Created on: 25 jul. 2022
 *      Author: utnso
 */


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <commons/process.h>
#include <semaphore.h>
#include <string.h>

#include <commons/string.h>

#include "../include/console_producer.h"
#include "../include/console.h"
#include "../../shared/include/kiss.structs.h"
#include "../../shared/include/kiss.serialization.h"
#include "../../shared/include/socket_handler.h"
#include "../../shared/include/kiss.error.h"
#include "../../shared/include/utils.h"


#define PORT_CONSOLE_SERVER "8000"
#define IP_CONSOLE_SERVER "127.0.0.1"

const t_console_test TEST = INTEGRAL;


static void producer();
static void handle_console();


int mock_console_run() {
	printf("console-producer :: iniciando productor de consolas ...\n");

	pthread_t tid;
	if (pthread_create(&tid, NULL, (void*)producer, NULL) != 0) {
		printf("console-producer :: Error al crear el hilo del productor de consolas\n");
		return EXIT_FAILURE;
	}

	if (pthread_detach(tid) != 0) {
		printf("console-producer :: Error al crear el hilo del productor de consolas\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}



static void producer() {
	printf("console-producer :: productor de consolas iniciado\n");

	pthread_t tid;
	char* path;
	char* file;
	char* n;
	int nfiles;
	int process_size;
	char* dir = strdup("/home/utnso/git/tp-2022-1c-Los-o-os/kiss.test/dataset/");

	switch (TEST) {
		case BASE:
			file = strdup("BASE_");
			nfiles = 2;
			process_size = 100;
			break;
		case PLANIFICADOR:
			file = strdup("PLANI_");
			nfiles = 3;
			process_size = 100;
			break;
		case SUSPENDIDO:
			file = strdup("SUSPE_");
			nfiles = 3;
			process_size = 100;
			break;
		case INTEGRAL:
			file = strdup("INTEGRAL_");
			nfiles = 5;
			process_size = 2048;
			break;
		default:
			printf("console-producer :: tipo de productor incorrecto\n");
			return;
	}

	t_console_parameter *params;

	for (int c = 1; c <= nfiles; c++) {
		n = string_itoa(c);
		path = strdup(dir);
		string_append(&path, file);
		string_append(&path, n);

		params = malloc(sizeof(t_console_parameter));
		params->path = string_duplicate(path);
		params->program_size = process_size;

		printf("console-producer :: programa:[%s]\n", params->path);

		pthread_create(&tid, NULL, (void*)handle_console, (void*)params);
		pthread_detach(tid);

		usleep(1000000);

		free(n);
		free(path);
	}

	free(dir);
	free(file);

	printf("console-producer :: productor de consolas detenido\n");

	pthread_exit(NULL);
}


static void handle_console(void* vparams) {
	t_console_parameter* params = (t_console_parameter*)vparams;

	console_run("console.config", params->path, params->program_size);
	free(params->path);
	free(params);
}

