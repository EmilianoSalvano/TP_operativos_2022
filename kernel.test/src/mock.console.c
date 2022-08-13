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

#include "../include/mock.console.h"
#include "../../shared/include/kiss.structs.h"
#include "../../shared/include/kiss.serialization.h"
#include "../../shared/include/socket_handler.h"
#include "../../shared/include/kiss.error.h"
#include "../../shared/include/utils.h"

#define PORT_CONSOLE_SERVER "8000"
#define IP_CONSOLE_SERVER "127.0.0.1"

bool stop_producer;

static void random_producer(void* vproducer);
static void test_producer(void* vproducer);
static void handle_console();
static t_handshake_result kernel_handshake(t_socket_handler* handler);

static t_program* get_program(char* file, int process_size);
static void get_instructions(char* line, t_list* list);


int mock_console_run(t_console_producer producer) {
	printf("mock-console :: iniciando productor de consolas ...\n");

	void (*func_producer)(void*);

	switch (producer) {
		case RANDOM:
			func_producer = random_producer;
			break;
		case BASE:
			func_producer = test_producer;
			break;
		case PLANIFICADOR:
			func_producer = test_producer;
			break;
		case SUSPENDIDO:
			func_producer = test_producer;
			break;
		case INTEGRAL:
			func_producer = test_producer;
			break;
		default:
			printf("mock-console :: tipo de productor incorrecto\n");
			return EXIT_FAILURE;
	}

	pthread_t tid;
	if (pthread_create(&tid, NULL, (void*)func_producer, (void*)producer) != 0) {
		printf("mock-console :: Error al crear el hilo del productor de consolas\n");
		return EXIT_FAILURE;
	}

	if (pthread_detach(tid) != 0) {
		printf("mock-console :: Error al crear el hilo del productor de consolas\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}


void mock_console_stop() {
	stop_producer = true;
}


static void random_producer(void* vproducer) {
	printf("mock-console :: productor de consolas iniciado\n");

	stop_producer = false;
	pthread_t tid;
	int interval;

	while (!stop_producer) {
		t_program* program = produce_program();
		pthread_create(&tid, NULL, (void*)handle_console, (void*)program);
		pthread_detach(tid);

		interval = 1000000;
		usleep(interval);
	}

	printf("mock-console :: productor de consolas detenido\n");

	pthread_exit(NULL);
}


static void test_producer(void* vproducer) {
	printf("mock-console :: productor de consolas iniciado\n");

	pthread_t tid;
	char* path;
	char* file;
	char* n;
	int nfiles;
	int process_size;
	char* dir = strdup("/home/utnso/git/tp-2022-1c-Los-o-os/kernel.test/dataset/");

	t_console_producer producer = (t_console_producer)vproducer;

	switch (producer) {
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
			printf("mock-console :: tipo de productor incorrecto\n");
			return;
	}

	t_list* programas = list_create();

	for (int c = 1; c <= nfiles; c++) {
		n = string_itoa(c);
		path = strdup(dir);
		string_append(&path, file);
		string_append(&path, n);

		list_add(programas, get_program(path, process_size));

		free(n);
		free(path);
	}

	free(dir);
	free(file);

	//char* p;
	int i = 0;
	while (!stop_producer && i < list_size(programas)) {
		/*
		p = program_to_string(list_get(programas, i));
		printf("%s\n", p);
		free(p);
		*/

		pthread_create(&tid, NULL, (void*)handle_console, list_get(programas, i));
		pthread_detach(tid);

		usleep(500000);
		i++;
	}

	list_destroy(programas);
	printf("mock-console :: productor de consolas detenido\n");

	pthread_exit(NULL);
}




static void handle_console(void* vprogram) {

	t_program* program = (t_program*)vprogram;

	unsigned int id = process_get_thread_id();
	printf("mock-console :: console id:[%d] nueva consola\n", id);
	printf("mock-console :: console id:[%d] conectada a ip:[%s] port:[%s]\n", id, IP_CONSOLE_SERVER, PORT_CONSOLE_SERVER);

	t_socket_handler* handler = connect_to_server(IP_CONSOLE_SERVER, PORT_CONSOLE_SERVER);

	if (handler->error != 0) {
		printf("mock-console :: Error al conectar al kernel. error:[%d] msg:[%s]\n", handler->error, errortostr(handler->error));
		return;
	}
	else if (kernel_handshake(handler) == CONNECTION_ACCEPTED) {

		t_buffer* buffer = serialize_program(program);
		program_destroy(program);

		send_message(handler, MC_PROGRAM, buffer);
		buffer_destroy(buffer);

		if (handler->error != 0) {
			printf("mock-console :: console id:[%d] error enviando el programa al kernel. error:[%d] msg:[%s]\n",
					id, handler->error, errortostr(handler->error));
		}
		else {
			t_package* package = recieve_message(handler);

			if (handler->error != 0) {
				printf("mock-console :: console id:[%d] error al recibir la respuesta del kernel. error:[%d] msg:[%s]\n",
						id, handler->error, errortostr(handler->error));
			}
			else if (package->header != MC_SIGNAL) {
				printf("mock-console :: console id:[%d] codigo:[%d] de paquete incorrecto\n", id, package->header);
			}
			else {
				t_signal status = deserialize_signal(package->payload);

				if (status == SG_OPERATION_SUCCESS)
					printf("mock-console :: console id:[%d] finalizada\n", id);
				else
					printf("mock-console :: console id:[%d] error de proceso\n", id);
			}

			buffer_destroy(package->payload);
			package_destroy(package);
		}
	}

	socket_handler_disconnect(handler);
	socket_handler_destroy(handler);
}


static t_handshake_result kernel_handshake(t_socket_handler* handler) {

	t_handshake_result result = CONNECTION_REJECTED;

	send_signal(handler, MC_PROCESS_CODE, PROCESS_CONSOLE);

	if (handler->error != 0) {
		printf("mock-console :: error enviando codigo de proceso para handshake con kernel. error:[%d] msg:[%s]\n",
				handler->error, errortostr(handler->error));

		return CONNECTION_REJECTED;
	}

	t_package* package = recieve_message(handler);

	if (handler->error != 0) {
		printf("mock-console :: error al recibir respuesta para handshake con kernel. error:[%d] msg:[%s]\n",
				handler->error, errortostr(handler->error));
	}
	else if (package->header != MC_SIGNAL) {
		printf("mock-console :: error en operacion de handshake con kernel. Codigo de header:[%d] incorrecto\n", package->header);
	}
	else {
		t_signal signal = deserialize_signal(package->payload);

		if (signal == SG_CONNECTION_ACCEPTED)
			result = CONNECTION_ACCEPTED;
	}

	buffer_destroy(package->payload);
	package_destroy(package);

	return result;
}



static void get_instructions(char* line, t_list* list) {
	t_instruction* inst;

	char** parsed = string_split(line, " ");

	if (strcmp(parsed[0], "NO_OP") == 0) {
		int c = atoi(parsed[1]);

		for (int i = 0; i < c; i++)
			list_add(list, instruccion_create(ci_NO_OP, false));
	}
	else if (strcmp(parsed[0], "I/O") == 0) {
		inst = instruccion_create(ci_I_O, false);
		list_add(inst->parametros, strdup(parsed[1]));
		list_add(list, inst);
	}
	else if (strcmp(parsed[0], "WRITE") == 0) {
		inst = instruccion_create(ci_WRITE, false);
		list_add(inst->parametros, strdup(parsed[1]));
		list_add(inst->parametros, strdup(parsed[2]));
		list_add(list, inst);
	}
	else if (strcmp(parsed[0], "COPY") == 0) {
		inst = instruccion_create(ci_COPY, false);
		list_add(inst->parametros, strdup(parsed[1]));
		list_add(inst->parametros, strdup(parsed[2]));
		list_add(list, inst);
	}
	else if (strcmp(parsed[0], "READ") == 0) {
		inst = instruccion_create(ci_READ, false);
		list_add(inst->parametros, strdup(parsed[1]));
		list_add(list, inst);
	}
	else if (strcmp(parsed[0], "EXIT") == 0) {
		inst = instruccion_create(ci_EXIT, false);
		list_add(list, inst);
	}

	char* v;
	int i = 0;
	while((v = parsed[i]) != NULL) {
		free(v);
		i++;
	}

	free(parsed);
}

static t_program* get_program(char* file, int process_size) {
	FILE *fp = fopen(file, "r");

	if(fp == NULL) {
		printf("No se puede abrir el archvivo. error:[%d] msg:[%s]", errno, errortostr(errno));
		return NULL;
	}

	t_program* program = program_create(list_create());
	program->size = process_size;

	char* line = NULL;
	size_t len = 0;

	while(getline(&line, &len, fp) != -1) {
		if (len > 0)
			get_instructions(line, program->instructions);

		//free(line);
	}

	fclose(fp);
	free(line);

	return program;
}
