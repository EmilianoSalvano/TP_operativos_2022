/*
 * memory.c
 *
 *  Created on: 14 jul. 2022
 *      Author: utnso
 */

#include "../include/imemory.h"

#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/log.h>

#include "../../shared/include/kiss.structs.h"
#include "../../shared/include/kiss.serialization.h"
#include "../../shared/include/socket_handler.h"
#include "../../shared/include/kiss.error.h"

#include "../include/imemory.h"


// TODO: tratar la desconexion de los modulos!!!!

t_kernel* kernel;
t_log* logger;
t_memory_module* memory;


// *********** Declaracion de meotodos privados *********** /
static int memory_connect();
static t_handshake_result memory_handshake(t_socket_handler* handler);



// ********************************* PUBLIC ************************************************* //

t_memory_module* memory_module_create() {
	memory = malloc(sizeof(t_memory_module));
	memory->socket_handler = NULL;
	pthread_mutex_init(&(memory->mutex_access), NULL);

	return memory;
}


void memory_module_destroy() {
	if (memory == NULL)
		return;

	if (memory->socket_handler != NULL)
		socket_handler_destroy(memory->socket_handler);

	pthread_mutex_destroy(&(memory->mutex_access));

	free(memory);
}


int memory_module_run() {

	if (memory == NULL) {
		log_info(logger, "No se puede iniciar la interface al modulo de memoria porque aun no fue creada");
		return EXIT_FAILURE;
	}

	log_info(logger, "Iniciando interface al modulo de memoria ...");

	if (memory_connect() != 0) {
		log_info(logger, "La interface al modulo de memoria no pudo ser iniciada");
		return EXIT_FAILURE;
	}

	log_info(logger, "Interface al modulo de memoria establecida");
	return EXIT_SUCCESS;
}


int memory_module_stop() {
	if (memory == NULL) {
		log_info(logger, "No se puede destruir la interface al modulo de memoria porque aun no fue creada");
		return EXIT_FAILURE;
	}

	if (memory->socket_handler != NULL)
		socket_handler_disconnect(memory->socket_handler);

	return EXIT_SUCCESS;
}


// inicializa la tabla de paginas
int get_entry_page_table(t_pcb* pcb) {

	int page = -1;

	pthread_mutex_lock(&memory->mutex_access);

	if (kernel->halt) {
		pthread_mutex_unlock(&memory->mutex_access);
		return page;
	}

	t_page_entry_request* request = page_entry_request_create();
	request->pcb_id = pcb->id;
	request->process_size = pcb->process_size;
	t_buffer* buffer = serializar_page_entry_request(request);

	send_message(memory->socket_handler, MC_NUEVO_PROCESO, buffer);
	page_entry_request_destroy(request);
	buffer_destroy(buffer);

	if (memory->socket_handler->error != 0) {
		log_info(logger, "pcb:[%d] get-page-num :: error al solicitar la entrada de pagina a memoria. error:[%d] msg:[%s]",
				pcb->id, memory->socket_handler->error, errortostr(memory->socket_handler->error));

		if (memory->socket_handler->error == SKT_CONNECTION_LOST)
			kernel->error();
	}
	else {
		t_package* package = recieve_message(memory->socket_handler);

		if (memory->socket_handler->error != 0) {
			log_info(logger, "pcb:[%d] get-page-num :: error al recibir la entrada de pagina de memoria. error:[%d] msg:[%s]",
					pcb->id, memory->socket_handler->error, errortostr(memory->socket_handler->error));

			if (memory->socket_handler->error == SKT_CONNECTION_LOST)
				kernel->error();
		}
		else if (package->header != MC_NUEVO_PROCESO) {
			log_info(logger, "pcb:[%d] get-page-num :: error al recibir la entrada de pagina de memoria. Codigo de header:[%d] incorrecto",
					pcb->id, package->header);
		}
		else {
			page = deserialize_uint32t(package->payload);
		}

		buffer_destroy(package->payload);
		package_destroy(package);
	}

	pthread_mutex_unlock(&memory->mutex_access);

	return page;
}


int swapp_process(t_pcb* pcb) {

	int result = EXIT_FAILURE;

	pthread_mutex_lock(&memory->mutex_access);

	if (kernel->halt) {
		pthread_mutex_unlock(&memory->mutex_access);
		return EXIT_FAILURE;
	}

	send_signal(memory->socket_handler, MC_SUSPENDER_PROCESO, pcb->tabla_paginas);

	if (memory->socket_handler->error != 0) {
		log_info(logger, "pcb:[%d] swap :: error al solicitar la entrada de pagina a memoria. error:[%d] msg:[%s]",
				pcb->id, memory->socket_handler->error, errortostr(memory->socket_handler->error));

		if (memory->socket_handler->error == SKT_CONNECTION_LOST)
			kernel->error();
	}
	else {
		t_package* package = recieve_message(memory->socket_handler);

		if (memory->socket_handler->error != 0) {
			log_info(logger, "pcb:[%d] swap :: error al recibir la respuesta de paginacion. error:[%d] msg:[%s]",
					pcb->id, memory->socket_handler->error, errortostr(memory->socket_handler->error));

			if (memory->socket_handler->error == SKT_CONNECTION_LOST)
				kernel->error();
		}
		else if (package->header != MC_SUSPENDER_PROCESO) {
			log_info(logger, "pcb:[%d] swap :: error al recibir la respuesta de paginacion. Codigo de header:[%d] incorrecto", pcb->id, package->header);
		}
		else {
			t_signal operation = deserialize_signal(package->payload);

			if (operation != SG_OPERATION_SUCCESS) {
				log_info(logger, "pcb:[%d] swap :: el modulo de memoria no pudo procesar el intercambio", pcb->id);
			}
			else {
				result = EXIT_SUCCESS;
			}
		}

		buffer_destroy(package->payload);
		package_destroy(package);
	}

	pthread_mutex_unlock(&memory->mutex_access);

	return result;
}


int free_allocated_memory(t_pcb* pcb) {
	int result = EXIT_FAILURE;

	pthread_mutex_lock(&memory->mutex_access);

	if (kernel->halt) {
		pthread_mutex_unlock(&memory->mutex_access);
		return EXIT_FAILURE;
	}

	send_signal(memory->socket_handler, MC_FINALIZAR_PROCESO, pcb->tabla_paginas);

	if (memory->socket_handler->error != 0) {
		log_info(logger, "free_mem :: Error al solicitar la descarga de espacio de memoria. error:[%d] msg:[%s]",
				memory->socket_handler->error, errortostr(memory->socket_handler->error));

		if (memory->socket_handler->error == SKT_CONNECTION_LOST)
			kernel->error();
	}
	else {
		t_package* package = recieve_message(memory->socket_handler);

		if (memory->socket_handler->error != 0) {
			log_info(logger, "free_mem :: error al recibir la respuesta para descarga de memoria. error:[%d] msg:[%s]",
					memory->socket_handler->error, errortostr(memory->socket_handler->error));

			if (memory->socket_handler->error == SKT_CONNECTION_LOST)
				kernel->error();
		}
		else if (package->header != MC_FINALIZAR_PROCESO) {
			log_info(logger, "free_mem :: error al recibir la respuesta de descarga de memoria. Codigo de header:[%d] incorrecto", package->header);
		}
		else {
			t_signal operation = deserialize_signal(package->payload);

			if (operation != SG_OPERATION_SUCCESS) {
				log_info(logger, "free_mem :: el modulo de memoria no pudo procesar la descarga de memoria para el pcb:[%d]", pcb->id);
			}
			else {
				result = EXIT_SUCCESS;
			}
		}

		buffer_destroy(package->payload);
		package_destroy(package);
	}

	pthread_mutex_unlock(&memory->mutex_access);

	return result;
}


// ********************************* PRIVATE ************************************************ //

static int memory_connect() {
	log_info(logger, "Conectando interface al modulo de memoria ...");

	// puerto interrupt
	memory->socket_handler = connect_to_server(kernel->config->ip_memoria, kernel->config->puerto_memoria);

	if (memory->socket_handler->error != 0) {
		log_error(logger, "No se pudo establecer conexion con el modulo de memoria. ip:[%s] port:[%s]. error:[%d] msg:[%s]",
				kernel->config->ip_memoria, kernel->config->puerto_memoria,
				memory->socket_handler->error, errortostr(memory->socket_handler->error));

		return EXIT_FAILURE;
	}

	if (memory_handshake(memory->socket_handler) != CONNECTION_ACCEPTED) {
		log_warning(logger, "Conexion rechazada por el modulo de memoria");
		return EXIT_FAILURE;
	}

	log_info(logger, "Interface al modulo de memoria conectada :)");

	return EXIT_SUCCESS;
}


static t_handshake_result memory_handshake(t_socket_handler* handler) {

	send_signal(handler, MC_PROCESS_CODE, PROCESS_KERNEL);

	if (handler->error != 0) {
		log_error(logger, "Error enviando codigo de proceso para handshake con memoria. error:[%d] msg:[%s]", handler->error, errortostr(handler->error));
		return CONNECTION_REJECTED;
	}


	t_handshake_result result = CONNECTION_REJECTED;
	t_package* package = recieve_message(handler);

	if (handler->error != 0) {
		log_error(logger, "Error respuesta para handshake con memoria. error:[%d] msg:[%s]", handler->error, errortostr(handler->error));
	}
	else if (package->header != MC_SIGNAL) {
		log_info(logger, "Error en operacion de handshake con memoria. Codigo de header:[%d] incorrecto", package->header);
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

