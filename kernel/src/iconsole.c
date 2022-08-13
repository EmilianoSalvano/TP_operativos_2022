/*
 * console.c
 *
 *  Created on: 10 jul. 2022
 *      Author: utnso
 */


#include "../include/iconsole.h"

#include <stdlib.h>
#include <stdbool.h>
#include <commons/log.h>

#include "../include/planner.h"

#include "../../shared/include/kiss.serialization.h"
#include "../../shared/include/kiss.error.h"

#define LOCAL_IP "127.0.0.1"

t_kernel* kernel;
t_log* logger;
t_console_module* console;


static t_handshake_result server_handshake(t_socket_handler* client);
static void console_handler(t_socket_handler* handler);

static void server_listening(t_socket_handler* server);
static void server_disconnected(t_socket_handler* server);
static void server_error(t_socket_handler* server);



/************************************** Public *******************************************/

t_console_module* console_module_create() {
	console = malloc(sizeof(t_console_module));
	console->server = NULL;

	return console;
}

void console_module_destroy() {
	if (console == NULL) {
		log_warning(logger, "No se puede destruir el modulo de administracion de consolas porque aun no fue creado");
		return;
	}

	if (console->server->connected) {
		log_warning(logger, "No se puede destruir el modulo de administracion de consolas. El servidor sigue activo");
		return;
	}

	if (console->server != NULL)
		socket_handler_destroy(console->server);

	free(console);
}


int console_module_run() {
	if (console == NULL) {
		log_warning(logger, "No se puede iniciar el modulo de administracion de consolas porque aun no fue creado");
		return EXIT_FAILURE;
	}

	log_info(logger, "Iniciando modulo de administracion de consolas ip:[%s] port:[%s] ...", LOCAL_IP, kernel->config->puerto_escucha);

	console->server = start_server(LOCAL_IP, kernel->config->puerto_escucha);

	if (console->server->error != 0) {
		log_error(logger, "Ocurrio un error al crear el servidor de consolas. Error:[%d] Msg:[%s]",
				console->server->error, errortostr(console->server->error));

		return EXIT_FAILURE;
	}

	log_info(logger, "Servidor de consolas iniciado en ip:[%s] port:[%s] socket:[%d]",
			console->server->ip, console->server->port, console->server->socket);

	console->server->on_handshake = server_handshake;
	console->server->on_client_connected = console_handler;
	console->server->on_error = server_error;
	console->server->on_disconnected = server_disconnected;
	console->server->on_listening = server_listening;

	server_listen_async(console->server);

	if (console->server->error != 0) {
		log_error(logger, "Ocurrio un error al iniciar el servidor de consolas. Error:[%d] Msg:[%s]",
				console->server->error, errortostr(console->server->error));

		socket_handler_destroy(console->server);
		console->server = NULL;
		return EXIT_FAILURE;
	}

	// el inicio depende de lo que diga el servidor
	log_info(logger, "Modulo de administracion de consolas iniciado :)");
	return EXIT_SUCCESS;
}



int console_module_stop() {

	if (console != NULL && console->server != NULL)
		socket_handler_disconnect(console->server);

	log_info(logger, "Modulo de administracion de consolas detenido");

	return EXIT_SUCCESS;
}



void console_terminate(t_socket_handler* client, t_signal status) {
	send_signal(client, MC_SIGNAL, status);
	log_info(logger, "Consola terminada, socket:[%d]", client->socket);

	socket_handler_disconnect(client);
	socket_handler_destroy(client);
}



/************************************** Private *******************************************/

// el metodo es invocado por el manejador del socket cuando el servidor esta listo para recibir conexiones
static void server_listening(t_socket_handler* server) {
	log_info(logger, "Servidor de consolas preparado y escuchando ...");
}


// el metodo es invocado por el manejador del socket cuando el servidor se detiene
static void server_disconnected(t_socket_handler* server) {
	log_info(logger, "Servidor de consolas desconectado");
}


static void server_error(t_socket_handler* server) {
	log_error(logger, "Ocurrio un error durante la ejecucion del servidor de consolas :: error:[%d] msg:[%s]", server->error, errortostr(server->error));
	kernel->error();
}


static t_handshake_result server_handshake(t_socket_handler* handler) {

	t_handshake_result result = CONNECTION_REJECTED;

	log_info(logger, "Procesando handshake con consola ...");

	t_package* package = recieve_message(handler);

	if (handler->error != 0) {
		log_error(logger, "error al recibir mensaje para operacion de handshake de consola. error[%d] msg:[%s]", handler->error, errortostr(handler->error));
		log_warning(logger, "conexion con consola rechazada");
	}
	else if (package->header != MC_PROCESS_CODE) {
		log_warning(logger, "Codigo:[%d] de paquete desconocido para operacion de handshake de consola. Conexion rechazada", package->header);
		log_warning(logger, "conexion con consola rechazada");
	}
	else {
		t_process_code process_code = deserialize_process_code(package->payload);

		if (process_code == PROCESS_CONSOLE) {
			log_info(logger, "Conexion con consola:[%d] aceptada", handler->socket);
			result = CONNECTION_ACCEPTED;
		}
		else {
			log_warning(logger, "Codigo de proceso:[%d] incorrecto para operacion de handshake de consola. Conexion rechazada", process_code);
			send_signal(handler, MC_SIGNAL, SG_CONNECTION_REJECTED);
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


static void console_handler(t_socket_handler* client) {

	log_info(logger, "Nueva consola conectada con socket:[%d] ip:[%s]", client->socket, client->ip);

	t_package* package = recieve_message(client);

	if (client->error != 0) {
		log_error(logger, "Ocurrio un error al recibir el paquete de consola socket:[%d] ip:[%s]. error:[%d] msg:[%s]",
				client->socket, client->ip, client->error, errortostr(client->error));

		console_terminate(client, SG_OPERATION_FAILURE);
	}
	else if (package->header != MC_PROGRAM) {
		log_error(logger, "Codigo de mensaje:[%d] incorrecto recibido de consola socket:[%d] ip:[%s]", package->header, client->socket, client->ip);

		console_terminate(client, SG_OPERATION_FAILURE);
	}
	else {
		t_program* program = deserialize_program(package->payload);

		if (program->instructions->elements_count == 0) {
			log_error(logger, "La lista de instrucciones recibida de consola socket:[%d] ip:[%s] esta vacia", client->socket, client->ip);
			program_destroy(program);

			console_terminate(client, SG_OPERATION_FAILURE);
		}
		else {
			ltp_new_process(client, program);
			program_destroy(program);
		}
	}

	buffer_destroy(package->payload);
	package_destroy(package);
}

