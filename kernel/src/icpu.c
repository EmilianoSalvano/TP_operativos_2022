/*
 * cpu.c
 *
 *  Created on: 10 jul. 2022
 *      Author: utnso
 */


#include "../include/icpu.h"

#include <stdlib.h>
#include <commons/log.h>

#include "../../shared/include/kiss.structs.h"
#include "../../shared/include/kiss.serialization.h"
#include "../../shared/include/socket_handler.h"
#include "../../shared/include/kiss.error.h"


t_kernel* kernel;
t_log* logger;
t_cpu_module* cpu;


// *********** Declaracion de meotodos privados *********** /
static int cpu_interrupt_connect();
static int cpu_dispatch_connect();
static t_handshake_result cpu_handshake(t_socket_handler* handler);
static void cpu_recieve_pcb_async(t_socket_handler* handler);




// ********************************* PUBLIC ************************************************* //

t_cpu_module* cpu_module_create() {
	cpu = malloc(sizeof(t_cpu_module));

	cpu->dispatch = NULL;
	cpu->interrupt = NULL;
	cpu->busy = false;
	cpu->interrupted = false;

	return cpu;
}


void cpu_module_destroy() {
	if (cpu->dispatch != NULL)
		socket_handler_destroy(cpu->dispatch);

	if (cpu->interrupt != NULL)
		socket_handler_destroy(cpu->interrupt);

	free(cpu);
}


int cpu_module_run() {

	if (cpu == NULL) {
		log_warning(logger, "No se puede iniciar la interface con el modulo CPU porque aun no fue creada");
		return EXIT_FAILURE;
	}

	log_info(logger, "Iniciando interface con modulo CPU ...");

	// puerto interrupt
	if (cpu_interrupt_connect() != 0) {
		log_info(logger, "La interface al modulo CPU no pudo ser establecida");
		return EXIT_FAILURE;
	}

	// puerto dispatch
	if (cpu_dispatch_connect() != 0) {
		log_info(logger, "La interface al modulo CPU no pudo ser establecida");
		return EXIT_FAILURE;
	}

	log_info(logger, "Interface con modulo CPU establecida :)");
	return EXIT_SUCCESS;
}


int cpu_module_stop() {
	if (cpu->dispatch != NULL)
		socket_handler_disconnect(cpu->dispatch);

	if (cpu->interrupt != NULL)
		socket_handler_disconnect(cpu->interrupt);

	return EXIT_SUCCESS;
}


int cpu_dispatch_pcb(t_pcb* pcb) {

	t_buffer* buffer = serializar_pcb(pcb);

	send_message(cpu->dispatch, MC_PCB, buffer);
	buffer_destroy(buffer);

	if (cpu->dispatch->error != 0) {
		log_error(logger, "Error enviando pcb:[%d] a CPU por dispatch, error[%d] msg:[%s]", pcb->id, cpu->dispatch->error, errortostr(cpu->dispatch->error));

		if (cpu->interrupt->error == SKT_CONNECTION_LOST)
			kernel->error();

		return EXIT_FAILURE;
	}

	log_debug(logger, "pcb:[%d] enviado a CPU por dispatch", pcb->id);
	log_debug(logger, "cpu_dispatch_pcb :: cpu->busy:[TRUE]");

	cpu->busy = true;
	return EXIT_SUCCESS;
}


int cpu_interrupt() {
	send_signal(cpu->interrupt, MC_SIGNAL, SG_CPU_INTERRUPT);

	if (cpu->interrupt->error != 0) {
		log_error(logger, "Error de envio de interrupcion a CPU. error:[%d] msg:[%s]", cpu->interrupt->error, errortostr(cpu->interrupt->error));

		if (cpu->interrupt->error == SKT_CONNECTION_LOST)
			kernel->error();

		return EXIT_FAILURE;
	}

	log_debug(logger, "interrupcion enviada a CPU");

	cpu->interrupted = true;
	return EXIT_SUCCESS;
}



void cpu_listen_dispatch_async() {
	recieve_message_async(cpu->dispatch, cpu_recieve_pcb_async);
}



/******************************** PRIVATE ***********************************************/
static void cpu_recieve_pcb_async(t_socket_handler* handler) {

	if (kernel->halt)
		return;

	if (handler->error != 0) {
		log_error(logger, "Error en mensaje recibido por dispatch. error:[%d] msg:[%s]", handler->error, errortostr(handler->error));
		kernel->planner->dispatch_error();
		return;
	}
	else if (handler->data == NULL || queue_size(handler->data) == 0) {
		log_error(logger, "Error en mensaje recibido por dispatch. El pcb no fue retornado por el CPU");
		kernel->planner->dispatch_error();
		return;
	}

	t_package* package = (t_package*)queue_pop(handler->data);

	if (package->header != MC_PCB) {
		log_error(logger, "Error en mensaje recibido por dispatch. El tipo de mensaje:[%d] no coincide con el esperado", package->header);
		kernel->planner->dispatch_error();
	}
	else {
		t_pcb* pcb = deserializar_pcb(package->payload);

		log_debug(logger, "pcb:[%d] recibido por dispatch", pcb->id);
		log_debug(logger, "cpu_recieve_pcb_async :: cpu->busy:[FALSE]");

		cpu->busy = false;
		kernel->planner->incomming_dispatch(pcb);
	}

	buffer_destroy(package->payload);
	package_destroy(package);
}


static int cpu_dispatch_connect() {
	log_info(logger, "Conectando al puerto 'dispatch' de CPU ip:[%s] port:[%s] ...",
		kernel->config->ip_cpu, kernel->config->puerto_cpu_dispatch);

	cpu->dispatch = connect_to_server(kernel->config->ip_cpu, kernel->config->puerto_cpu_dispatch);

	if (cpu->dispatch->error != 0) {
		log_error(logger, "No se pudo establecer conexion con el puerto 'dispatch' del CPU. error:[%d] msg:[%s]",
				cpu->dispatch->error, errortostr(cpu->dispatch->error));

		return EXIT_FAILURE;
	}

	if (cpu_handshake(cpu->dispatch) != CONNECTION_ACCEPTED) {
		log_warning(logger, "Conexion rechazada por el modulo CPU para puerto 'dispatch'");
		return EXIT_FAILURE;
	}

	log_info(logger, "Puerto 'dispatch' de CPU conectado!");

	return EXIT_SUCCESS;
}


static int cpu_interrupt_connect() {
	log_info(logger, "Conectando al puerto 'interrupt' de CPU ip:[%s] port:[%s] ...",
			kernel->config->ip_cpu, kernel->config->puerto_cpu_interrupt);

	cpu->interrupt = connect_to_server(kernel->config->ip_cpu, kernel->config->puerto_cpu_interrupt);

	if (cpu->interrupt->error != 0) {
		log_error(logger, "No se pudo establecer conexion con el puerto 'interrupt' del CPU. error:[%d] msg:[%s]",
				cpu->interrupt->error, errortostr(cpu->interrupt->error));

		return EXIT_FAILURE;
	}

	if (cpu_handshake(cpu->interrupt) != CONNECTION_ACCEPTED) {
		log_warning(logger, "Conexion rechazada por el modulo CPU para puerto 'interrupt'");
		return EXIT_FAILURE;
	}

	log_info(logger, "Puerto 'interrupt' de CPU conectado!");

	return EXIT_SUCCESS;
}


static t_handshake_result cpu_handshake(t_socket_handler* handler) {

	send_signal(handler, MC_PROCESS_CODE, PROCESS_KERNEL);

	if (handler->error != 0) {
		log_error(logger, "Error enviando codigo de proceso para handshake con memoria. error:[%d] msg:[%s]",
				handler->error, errortostr(handler->error));

		return CONNECTION_REJECTED;
	}


	t_handshake_result result = CONNECTION_REJECTED;
	t_package* package = recieve_message(handler);

	if (handler->error != 0) {
		log_error(logger, "Error recibiendo respuesta para handshake con memoria. error:[%d] msg:[%s]",
				handler->error, errortostr(handler->error));
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


