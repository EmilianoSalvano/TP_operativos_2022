/*
 * cpu.c
 *
 *  Created on: 31 jul. 2022
 *      Author: utnso
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/string.h>


#include "../include/cpu.h"
#include "../include/cpu_global.h"
#include "../include/cicloInstruccion.h"
#include "../include/tlb.h"
#include "../../shared/include/kiss.structs.h"
#include "../../shared/include/kiss.serialization.h"
#include "../../shared/include/socket_handler.h"
#include "../../shared/include/kiss.error.h"
#include "../../shared/include/utils.h"
#include "../../shared/include/time_utils.h"


#define IP_LOCAL "127.0.0.1"
#define ALGORITMO_FIFO "FIFO"
#define ALGORITMO_LRU "LRU"


static void server_disconnected(t_socket_handler* server);
static void server_listening(t_socket_handler* server);
static void server_error(t_socket_handler* server);
static t_handshake_result kernel_handshake(t_socket_handler* handler);
static t_handshake_result memoria_handshake(t_socket_handler* handler);
static void handle_dispatch(t_socket_handler* dispatch);
static void handle_interrupt(t_socket_handler* interrupt);
//static void execute(t_pcb* pcb);


t_socket_handler* dispatch_server = NULL;
t_socket_handler* interrupt_server = NULL;
t_log* logger_cpu;


void cpu_iniciar(t_log_level log_level, char* cfg) {

	logger_cpu = log_create("cpu.log", "cpu", true, log_level);
	config_cpu = create_configuration(cfg);

	sem_init(&sem_memory_request, 0, 1);

	// conexion a memoria
	memoria = connect_to_server(config_cpu->ip_memoria, config_cpu->puerto_memoria);

	if (memoria->error != 0) {
		log_error(logger_cpu, "No se pudo establecer conexion con el modulo de memoria. error:[%d] msg:[%s]",
				memoria->error, errortostr(memoria->error));

		return ;
	}

	if (memoria_handshake(memoria) != CONNECTION_ACCEPTED) {
		log_error(logger_cpu, "Conexion rechazada por el modulo de memoria\n");
		return;
	}

	log_info(logger_cpu, "conexion con memoria establecida");


	// Servidor dispatch
	log_info(logger_cpu, "iniciando servidor dispatch en ip:[%s] port:[%s]", IP_LOCAL, config_cpu->puerto_escucha_dispath);
	dispatch_server = start_server(IP_LOCAL, config_cpu->puerto_escucha_dispath);
	dispatch_server->name = strdup("dispatch");

	if (dispatch_server == NULL) {
		log_error(logger_cpu, "Ocurrio un error instanciando el servidor\n");
		return;
	}

	if (dispatch_server->error != 0) {
		log_error(logger_cpu, "Ocurrio un error instanciando el servidor dispatch. error[%d] msg:[%s]",
				dispatch_server->error, errortostr(dispatch_server->error));

		socket_handler_destroy(dispatch_server);
		dispatch_server = NULL;
		return ;
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
		log_error(logger_cpu, "Ocurrio un error iniciando la conexion del servidor dispatch. error[%d] msg:[%s]",
				dispatch_server->error, errortostr(dispatch_server->error));

		socket_handler_destroy(dispatch_server);
		dispatch_server = NULL;
		return ;
	}


	// Servidor interrupt
	log_info(logger_cpu, "iniciando servidor interrupt en ip:[%s] port:[%s]", IP_LOCAL, config_cpu->puerto_escucha_interrupt);
	interrupt_server = start_server(IP_LOCAL, config_cpu->puerto_escucha_interrupt);
	interrupt_server->name = strdup("interrupt");

	if (interrupt_server == NULL) {
		log_error(logger_cpu, "Ocurrio un error instanciando el servidor interrupt\n");
		return;
	}

	if (interrupt_server->error != 0) {
		log_error(logger_cpu, "Ocurrio un error instanciando el servidor interrupt. error[%d] msg:[%s]",
				interrupt_server->error, errortostr(interrupt_server->error));

		socket_handler_destroy(interrupt_server);
		interrupt_server = NULL;
		return ;
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
		log_error(logger_cpu, "Ocurrio un error iniciando la conexion del servidor interrupt. error[%d] msg:[%s]",
				interrupt_server->error, errortostr(interrupt_server->error));

		socket_handler_destroy(interrupt_server);
		interrupt_server = NULL;
		return ;
	}


	hay_interrupcion = 0;
	t_setup_memoria* setup_memoria = queue_pop(memoria->data);

	t_alg_reemplazo algoritmo;
	if (string_equals_ignore_case(config_cpu->algoritmo_reemplazo_tlb, ALGORITMO_FIFO))
		algoritmo = AR_FIFO;
	else if (string_equals_ignore_case(config_cpu->algoritmo_reemplazo_tlb, ALGORITMO_LRU))
		algoritmo = AR_LRU;
	else {
		log_error(logger_cpu, "El algoritmo:[%s] de reemplazo es incorrecto\n", config_cpu->algoritmo_reemplazo_tlb);
		return;
	}

	MMU = create_mmu(setup_memoria, config_cpu->cant_entradas_tlb, algoritmo);

	pthread_exit(NULL);
}


void cpu_detener() {
	if (memoria != NULL) {
		if (memoria->connected)
			socket_handler_disconnect(dispatch_server);

		socket_handler_destroy(memoria);
	}

	if (dispatch_server != NULL && dispatch_server->connected)
		socket_handler_disconnect(dispatch_server);

	if (interrupt_server != NULL && interrupt_server->connected)
		socket_handler_disconnect(interrupt_server);
}


static t_handshake_result memoria_handshake(t_socket_handler* handler) {
	send_signal(handler, MC_PROCESS_CODE, PROCESS_CPU);

	if (handler->error != 0) {
		log_error(logger_cpu, "error enviando codigo de proceso para handshake con memoria. error:[%d] msg:[%s]", handler->error, errortostr(handler->error));
		return CONNECTION_REJECTED;
	}

	t_handshake_result result = CONNECTION_REJECTED;
	t_package* package = recieve_message(handler);

	if (handler->error != 0) {
		log_error(logger_cpu, "error respuesta para handshake con memoria. error:[%d] msg:[%s]", handler->error, errortostr(handler->error));
	}
	else if (package->header != MC_MEMORY_SETUP) {
		log_error(logger_cpu, "error en operacion de handshake con memoria. Codigo de header:[%d] incorrecto", package->header);
	}
	else {
		t_setup_memoria* setup = deserializar_setup_memoria(package->payload);
		queue_push(memoria->data, (void*)setup);

		result = CONNECTION_ACCEPTED;
	}

	buffer_destroy(package->payload);
	package_destroy(package);

	return result;
}


static t_handshake_result kernel_handshake(t_socket_handler* handler) {

	t_handshake_result result = CONNECTION_REJECTED;
	t_package* package = recieve_message(handler);

	if (handler->error != 0) {
		log_error(logger_cpu, "Ocurrio un error al recibir el mensaje del kernel. error:[%d] msg:[%s]. Conexion rechazada de socket:[%d]",
				handler->error, errortostr(handler->error), handler->socket);
	}
	else if (package->header != MC_PROCESS_CODE) {
		log_error(logger_cpu, "Codigo:[%d] de paquete incorrecto para handshake. Conexion rechazada de socket:[%d]", package->header, handler->socket);
	}
	else {
		t_process_code process_code = deserialize_process_code(package->payload);

		if (process_code == PROCESS_KERNEL) {
			log_info(logger_cpu, "Conexion aceptada con kernel en socket:[%d]", handler->socket);
			result = CONNECTION_ACCEPTED;
		}
		else {
			log_error(logger_cpu, "Codigo:[%d] de proceso para handshake incorrecto. Conexion rechazada de socket:[%d]", process_code, handler->socket);
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
	bool connected = true;

	socket_handler_disconnect(dispatch_server);

	while(connected) {
		package = recieve_message(dispatch);

		if (dispatch->error != 0) {
			if (dispatch->error == SKT_CONNECTION_LOST) {
				log_error(logger_cpu, "dispatch :: desconectado. error:[%d] msg:[%s]", dispatch->error, errortostr(dispatch->error));
				connected = false;
			}
			else {
				log_error(logger_cpu, "dispatch :: error al recibir el mensaje del kernel. error:[%d] msg:[%s]", dispatch->error, errortostr(dispatch->error));
			}
		}
		else if (package->header != MC_PCB) {
			log_error(logger_cpu, "dispatch :: error al recibir el mensaje del kernel. encabezado:[%d] incorrecto. Se esperaba MC_PCB", package->header);
		}
		else {
			t_pcb* pcb = deserializar_pcb(package->payload);

			hay_interrupcion = 0;
			ejecutar_pcb(pcb);

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
	bool connected = true;

	while(connected) {

		// ACTIVAR MUTEX

		package = recieve_message(interrupt);

		if (interrupt->error != 0) {
			if (interrupt->error == SKT_CONNECTION_LOST) {
				log_error(logger_cpu, "interrupt :: desconectado. error:[%d] msg:[%s]", interrupt->error, errortostr(interrupt->error));
				connected = false;
			}
			else {
				log_error(logger_cpu, "interrupt :: error de mensaje recibido. error:[%d] msg:[%s]", interrupt->error, errortostr(interrupt->error));
			}
		}
		else if (package->header != MC_SIGNAL) {
			log_error(logger_cpu, "interrupt :: encabezado:[%d] incorrecto. Se esperaba 'MC_SIGNAL'", package->header);
		}
		else {
			t_signal signal = deserialize_signal(package->payload);

			if (signal != SG_CPU_INTERRUPT) {
				log_error(logger_cpu, "interrupt :: señal:[%d] incorrecta. Se esperaba 'SG_CPU_INTERRUPT'", package->header);
			}
			else {
				log_info(logger_cpu, "interrupt :: interrupcion de cpu");
				hay_interrupcion = 1;
			}
		}

		buffer_destroy(package->payload);
		package_destroy(package);
	}

	// DESACTIVAR MUTEX

	socket_handler_disconnect(interrupt);
	socket_handler_destroy(interrupt);
}


/*
static void execute(t_pcb* pcb) {
	bool exit_exec = false;
	uint64_t start = get_time_stamp();
	int inst_count = list_size(pcb->instructions);

	if (inst_count <= 0) {
		printf("mock-cpu :: el pcb:[%d] no tiene instrucciones para procesar", pcb->id);
		return;
	}

	while (!interrupted && !exit_exec && pcb->program_counter < inst_count) {
		t_instruction* instruction = list_get(pcb->instructions, pcb->program_counter);

		// TODO: en cada funcion, hay que validar que se haya ejecutado correctemnte
		// si no es asi tenes que parar el CPU!!!!
		switch(instruction->codigo) {
			case ci_NO_OP:
				printf("mock-cpu :: pcb:[%d] execute NO_OP", pcb->id);
				usleep(100000);
				break;
			case ci_I_O:
				pcb->cpu_dispatch_cond = CPU_IO_SYSCALL;
				pcb->io_burst =  atoi(list_get(instruction->parametros, 0));
				printf("mock-cpu :: pcb:[%d] execute IO(%d)", pcb->id, pcb->io_burst);
				exit_exec = true;
				break;
			case ci_WRITE:
				printf("mock-cpu :: pcb:[%d] execute WRITE(%d, %d)", pcb->id, atoi(list_get(instruction->parametros, 0)), atoi(list_get(instruction->parametros, 1)));
				usleep(500000);
				break;
			case ci_COPY:
				printf("mock-cpu :: pcb:[%d] execute COPY(%d, %d)", pcb->id, atoi(list_get(instruction->parametros, 0)), atoi(list_get(instruction->parametros, 1)));
				usleep(500000 * 2);
				break;
			case ci_READ:
				printf("mock-cpu :: pcb:[%d] execute READ(%d)", pcb->id, atoi(list_get(instruction->parametros, 0)));
				usleep(500000);
				break;
			case ci_EXIT:
				printf("mock-cpu :: pcb:[%d] execute EXIT", pcb->id);
				pcb->cpu_dispatch_cond = CPU_EXIT_PROGRAM;
				exit_exec = true;
				break;
			default:
				printf("mock-cpu :: la instruccion:[%d] del pcb:[%d] tiene un codigo invalido", instruction->codigo, pcb->id);
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

	printf("mock-cpu :: pcb:[%d] desalojado por '%s' executed time:[%lu]", pcb->id, dispatchcondtostr(pcb->cpu_dispatch_cond), (long unsigned)diff);
}
*/



static void server_listening(t_socket_handler* server) {
	log_info(logger_cpu, "Servidor '%s' preparado y escuchando ...", server->name);
}


// el metodo es invocado por el manejador del socket cuando el servidor se detiene
static void server_disconnected(t_socket_handler* server) {
	log_info(logger_cpu, "El servidor '%s' desconectado", server->name);

	if (server->socket == dispatch_server->socket)
		dispatch_server = NULL;
	else if (server->socket == interrupt_server->socket)
		interrupt_server = NULL;

	socket_handler_destroy(server);
}


static void server_error(t_socket_handler* server) {
	log_info(logger_cpu, "server '%s' error >> Ocurrio un error durante la ejecucion del servicio de conexiones :: error:[%d] msg:[%s]",
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
