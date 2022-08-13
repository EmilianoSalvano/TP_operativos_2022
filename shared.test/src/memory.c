/*
 * memory.c
 *
 *  Created on: 18 jul. 2022
 *      Author: utnso
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>
#include "../include/memory.h"
#include "../include/socket_handler.h"
#include "../include/kiss.serialization.h"
#include "../include/kiss.structs.h"


static void server_disconnected(t_socket_handler* server);
static void server_listening(t_socket_handler* server);
static void server_error(t_socket_handler* server);
static void handle_client(t_socket_handler* client);
static t_handshake_result server_cpu_handshake(t_socket_handler* server);

static void handle_cpu(t_socket_handler* client_cpu);
static void handle_kernel(t_socket_handler* client_cpu);



int memory_initialize() {


	t_socket_handler* server_cpu = start_server("127.0.0.1", "8000");

	server_cpu->on_client_connected = handle_cpu;
	// asigno el metodo que invoca el server cuando ya esta listo para escuchar conexiones
	server_cpu->on_listening = server_listening;
	// asigno el metodo que invoca el server cuando deja de ecuchar conexiones y se cierra el ciclo de espera
	server_cpu->on_disconnected = server_disconnected;
	// asigno el metodo que invoca el server cuando ocurre un error mientras esta escuchando conexiones
	server_cpu->on_error = server_error;

	server_cpu->on_handshake = server_cpu_handshake;

	server_listen_async(server_cpu);

	// hacer lo mismo para kernel




	pthread_exit(NULL);
}


static t_handshake_result server_cpu_handshake(t_socket_handler* server) {

	// envio de mensajes para determinar quien se conecto

	return CONNECTION_ACCEPTED;
}


// funcion a la que llama el servidor cuando se conecta un nuevo cliente
// Esta funcoin corre sobre un hilo secundario
// cada vez que el servidor la llama, crea un hilo distinto por cada llamado y el socket_handler tambien es distinto
static void handle_client(t_socket_handler* client) {
	/*
	if (CPU)
		handle_cpu(client)
	else if (KERNEL)
		handle_kernel(client)

	if (CPU y KERNEL conectados) {
		socket_handler_disconnect(client_cpu->server);
		socket_handler_destroy(client_cpu->server);
	}
	*/
}

static void handle_kernel(t_socket_handler* client_kernel) {
	// ..........
}

static void handle_cpu(t_socket_handler* client_cpu) {
	while(true) {

		t_package* package = recieve_message(client_cpu);
		if (client_cpu->error != 0) {
			printf("error %s", strerror(client_cpu->error));
			pthread_exit(NULL);
		}

		/*
		switch(package->header) {
		case MC_PAGINAR

		}
		*/

		if (package->header == MC_PCB) {
			t_pcb* pcb = deserializar_pcb(package->payload);

			buffer_destroy(package->payload);
			package_destroy(package);

			t_buffer* buffer = serialize_uint32t(5);
			send_message(client_cpu, MC_UINT32T_CODE, buffer);
			buffer_destroy(buffer);

			if (client_cpu->error != 0) {

			}

		}
	}
	socket_handler_disconnect(client_cpu);

}






static void server_listening(t_socket_handler* server) {
	printf("server_ready >> Servidor preparado y escuchando ... || %s (%d)\n", __FILE__, __LINE__);
	//sem_post(&sem_server_ready);
}

// el metodo es invocado por el manejador del socket cuando el servidor se detiene
static void server_disconnected(t_socket_handler* server) {
	printf("server_close >> El servidor ha terminado su ejecucion || %s (%d)\n", __FILE__, __LINE__);
}

// el metodo es invocado por el manejador de socket cuando ocurre algun error con el servidor durante el "accept"
// TODO: el handler deberia ser del nuevo socket, no del server
static void server_error(t_socket_handler* server) {
	printf("server_error >> Ocurrio un error durante la ejecucion del servicio de conexiones :: Error:[%d] %s  || %s (%d)\n",
			server->error, strerror(server->error), __FILE__, __LINE__);
}




