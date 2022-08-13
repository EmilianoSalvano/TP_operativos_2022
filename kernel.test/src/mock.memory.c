/*
 * mock.memory.c
 *
 *  Created on: 25 jul. 2022
 *      Author: utnso
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "../include/mock.memory.h"
#include "../../shared/include/kiss.structs.h"
#include "../../shared/include/kiss.serialization.h"
#include "../../shared/include/socket_handler.h"
#include "../../shared/include/kiss.error.h"
#include "../../shared/include/utils.h"

#define PUERTO_MEMORIA "8002"
#define IP_MEMORIA "127.0.0.1"

static void server_disconnected(t_socket_handler* server);
static void server_listening(t_socket_handler* server);
static void server_error(t_socket_handler* server);
static void handle_kernel(t_socket_handler* handler);
static t_handshake_result kernel_handshake(t_socket_handler* server);
static void get_page_table_entry(t_socket_handler* handler, int pcb_id, int process_size);
static void swapp_process(t_socket_handler* handler, int table_entry);
static void message_error(t_socket_handler* handler, t_signal signal);
static void dump_allocated_space(t_socket_handler* handler, int table_entry);


t_socket_handler* memory_server = NULL;


int mock_memory_run() {

	// servidor para conexion de kernel
	printf("mock-memory :: iniciando servidor de memoria en ip:[%s] port:[%s]\n", IP_MEMORIA, PUERTO_MEMORIA);
	memory_server = start_server(IP_MEMORIA, PUERTO_MEMORIA);

	if (memory_server == NULL) {
		printf("mock-memory :: Ocurrio un error instanciando el servidor\n");
		return EXIT_FAILURE;
	}

	if (memory_server->error != 0) {
		printf("mock-memory :: Ocurrio un error instanciando el servidor. error[%d] msg:[%s]\n",
				memory_server->error, errortostr(memory_server->error));

		socket_handler_destroy(memory_server);
		memory_server = NULL;
		return EXIT_FAILURE;
	}

	memory_server->on_client_connected = handle_kernel;
	// asigno el metodo que invoca el server cuando ya esta listo para escuchar conexiones
	memory_server->on_listening = server_listening;
	// asigno el metodo que invoca el server cuando deja de ecuchar conexiones y se cierra el ciclo de espera
	memory_server->on_disconnected = server_disconnected;
	// asigno el metodo que invoca el server cuando ocurre un error mientras esta escuchando conexiones
	memory_server->on_error = server_error;

	memory_server->on_handshake = kernel_handshake;

	server_listen_async(memory_server);

	if (memory_server->error != 0) {
		printf("mock-memory :: Ocurrio un error iniciando la conexion del servidor. error[%d] msg:[%s]\n",
				memory_server->error, errortostr(memory_server->error));

		socket_handler_destroy(memory_server);
		memory_server = NULL;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}


void mock_memory_stop() {
	if (memory_server != NULL && memory_server->connected)
		socket_handler_disconnect(memory_server);
}


static t_handshake_result kernel_handshake(t_socket_handler* handler) {

	t_handshake_result result = CONNECTION_REJECTED;
	t_package* package = recieve_message(handler);

	if (handler->error != 0) {
		printf("mock-memory :: Ocurrio un error al recibir el mensaje del kernel. error:[%d] msg:[%s]. Conexion rechazada de socket:[%d]\n",
				handler->error, errortostr(handler->error), handler->socket);
	}
	else if (package->header != MC_PROCESS_CODE) {
		printf("mock-memory :: Codigo:[%d] de paquete incorrecto para handshake. Conexion rechazada de socket:[%d]\n", package->header, handler->socket);
	}
	else {
		t_process_code process_code = deserialize_process_code(package->payload);

		if (process_code == PROCESS_KERNEL) {
			printf("mock-memory :: Conexion aceptada con kernel en socket:[%d]\n", handler->socket);
			result = CONNECTION_ACCEPTED;
		}
		else {
			printf("mock-memory :: Codigo:[%d] de proceso para handshake incorrecto. Conexion rechazada de socket:[%d]\n", process_code, handler->socket);
		}
	}

	buffer_destroy(package->payload);
	package_destroy(package);

	if (result == CONNECTION_ACCEPTED)
		send_signal(handler, MC_SIGNAL, SG_CONNECTION_ACCEPTED);
	else if (handler->error != SKT_CONNECTION_LOST)
		send_signal(handler, MC_SIGNAL, SG_CONNECTION_REJECTED);

	return result;
}


static void handle_kernel(t_socket_handler* handler) {

	socket_handler_disconnect(memory_server);
	bool connected = true;
	int table_entry;
	t_page_entry_request* request;

	while(connected) {

		t_package* package = recieve_message(handler);

		if (handler->error != 0) {
			if (handler->error == SKT_CONNECTION_LOST) {
				printf("mock-memory :: desconectado. error:[%d] msg:[%s]\n", handler->error, errortostr(handler->error));
				connected = false;
			}
			else {
				printf("mock-memory :: error al recibir el mensaje del kernel. error:[%d] msg:[%s]\n", handler->error, errortostr(handler->error));
				message_error(handler, SG_ERROR_PACKAGE);
			}
		}
		else {

			switch(package->header) {
			case MC_NUEVO_PROCESO:
				request = deserializar_page_entry_request(package->payload);
				get_page_table_entry(handler, request->pcb_id, request->process_size);
				page_entry_request_destroy(request);
				break;
			case MC_SUSPENDER_PROCESO:
				table_entry = deserialize_uint32t(package->payload);
				swapp_process(handler, table_entry);
				break;
			case MC_FINALIZAR_PROCESO:
				table_entry = deserialize_uint32t(package->payload);
				dump_allocated_space(handler, table_entry);
				break;
			default:
				printf("mock-memory :: codigo:[%d] de mensaje incorrecto para operacion de memoria\n", package->header);
				message_error(handler, SG_CODE_NOT_EXPECTED);
				continue;
			}
		}

		buffer_destroy(package->payload);
		package_destroy(package);
	}

	socket_handler_disconnect(handler);
	socket_handler_destroy(handler);

}


static void get_page_table_entry(t_socket_handler* handler, int pcb_id, int process_size) {
	int page = produce_num_in_range(1, 100);
	printf("mock-memory:: espacio reservado para pcb:[%d]. process_size:[%d] entry:[%d]\n", pcb_id, process_size, page);
	usleep(500000);

	send_signal(handler, MC_NUEVO_PROCESO, page);
}


static void swapp_process(t_socket_handler* handler, int table_entry) {
	printf("mock-memory :: proceso paginado. entrada de tabla de paginas:[%d]\n", table_entry);
	usleep(1000000);

	send_signal(handler, MC_SUSPENDER_PROCESO, SG_OPERATION_SUCCESS);
}


static void dump_allocated_space(t_socket_handler* handler, int table_entry) {
	printf("mock-memory :: espacio de memoria liberado para entrada de paginas paginas:[%d]\n", table_entry);
	usleep(500000);

	send_signal(handler, MC_SUSPENDER_PROCESO, SG_OPERATION_SUCCESS);
}


static void message_error(t_socket_handler* handler, t_signal signal) {
	printf("mock-memory :: message_error\n");
	send_signal(handler, MC_SIGNAL, signal);
}


static void server_listening(t_socket_handler* server) {
	printf("mock-memory :: Servidor preparado y escuchando ...\n");
}


// el metodo es invocado por el manejador del socket cuando el servidor se detiene
static void server_disconnected(t_socket_handler* server) {
	printf("mock-memory :: El servidor ha terminado su ejecucion\n");

	socket_handler_destroy(memory_server);
	memory_server = NULL;
}


// el metodo es invocado por el manejador de socket cuando ocurre algun error con el servidor durante el "accept"
static void server_error(t_socket_handler* server) {
	printf("mock-memory :: server error >> Ocurrio un error durante la ejecucion del servicio de conexiones :: error:[%d] msg:[%s]\n",
			server->error, errortostr(server->error));

	if (!server->connected) {
		socket_handler_destroy(memory_server);
		memory_server = NULL;
	}
}

