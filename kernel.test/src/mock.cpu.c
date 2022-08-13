/*
 * mock.cpu.c
 *
 *  Created on: 25 jul. 2022
 *      Author: utnso
 */


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "../include/mock.cpu.h"
#include "../../shared/include/kiss.structs.h"
#include "../../shared/include/kiss.serialization.h"
#include "../../shared/include/socket_handler.h"
#include "../../shared/include/kiss.error.h"
#include "../../shared/include/utils.h"
#include "../../shared/include/time_utils.h"

#include <pthread.h>
#include <semaphore.h>

#define IP_CPU "127.0.0.1"
#define PUERTO_CPU_DISPATCH "8001"
#define PUERTO_CPU_INTERRUPT "8005"


static void server_disconnected(t_socket_handler* server);
static void server_listening(t_socket_handler* server);
static void server_error(t_socket_handler* server);
static t_handshake_result kernel_handshake(t_socket_handler* handler);
static void handle_dispatch(t_socket_handler* dispatch);
static void handle_interrupt(t_socket_handler* interrupt);
static void execute(t_pcb* pcb);



t_socket_handler* dispatch_server = NULL;
t_socket_handler* interrupt_server = NULL;
bool interrupted;



int mock_cpu_run() {

	// Servidor dispatch
	printf("mock-cpu :: iniciando servidor dispatch en ip:[%s] port:[%s]\n", IP_CPU, PUERTO_CPU_DISPATCH);
	dispatch_server = start_server(IP_CPU, PUERTO_CPU_DISPATCH);
	dispatch_server->name = strdup("dispatch");

	if (dispatch_server == NULL) {
		printf("mock-cpu :: Ocurrio un error instanciando el servidor\n");
		return EXIT_FAILURE;
	}

	if (dispatch_server->error != 0) {
		printf("mock-cpu :: Ocurrio un error instanciando el servidor dispatch. error[%d] msg:[%s]\n",
				dispatch_server->error, errortostr(dispatch_server->error));

		socket_handler_destroy(dispatch_server);
		dispatch_server = NULL;
		return EXIT_FAILURE;
	}

	dispatch_server->on_client_connected = handle_dispatch;
	// asigno el metodo que invoca el server cuando ya esta listo para escuchar conexiones
	dispatch_server->on_listening = server_listening;
	// asigno el metodo que invoca el server cuando deja de ecuchar conexiones y se cierra el ciclo de espera
	dispatch_server->on_disconnected = server_disconnected;
	// asigno el metodo que invoca el server cuando ocurre un error mientras esta escuchando conexiones
	dispatch_server->on_error = server_error;

	dispatch_server->on_handshake = kernel_handshake;

	server_listen_async(dispatch_server);

	if (dispatch_server->error != 0) {
		printf("mock-cpu :: Ocurrio un error iniciando la conexion del servidor dispatch. error[%d] msg:[%s]\n",
				dispatch_server->error, errortostr(dispatch_server->error));

		socket_handler_destroy(dispatch_server);
		dispatch_server = NULL;
		return EXIT_FAILURE;
	}


	// Servidor interrupt
	printf("mock-cpu :: iniciando servidor interrupt en ip:[%s] port:[%s]\n", IP_CPU, PUERTO_CPU_INTERRUPT);
	interrupt_server = start_server(IP_CPU, PUERTO_CPU_INTERRUPT);
	interrupt_server->name = strdup("interrupt");

	if (interrupt_server == NULL) {
		printf("mock-cpu :: Ocurrio un error instanciando el servidor interrupt\n");
		return EXIT_FAILURE;
	}

	if (interrupt_server->error != 0) {
		printf("mock-cpu :: Ocurrio un error instanciando el servidor interrupt. error[%d] msg:[%s]\n",
				interrupt_server->error, errortostr(interrupt_server->error));

		socket_handler_destroy(interrupt_server);
		interrupt_server = NULL;
		return EXIT_FAILURE;
	}

	interrupt_server->on_client_connected = handle_interrupt;
	// asigno el metodo que invoca el server cuando ya esta listo para escuchar conexiones
	interrupt_server->on_listening = server_listening;
	// asigno el metodo que invoca el server cuando deja de ecuchar conexiones y se cierra el ciclo de espera
	interrupt_server->on_disconnected = server_disconnected;
	// asigno el metodo que invoca el server cuando ocurre un error mientras esta escuchando conexiones
	interrupt_server->on_error = server_error;

	interrupt_server->on_handshake = kernel_handshake;

	server_listen_async(interrupt_server);

	if (interrupt_server->error != 0) {
		printf("mock-cpu :: Ocurrio un error iniciando la conexion del servidor interrupt. error[%d] msg:[%s]\n",
				interrupt_server->error, errortostr(interrupt_server->error));

		socket_handler_destroy(interrupt_server);
		interrupt_server = NULL;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}


void mock_cpu_stop() {
	if (dispatch_server != NULL && dispatch_server->connected)
		socket_handler_disconnect(dispatch_server);

	if (interrupt_server != NULL && interrupt_server->connected)
		socket_handler_disconnect(interrupt_server);
}


static t_handshake_result kernel_handshake(t_socket_handler* handler) {

	t_handshake_result result = CONNECTION_REJECTED;
	t_package* package = recieve_message(handler);

	if (handler->error != 0) {
		printf("mock-cpu :: Ocurrio un error al recibir el mensaje del kernel. error:[%d] msg:[%s]. Conexion rechazada de socket:[%d]\n",
				handler->error, errortostr(handler->error), handler->socket);
	}
	else if (package->header != MC_PROCESS_CODE) {
		printf("mock-cpu :: Codigo:[%d] de paquete incorrecto para handshake. Conexion rechazada de socket:[%d]\n", package->header, handler->socket);
	}
	else {
		t_process_code process_code = deserialize_process_code(package->payload);

		if (process_code == PROCESS_KERNEL) {
			printf("mock-cpu :: Conexion aceptada con kernel en socket:[%d]\n", handler->socket);
			result = CONNECTION_ACCEPTED;
		}
		else {
			printf("mock-cpu :: Codigo:[%d] de proceso para handshake incorrecto. Conexion rechazada de socket:[%d]\n", process_code, handler->socket);
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


static void handle_dispatch(t_socket_handler* dispatch) {
	t_package* package;
	bool stop = false;

	socket_handler_disconnect(dispatch_server);

	while(!stop) {
		package = recieve_message(dispatch);

		if (dispatch->error != 0) {
			if (dispatch->error == SKT_CONNECTION_LOST) {
				printf("mock-cpu-dispatch :: desconectado. error:[%d] msg:[%s]\n", dispatch->error, errortostr(dispatch->error));
				stop = true;
			}
			else {
				printf("mock-cpu-dispatch :: error al recibir el mensaje del kernel. error:[%d] msg:[%s]\n", dispatch->error, errortostr(dispatch->error));
			}
		}
		else if (package->header != MC_PCB) {
			printf("mock-cpu-dispatch :: error al recibir el mensaje del kernel. encabezado:[%d] incorrecto. Se esperaba MC_PCB\n", package->header);
		}
		else {
			t_pcb* pcb = deserializar_pcb(package->payload);
			execute(pcb);

			t_buffer* buffer = serializar_pcb(pcb);
			send_message(dispatch, MC_PCB, buffer);
			buffer_destroy(buffer);

			pcb_destroy(pcb);
		}

		buffer_destroy(package->payload);
		package_destroy(package);
	}

	socket_handler_disconnect(dispatch);
	socket_handler_destroy(dispatch);
}



static void handle_interrupt(t_socket_handler* interrupt) {
	t_package* package;

	socket_handler_disconnect(interrupt_server);
	bool stop = false;

	while(!stop) {

		// ACTIVAR MUTEX

		package = recieve_message(interrupt);

		if (interrupt->error != 0) {
			if (interrupt->error == SKT_CONNECTION_LOST) {
				printf("mock-cpu-interrupt :: desconectado. error:[%d] msg:[%s]\n", interrupt->error, errortostr(interrupt->error));
				stop = true;
			}
			else {
				printf("mock-cpu-interrupt :: error de mensaje recibido. error:[%d] msg:[%s]\n", interrupt->error, errortostr(interrupt->error));
			}
		}
		else if (package->header != MC_SIGNAL) {
			printf("mock-cpu-interrupt :: encabezado:[%d] incorrecto. Se esperaba 'MC_SIGNAL'\n", package->header);
		}
		else {
			t_signal signal = deserialize_signal(package->payload);

			if (signal != SG_CPU_INTERRUPT) {
				printf("mock-cpu-interrupt :: señal:[%d] incorrecta. Se esperaba 'SG_CPU_INTERRUPT'\n", package->header);
			}
			else {
				printf("mock-cpu-interrupt :: interrupcion de cpu\n");
				interrupted = true;
			}
		}

		buffer_destroy(package->payload);
		package_destroy(package);
	}

	// DESACTIVAR MUTEX

	socket_handler_disconnect(interrupt);
	socket_handler_destroy(interrupt);
}



static void execute(t_pcb* pcb) {
	bool exit_exec = false;
	uint64_t start = get_time_stamp();
	int inst_count = list_size(pcb->instructions);

	if (inst_count <= 0) {
		printf("mock-cpu :: el pcb:[%d] no tiene instrucciones para procesar\n", pcb->id);
		return;
	}

	while (!interrupted && !exit_exec && pcb->program_counter < inst_count) {
		t_instruction* instruction = list_get(pcb->instructions, pcb->program_counter);

		// TODO: en cada funcion, hay que validar que se haya ejecutado correctemnte
		// si no es asi tenes que parar el CPU!!!!
		switch(instruction->codigo) {
			case ci_NO_OP:
				printf("mock-cpu :: pcb:[%d] execute NO_OP\n", pcb->id);
				usleep(100000);
				break;
			case ci_I_O:
				pcb->cpu_dispatch_cond = CPU_IO_SYSCALL;
				pcb->io_burst =  atoi(list_get(instruction->parametros, 0));
				printf("mock-cpu :: pcb:[%d] execute IO(%d)\n", pcb->id, pcb->io_burst);
				exit_exec = true;
				break;
			case ci_WRITE:
				printf("mock-cpu :: pcb:[%d] execute WRITE(%d, %d)\n", pcb->id, atoi(list_get(instruction->parametros, 0)), atoi(list_get(instruction->parametros, 1)));
				usleep(500000);
				break;
			case ci_COPY:
				printf("mock-cpu :: pcb:[%d] execute COPY(%d, %d)\n", pcb->id, atoi(list_get(instruction->parametros, 0)), atoi(list_get(instruction->parametros, 1)));
				usleep(500000 * 2);
				break;
			case ci_READ:
				printf("mock-cpu :: pcb:[%d] execute READ(%d)\n", pcb->id, atoi(list_get(instruction->parametros, 0)));
				usleep(500000);
				break;
			case ci_EXIT:
				printf("mock-cpu :: pcb:[%d] execute EXIT\n", pcb->id);
				pcb->cpu_dispatch_cond = CPU_EXIT_PROGRAM;
				exit_exec = true;
				break;
			default:
				printf("mock-cpu :: la instruccion:[%d] del pcb:[%d] tiene un codigo invalido\n", instruction->codigo, pcb->id);
				return;
		}

		pcb->program_counter++;

		// ACTIVAR MUTEX
		//if (inte)
		// DESACTIVAR
	}

	uint64_t end = get_time_stamp();
	uint64_t diff = end - start;
	pcb->cpu_executed_burst = diff;

	// si salio por interrupcion ...
	if (interrupted && !exit_exec) {
		pcb->cpu_dispatch_cond = CPU_INTERRUPTED_BY_KERNEL;
	}

	interrupted = false;

	printf("mock-cpu :: pcb:[%d] desalojado por '%s' executed time:[%lu]\n", pcb->id, dispatchcondtostr(pcb->cpu_dispatch_cond), (long unsigned)diff);
}




static void server_listening(t_socket_handler* server) {
	printf("mock-cpu :: Servidor '%s' preparado y escuchando ...\n", server->name);
}


// el metodo es invocado por el manejador del socket cuando el servidor se detiene
static void server_disconnected(t_socket_handler* server) {
	printf("mock-cpu :: El servidor '%s' desconectado\n", server->name);

	if (server->socket == dispatch_server->socket)
		dispatch_server = NULL;
	else if (server->socket == interrupt_server->socket)
		interrupt_server = NULL;

	socket_handler_destroy(server);
}


static void server_error(t_socket_handler* server) {
	printf("mock-cpu :: server '%s' error >> Ocurrio un error durante la ejecucion del servicio de conexiones :: error:[%d] msg:[%s]\n",
			server->name, server->error, errortostr(server->error));

	if (server->socket == dispatch_server->socket) {
		if (!server->connected) {
			dispatch_server = NULL;
			socket_handler_destroy(server);
		}
	}
	else if (server->socket == interrupt_server->socket) {
		if (!server->connected) {
			interrupt_server = NULL;
			socket_handler_destroy(server);
		}
	}
}





