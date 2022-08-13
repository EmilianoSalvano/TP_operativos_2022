/*
 * console.c
 *
 *  Created on: 30 jul. 2022
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <commons/string.h>
#include <commons/config.h>

#include "../include/console.h"
#include "../include/parser.h"

#include "../../shared/include/socket_handler.h"
#include "../../shared/include/kiss.serialization.h"
#include "../../shared/include/kiss.structs.h"
#include "../../shared/include/kiss.error.h"



static t_config_consola* load_config(char* file);
static t_handshake_result kernel_handshake(t_socket_handler* handler);
static void config_consola_destroy(t_config_consola* self);
static t_config_consola* config_consola_create();


void console_run(char* config_file, char* program_file, int program_size) {

	t_config_consola* config = load_config(config_file);

	if (config == NULL) {
		printf("ejecucion de consola terminada por error de archivo de configuracion\n");
		return;
	}

	t_program* program = parse_program(program_file, program_size);

	if (program == NULL) {
		printf("ejecucion de consola terminada por error de parser\n");
		return;
	}

	printf("iniciando conexion con kernel. ip:[%s] port:[%s]\n", config->ip_kernel, config->puerto_kernel);
	t_socket_handler* handler = connect_to_server(config->ip_kernel, config->puerto_kernel);

	if (handler->error != 0) {
		printf("error de conexion con kernel. error:[%d] msg:[%s]\n", handler->error, errortostr(handler->error));
		return;
	}
	else if (kernel_handshake(handler) == CONNECTION_ACCEPTED) {

		printf("conexion aceptada con kernel. enviando programa ...\n");

		t_buffer* buffer = serialize_program(program);
		program_destroy(program);

		send_message(handler, MC_PROGRAM, buffer);
		buffer_destroy(buffer);

		if (handler->error != 0) {
			printf("error enviando el programa al kernel. error:[%d] msg:[%s]\n", handler->error, errortostr(handler->error));
		}
		else {
			printf("programa enviado. esperando respuesta de kernel ...\n");

			t_package* package = recieve_message(handler);

			if (handler->error != 0) {
				printf("error de respuesta de kernel. error:[%d] msg:[%s]\n", handler->error, errortostr(handler->error));
			}
			else if (package->header != MC_SIGNAL) {
				printf("codigo:[%d] de respuesta incorecta de kernel. se esperaba 'MC_SIGNAL'\n", package->header);
			}
			else {
				t_signal status = deserialize_signal(package->payload);

				if (status == SG_OPERATION_SUCCESS)
					printf("programa ejecutado con exito\n");
				else
					printf("error de ejecucion de programa. status:[%d]\n", status);
			}

			buffer_destroy(package->payload);
			package_destroy(package);
		}
	}

	socket_handler_disconnect(handler);
	socket_handler_destroy(handler);
	config_consola_destroy(config);

	printf("ejecucion de consola terminada\n");

}


static void config_consola_destroy(t_config_consola* self) {
	free(self->ip_kernel);
	free(self->puerto_kernel);
	free(self);
}

static t_config_consola* config_consola_create() {
	t_config_consola* self = malloc(sizeof(t_config_consola));
	self->ip_kernel = NULL;
	self->puerto_kernel = NULL;

	return self;
}

static t_config_consola* load_config(char* file) {
	t_config* file_config = config_create(file);

	t_config_consola* config_consola = config_consola_create();

	config_consola->ip_kernel = strdup(config_get_string_value(file_config, "IP_KERNEL"));
	config_consola->puerto_kernel = strdup(config_get_string_value(file_config, "PUERTO_KERNEL"));

	config_destroy(file_config);

	return config_consola;
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

