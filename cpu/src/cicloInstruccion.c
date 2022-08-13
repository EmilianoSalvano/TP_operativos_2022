/*
 * cicloInstruccion.c
 *
 *  Created on: 15 jun. 2022
 *      Author: utnso
 */

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <commons/collections/list.h>

#include "../include/cicloInstruccion.h"
#include "../include/cpu_global.h"
#include "../include/tlb.h"
#include "../../shared/include/kiss.serialization.h"
#include "../../shared/include/kiss.error.h"


t_log* logger_cpu;


static bool get_valor_memoria(t_socket_handler* handler, t_pcb* pcb, t_dir_fisica direccion, int* valor);
static bool set_valor_memoria(t_socket_handler* handler, t_pcb* pcb, t_dir_fisica direccion, int valor);

static t_instruction* fetch(t_pcb* PCB);
static int decode(t_instruction* instruccion);
static bool faseOperands(t_pcb* pcb, t_instruction* instruccion, int* operando);
static bool ci_read(t_pcb* pcb, int direccion_logica, int* operando);
static bool ci_write(t_pcb* pcb, int direccion_logica, int operando);
int execute(t_instruction * instr, t_pcb* PCB, int valor_obtenido_fase_operands);
static int checkInterrupt();




//ejecucion de intruccion de una pcb
bool ejecutar_pcb(t_pcb * pcb)
{
	limpiar_tlb(MMU->TLB);
	log_info(logger_cpu, "Se limpio la TLB por nuevo proceso");

	bool error = false;
	t_instruction * instruccion;
	uint64_t inicio, fin, tiempo_total;
	int valor_2da_dir_logic;

	enviar_pcb_o_exit = 0;
	//hay_interrupcion = 0;
	inicio = get_time_stamp();

	do
	{
		instruccion = fetch(pcb);

		if (decode(instruccion))
		{
			if (!faseOperands(pcb, instruccion, &valor_2da_dir_logic)) {
				log_error(logger_cpu, "no se pudo obtener el valor de memoria para la instruccion:[%d] del pcb:[%d]", instruccion->codigo, pcb->id);
				error = true;
				break;
			}
		}

		execute(instruccion, pcb, valor_2da_dir_logic);

	} while (checkInterrupt() == 0);

	fin = get_time_stamp();
	tiempo_total = fin - inicio;

	//log_info(logger_cpu, "fin de ejecucion de pcb:[%d] tiempo:[%lu]", pcb->id, (unsigned long)tiempo_total);

	pcb->cpu_executed_burst = tiempo_total;
	hay_interrupcion = 0;

	// TODO: pasar el error al pcb
	return (!error);
}


static t_instruction* fetch(t_pcb* pcb)
{
	//log_debug(logger_cpu, "pcb:[%d] program_counter:[%d] instrucciones:[%d]", pcb->id, pcb->program_counter, list_size(pcb->instructions));
	t_instruction * instruccion = list_get(pcb->instructions, pcb->program_counter);
	pcb->program_counter++;
	return instruccion;
}


static int decode(t_instruction* instruccion)
{
	return instruccion->codigo == ci_COPY? 1:0;
}


static bool faseOperands(t_pcb* pcb, t_instruction* instruccion, int* operando)
{
	int dir_logic_2do_param = atoi(list_get(instruccion->parametros, 1));
	t_dir_fisica dir_fisica = direccionFisica(MMU, pcb, dir_logic_2do_param);

	if (!dir_fisica.acierto)
		return false;

	return get_valor_memoria(memoria, pcb, dir_fisica, operando);
}


static bool ci_read(t_pcb* pcb, int direccion_logica, int* operando) {
	t_dir_fisica dir_fisica = direccionFisica(MMU, pcb, direccion_logica);
	return get_valor_memoria(memoria, pcb, dir_fisica, operando);
}


static bool ci_write(t_pcb* pcb, int direccion_logica, int operando) {
	t_dir_fisica dir_fisica = direccionFisica(MMU, pcb, direccion_logica);
	return set_valor_memoria(memoria, pcb, dir_fisica, operando);
}


int execute(t_instruction * instr, t_pcb* pcb, int valor_obtenido_fase_operands)
{
	int valor_obtenido = 0;

	switch (instr->codigo) {
		case ci_NO_OP:
			usleep(config_cpu->retardo_NO_OP * 1000);
			log_info(logger_cpu, "pid:[%d] execute :: NO_OP(%d)", pcb->id, config_cpu->retardo_NO_OP);
			break;
		case ci_I_O:
			pcb->cpu_dispatch_cond= CPU_IO_SYSCALL;
			pcb->io_burst = atoi(list_get(instr->parametros, 0));
			log_info(logger_cpu, "pid:[%d] execute :: I_O(%d)", pcb->id, pcb->io_burst);
			//con esto salimos del while
			enviar_pcb_o_exit = 1;
			break;
		case ci_READ:
			// TODO: resolver cuando falla la operacion
			ci_read(pcb, atoi(list_get(instr->parametros, 0)), &valor_obtenido);
			log_info(logger_cpu, "pid:[%d] execute :: READ(%d) = %d", pcb->id, atoi(list_get(instr->parametros, 0)), valor_obtenido);
			break;
		case ci_WRITE:
			// TODO: resolver cuando falla la operacion
			log_info(logger_cpu, "pid:[%d] execute :: WRITE(%d, %d)", pcb->id, atoi(list_get(instr->parametros, 0)), atoi(list_get(instr->parametros, 1)));
			ci_write(pcb, atoi(list_get(instr->parametros, 0)), atoi(list_get(instr->parametros, 1)));
			break;
		case ci_COPY:
			// TODO: resolver cuando falla la operacion
			log_info(logger_cpu, "pid:[%d] execute :: COPY(%d, %d)", pcb->id, atoi(list_get(instr->parametros, 0)), valor_obtenido_fase_operands);
			ci_write(pcb, atoi(list_get(instr->parametros, 0)), valor_obtenido_fase_operands);
			break;
		case ci_EXIT:
			pcb->cpu_dispatch_cond = CPU_EXIT_PROGRAM;
			//con esto salimos del while
			enviar_pcb_o_exit = 1;
			log_info(logger_cpu, "pid:[%d] execute :: EXIT", pcb->id);
			break;
		default:
			log_error(logger_cpu, "la instruccion:[%d] del pcb:[%d] tiene un codigo invalido", instr->codigo, pcb->id);
			return EXIT_FAILURE;
	}

	if (hay_interrupcion && !enviar_pcb_o_exit)
		pcb->cpu_dispatch_cond = CPU_INTERRUPTED_BY_KERNEL;

	return 0;
}


static int checkInterrupt()
{
	if(hay_interrupcion != 0)
	{
		enviar_pcb_o_exit = 1;
	}

	return enviar_pcb_o_exit;
}



static bool get_valor_memoria(t_socket_handler* handler, t_pcb* pcb, t_dir_fisica direccion, int* valor) {

	sem_wait(&sem_memory_request);

	bool valor_obtenido = false;

	t_operacion_EU* operacion = operacion_eu_create();
	operacion->pid = pcb->id;
	operacion->numero_marco = direccion.marco;
	operacion->desplazamiento = direccion.desplazamiento;

	log_debug(logger_cpu, "solicitud MC_LEER_MARCO :: pid:[%d] numero_marco:[%d] desplazamiento:[%d] dato:[%d]",
			operacion->pid, operacion->numero_marco, operacion->desplazamiento, operacion->dato);

	t_buffer* buffer = serializar_operacion_eu(operacion);
	send_message(handler, MC_LEER_MARCO, buffer);
	buffer_destroy(buffer);
	operacion_eu_destroy(operacion);

	if (handler->error != 0) {
		log_error(logger_cpu, "error al solicitar el valor del marco:[%d] desplazamiento:[%d] a memoria. error[%d] msg:[%s]",
				direccion.marco, direccion.desplazamiento, handler->error, errortostr(handler->error));

		/*
		if (self->memoria->error == SKT_CONNECTION_LOST) {
			direccion_destroy(direccion);
			return dir_fisica;
		}
		*/
	}
	else {
		t_package* package = recieve_message(handler);

		if (handler->error != 0) {
			log_error(logger_cpu, "error al recibir el valor del marco:[%d] solicitado. error:[%d] msg:[%s]",
					direccion.marco, handler->error, errortostr(handler->error));

			/*
			if (self->memoria->error == SKT_CONNECTION_LOST)
				kernel->error();
			*/
		}
		else if (package->header == MC_LEER_MARCO) {
			(*valor) = deserialize_uint32t(package->payload);
			valor_obtenido = true;

			log_debug(logger_cpu, "respuesta MC_LEER_MARCO :: marco:[%d]", (*valor));
		}
		else {
			log_error(logger_cpu, "error al recibir el valor de marco:[%d]. Codigo de header:[%d] incorrecto", direccion.marco, package->header);
		}

		buffer_destroy(package->payload);
		package_destroy(package);
	}

	sem_post(&sem_memory_request);

	return valor_obtenido;
}


static bool set_valor_memoria(t_socket_handler* handler, t_pcb* pcb, t_dir_fisica direccion, int valor) {

	sem_wait(&sem_memory_request);

	bool valor_escrito = false;

	t_operacion_EU* operacion = operacion_eu_create();
	operacion->pid = pcb->id;
	operacion->numero_marco = direccion.marco;
	operacion->desplazamiento = direccion.desplazamiento;
	operacion->dato = valor;

	log_debug(logger_cpu, "set_valor_memoria :: MC_ESCRIBIR_MARCO :: pid:[%d] numero_marco:[%d] desplazamiento:[%d] dato:[%d]",
			operacion->pid, operacion->numero_marco, operacion->desplazamiento, operacion->dato);

	t_buffer* buffer = serializar_operacion_eu(operacion);
	send_message(handler, MC_ESCRIBIR_MARCO, buffer);
	buffer_destroy(buffer);
	operacion_eu_destroy(operacion);

	if (handler->error != 0) {
		log_error(logger_cpu, "error al solicitar el valor del marco:[%d] desplazamiento:[%d] a memoria. error[%d] msg:[%s]",
				direccion.marco, direccion.desplazamiento, handler->error, errortostr(handler->error));

		/*
		if (self->memoria->error == SKT_CONNECTION_LOST) {
			direccion_destroy(direccion);
			return dir_fisica;
		}
		*/
	}
	else {
		t_package* package = recieve_message(handler);

		if (handler->error != 0) {
			log_error(logger_cpu, "error al recibir el valor del marco:[%d] solicitado. error:[%d] msg:[%s]",
					direccion.marco, handler->error, errortostr(handler->error));

			/*
			if (self->memoria->error == SKT_CONNECTION_LOST)
				kernel->error();
			*/
		}
		else if (package->header == MC_SIGNAL) {
			t_signal signal = deserialize_signal(package->payload);
			valor_escrito = (signal == SG_OPERATION_SUCCESS);
		}
		else {
			log_error(logger_cpu, "error al recibir el valor de marco:[%d]. Codigo de header:[%d] incorrecto", direccion.marco, package->header);
		}

		buffer_destroy(package->payload);
		package_destroy(package);
	}

	sem_post(&sem_memory_request);

	return valor_escrito;
}

