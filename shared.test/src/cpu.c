/*
 * cpu.c
 *
 *  Created on: 19 jul. 2022
 *      Author: utnso
 */


#include "../include/cpu.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>

#include "../include/socket_handler.h"
#include "../include/kiss.structs.h"
#include "../include/kiss.serialization.h"



static void server_dispatch_disconnected(t_socket_handler* server);
static void server_dispatch_listening(t_socket_handler* server);
static void server_dispatch_error(t_socket_handler* server);
static t_handshake_result server_dispatch_handshake(t_socket_handler* server);
static void handle_dispatch(t_socket_handler* dispatch);

static void server_interrupt_disconnected(t_socket_handler* server);
static void server_interrupt_listening(t_socket_handler* server);
static void server_interrupt_error(t_socket_handler* server);
static t_handshake_result server_interrupt_handshake(t_socket_handler* server);
static void handle_interrupt(t_socket_handler* interrupt);

t_socket_handler* server_dispatch;
t_socket_handler* server_interrupt;
bool interrupted = false;
//sem_t sem_dispatch;



// main
void cpu_run() {

	//sem_init(&sem_dispatch, 0, 1);

	// servidor para conexion con kernel-dispatch
	server_dispatch = start_server("127.0.0.1", "8000");

	server_dispatch->on_client_connected = handle_dispatch;
	// asigno el metodo que invoca el server cuando ya esta listo para escuchar conexiones
	server_dispatch->on_listening = server_dispatch_listening;
	// asigno el metodo que invoca el server cuando deja de ecuchar conexiones y se cierra el ciclo de espera
	server_dispatch->on_disconnected = server_dispatch_disconnected;
	// asigno el metodo que invoca el server cuando ocurre un error mientras esta escuchando conexiones
	server_dispatch->on_error = server_dispatch_error;

	server_dispatch->on_handshake = server_dispatch_handshake;

	server_listen_async(server_dispatch);


	// servidor para conexion con kernel-interrupt
	server_interrupt = start_server("127.0.0.1", "8000");

	server_interrupt->on_client_connected = handle_interrupt;
	// asigno el metodo que invoca el server cuando ya esta listo para escuchar conexiones
	server_interrupt->on_listening = server_interrupt_listening;
	// asigno el metodo que invoca el server cuando deja de ecuchar conexiones y se cierra el ciclo de espera
	server_interrupt->on_disconnected = server_interrupt_disconnected;
	// asigno el metodo que invoca el server cuando ocurre un error mientras esta escuchando conexiones
	server_interrupt->on_error = server_interrupt_error;

	server_interrupt->on_handshake = server_interrupt_handshake;

	server_listen_async(server_interrupt);


	// conexion con memoria
	t_socket_handler* socket_memoria = connect_to_server("196.168.1.26", "8002");


	pthread_exit(NULL);
}





void send_dispatch_pcb(t_pcb* pcb, t_socket_handler* dispatch) {

	t_buffer* buffer = serializar_pcb(pcb);
	send_message(dispatch, MC_PCB, buffer);

	if (dispatch->error != 0) {
		// se rompio
	}

	//sem_post(&sem_dispatch);
}


static t_handshake_result server_dispatch_handshake(t_socket_handler* server) {

	// controlar que se esta conectando el kernel y que sea dispatch

	return CONNECTION_ACCEPTED;
}


// funcion a la que llama el servidor cuando se conecta un nuevo cliente
// Esta funcoin corre sobre un hilo secundario
// cada vez que el servidor la llama, crea un hilo distinto por cada llamado y el socket_handler tambien es distinto
static void handle_dispatch(t_socket_handler* dispatch) {

	socket_handler_disconnect(server_dispatch);
	socket_handler_destroy(server_dispatch);

	while(true) {

		// sem_wait(&sem_dispatch);

		// bloqueante
		t_package* package = recieve_message(dispatch);

		if (dispatch->error != 0) {
			printf("error %s", strerror(dispatch->error));
			pthread_exit(NULL);
		}


		if (package->header == MC_PCB) {
			t_pcb* pcb = deserializar_pcb(package->payload);

			buffer_destroy(package->payload);
			package_destroy(package);


			// ejecutar rutina de ejecucion de instrucciones
			// o habilitar el semaforo de ejecucion de instrucciones
		}
	}

	socket_handler_disconnect(dispatch);
	socket_handler_destroy(dispatch);

	// FIN DEL HILO
}


static void server_dispatch_listening(t_socket_handler* server) {
	printf("server_ready >> Servidor preparado y escuchando ... || %s (%d)\n", __FILE__, __LINE__);
	//sem_post(&sem_server_ready);
}

// el metodo es invocado por el manejador del socket cuando el servidor se detiene
static void server_dispatch_disconnected(t_socket_handler* server) {
	printf("server_close >> El servidor ha terminado su ejecucion || %s (%d)\n", __FILE__, __LINE__);
}

// el metodo es invocado por el manejador de socket cuando ocurre algun error con el servidor durante el "accept"
// TODO: el handler deberia ser del nuevo socket, no del server
static void server_dispatch_error(t_socket_handler* server) {
	printf("server_error >> Ocurrio un error durante la ejecucion del servicio de conexiones :: Error:[%d] %s  || %s (%d)\n",
			server->error, strerror(server->error), __FILE__, __LINE__);
}




static t_handshake_result server_interrupt_handshake(t_socket_handler* server) {

	// controlar que se esta conectando el kernel y que sea dispatch

	return CONNECTION_ACCEPTED;
}


static void handle_interrupt(t_socket_handler* interrupt) {

	socket_handler_disconnect(server_interrupt);
	socket_handler_destroy(server_interrupt);

	while(true) {

		// bloqueante
		t_package* package = recieve_message(interrupt);

		if (interrupt->error != 0) {
			printf("error %s", strerror(interrupt->error));
			pthread_exit(NULL);
		}


		if (package->header == MC_SIGNAL) {
			t_signal signal = deserialize_signal(package->payload);

			buffer_destroy(package->payload);
			package_destroy(interrupt);

			if (signal == SG_CPU_INTERRUPT) {
				interrupted = true;
			}
		}
	}

	socket_handler_disconnect(interrupt);
	socket_handler_destroy(interrupt);

	// FIN DEL HILO
}


static void server_interrupt_listening(t_socket_handler* server) {
	printf("server_ready >> Servidor preparado y escuchando ... || %s (%d)\n", __FILE__, __LINE__);
	//sem_post(&sem_server_ready);
}

// el metodo es invocado por el manejador del socket cuando el servidor se detiene
static void server_interrupt_disconnected(t_socket_handler* server) {
	printf("server_close >> El servidor ha terminado su ejecucion || %s (%d)\n", __FILE__, __LINE__);
}

// el metodo es invocado por el manejador de socket cuando ocurre algun error con el servidor durante el "accept"
// TODO: el handler deberia ser del nuevo socket, no del server
static void server_interrupt_error(t_socket_handler* server) {
	printf("server_error >> Ocurrio un error durante la ejecucion del servicio de conexiones :: Error:[%d] %s  || %s (%d)\n",
			server->error, strerror(server->error), __FILE__, __LINE__);
}


