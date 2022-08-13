/*
 * estructuras.c
 *
 *  Created on: 29 may. 2022
 *      Author: utnso
 */


#include <stdlib.h>
#include <commons/string.h>

#include "../include/kiss.structs.h"
#include "../include/utils.h"


#define PCB_NEW_DESC "NEW"
#define PCB_READY_DESC "READY"
#define PCB_BLOCKED_DESC "BLOCKED"
#define PCB_EXEC_DESC "EXEC"
#define PCB_EXIT_DESC "EXIT"
#define PCB_SUSPENDED_READY_DESC "SUSPENDED_READY"
#define PCB_SUSPENDED_BLOCKED_DESC "SUSPENDED_BLOCKED"
#define PCB_UNKOWN_DESC "UNKOWN"

#define CPU_NULL_CONDITION_DESC "NULL_CONDITION"
#define CPU_EXIT_PROGRAM_DESC "EXIT_PROGRAM"
#define CPU_INTERRUPTED_BY_KERNEL_DESC "INTERRUPTED_BY_KERNEL"
#define CPU_IO_SYSCALL_DESC "IO_SYSCALL"


char* prcstatustostr(t_process_status status) {
	switch(status) {
	case PCB_NEW:
		return PCB_NEW_DESC;
	case PCB_READY:
		return PCB_READY_DESC;
	case PCB_BLOCKED:
		return PCB_BLOCKED_DESC;
	case PCB_EXEC:
		return PCB_EXEC_DESC;
	case PCB_EXIT:
		return PCB_EXIT_DESC;
	case PCB_SUSPENDED_READY:
		return PCB_SUSPENDED_READY_DESC;
	case PCB_SUSPENDED_BLOCKED:
		return PCB_SUSPENDED_BLOCKED_DESC;
	default:
		return PCB_UNKOWN_DESC;

	}
}

char* dispatchcondtostr(t_dispatch_cond cond) {
	switch(cond) {
		case CPU_EXIT_PROGRAM:
			return CPU_EXIT_PROGRAM_DESC;
		case CPU_INTERRUPTED_BY_KERNEL:
			return CPU_INTERRUPTED_BY_KERNEL_DESC;
		case CPU_IO_SYSCALL:
			return CPU_IO_SYSCALL_DESC;
		default:
			return CPU_NULL_CONDITION_DESC;
	}
}

/************************ t_instruccion **********************************************************/

t_instruction* instruccion_create(t_codigo_instruccion codigo, bool empty) {
	t_instruction* self = malloc(sizeof(t_instruction));
	self->codigo = codigo;

	if (empty)
		self->parametros = NULL;
	else
		self->parametros = list_create();

	return self;
}

void instruccion_destroy(t_instruction* self) {
	if (self->parametros != NULL)
		list_destroy_and_destroy_elements(self->parametros, param_destroy_in_list);

	free(self);
}

void instruccion_destroy_in_list(void* instruccion) {
	instruccion_destroy((t_instruction*)instruccion);
}

char* instruccion_to_string(t_instruction* self) {
	char* 	params;
	char* 	formated_param;
	char* 	param;
	char*	string;
	int		p_count;

	char* format_inst = "{codigo: %d, cantidad_params: %d, params[%s]}";
	char* format_param = "{indice: %d, valor: '%s'}";
	char* format_param_next = string_from_format(", %s", format_param);

	params = string_new();

	p_count = list_size(self->parametros);
	if (p_count > 0) {
		t_list_iterator* iterator = list_iterator_create(self->parametros);

		while(list_iterator_has_next(iterator)) {
			param = list_iterator_next(iterator);

			if (iterator->index == 0)
				formated_param = string_from_format(format_param, iterator->index, param);
			else
				formated_param = string_from_format(format_param_next, iterator->index, param);

			string_append(&params, formated_param);

			free(formated_param);
		}

		list_iterator_destroy(iterator);
	}

	string = string_from_format(format_inst, self->codigo, list_size(self->parametros), params);

	free(params);
	free(format_param_next);

	return string;
}

char* instruccion_list_to_string(t_list* instrucciones) {
	t_instruction* instruccion;
	char* str_instrucciones;
	char* str_instruccion;
	bool primero = true;
	char* string;

	t_list_iterator* iterator = list_iterator_create(instrucciones);

	while(list_iterator_has_next(iterator)) {
		instruccion = list_iterator_next(iterator);

		str_instruccion = instruccion_to_string(instruccion);

		if (primero) {
			primero = false;
			str_instrucciones = string_duplicate(str_instruccion);
		}
		else {
			string_append(&str_instrucciones, ", ");
			string_append(&str_instrucciones, str_instruccion);
		}

		free(str_instruccion);
	}

	list_iterator_destroy(iterator);


	string = string_from_format("[%s]", str_instrucciones);
	free(str_instrucciones);

	return string;

}


t_instruction* produce_instruccion() {
	t_instruction* instruccion = instruccion_create(produce_num(4), false);
	int param_count = 0;

	switch(instruccion->codigo) {
	case ci_NO_OP:
		param_count = 1;
		break;
	case ci_I_O:
		param_count = 1;
		break;
	case ci_WRITE:
		param_count = 2;
		break;
	case ci_COPY:
		param_count = 2;
		break;
	case ci_READ:
		param_count = 1;
		break;
	/*
	case ci_EXIT:
		param_count = 0;
		break;
	*/
	}

	for (int p = 0; p < param_count; p++) {
		if (instruccion->codigo == ci_I_O)
			list_add(instruccion->parametros, string_itoa(produce_num_in_range(1000, 5000)));
		else
			list_add(instruccion->parametros, string_itoa(produce_num(100)));
	}

	return instruccion;
}


void param_destroy_in_list(void* param) {
	free(param);
}


/************************ t_pcb **********************************************************/

char* pcb_to_string(t_pcb* self) {
	char* instrucciones;
	char* format_pcb = "{id: %d, status: %s, substatus: %s, process_size: %d, program_counter: %d, tabla_paginas: %d, cpu_dispatch_cond: %d, "
			"cpu_estimated_burst: %d, cpu_executed_burst: %d, io_burst: %d, instrucciones: %s}";

	instrucciones = instruccion_list_to_string(self->instructions);
	char* string = string_from_format(format_pcb, self->id, prcstatustostr(self->status), prcstatustostr(self->prev_status), self->process_size,
			self->program_counter, self->tabla_paginas, self->cpu_dispatch_cond, self->cpu_estimated_burst, self->cpu_executed_burst,
			self->io_burst, instrucciones);


	free(instrucciones);
	return string;
}


t_pcb* produce_pcb() {
	t_instruction* instruccion;
	t_pcb* pcb = pcb_create(NULL);

	pcb->id = produce_num_in_range(1, 100);
	pcb->status = PCB_NEW;
	pcb->prev_status = PCB_NEW;

	pcb->process_size = produce_num_in_range(10, 100);
	pcb->program_counter = produce_num_in_range(1, 10);
	pcb->tabla_paginas = produce_num(100);

	pcb->cpu_dispatch_cond = produce_num_in_range(0, 3);
	pcb->cpu_estimated_burst = produce_num_in_range(500, 3000);
	pcb->cpu_executed_burst = produce_num_in_range(500, 3000);
	pcb->io_burst = produce_num_in_range(500, 3000);

	pcb->instructions = list_create();

	int i_count = produce_num_in_range(1, 5);

	for (int i = 0; i < i_count; i++) {
		instruccion = produce_instruccion();
		list_add(pcb->instructions, instruccion);
	}

	// EXIT
	instruccion = instruccion_create(ci_EXIT, false);
	list_add(pcb->instructions, instruccion);

	return pcb;
}


t_pcb* pcb_create(t_list* instructions) {
	t_pcb* self = malloc(sizeof(t_pcb));

	self->id = -1;
	self->status = PCB_NEW;
	self->prev_status = PCB_NEW;

	self->process_size = 0;
	self->tabla_paginas = 0;
	self->program_counter = 0;
	self->instructions = instructions;
	self->socket_handler = NULL;

	self->cpu_dispatch_cond = CPU_NULL_CONDITION;
	self->cpu_estimated_burst = 0;
	self->cpu_executed_burst = 0;
	self->io_burst = 0;
	self->timer_blocked = NULL;
	self->timer_suspended_blocked = NULL;

	// estadisticas
	self->error = false;
	self->cpu_execution_count = 0;
	self->total_cpu_burst_elapsed = 0;
	self->io_execution_count = 0;
	self->total_io_burst_elapsed = 0;

	self->start_time = 0;

	return self;
}

void pcb_destroy(t_pcb* self) {
	if (self->instructions  != NULL)
		list_destroy_and_destroy_elements(self->instructions, instruccion_destroy_in_list);

	free(self);
}

void pcb_destroy_in_list(void* pcb) {
	pcb_destroy((t_pcb*)pcb);
}



/************************ t_pcb **********************************************************/
t_program* program_create(t_list* instructions) {
	t_program* self = malloc(sizeof(t_program));
	self->size = 0;
	self->instructions = instructions;

	return self;
}

void program_destroy(t_program* self) {

	if (self->instructions != NULL)
		list_destroy_and_destroy_elements(self->instructions, instruccion_destroy_in_list);

	free(self);
}

char* program_to_string(t_program* self) {
	char* instrucciones;
	char* format_pcb = "{process_size: %d, instrucciones:%s}";

	instrucciones = instruccion_list_to_string(self->instructions);
	char* string = string_from_format(format_pcb, self->size, instrucciones);

	free(instrucciones);
	return string;
}

t_program* produce_program() {
	t_instruction* instruccion;

	t_list* list = list_create();
	int i_count = produce_num_in_range(1, 5);

	for (int i = 0; i < i_count; i++) {
		instruccion = produce_instruccion();
		list_add(list, instruccion);
	}

	// EXIT
	instruccion = instruccion_create(ci_EXIT, false);
	list_add(list, instruccion);

	t_program* program = program_create(list);
	program->size = produce_num_in_range(10, 100);

	return program;
}



/************************ t_estruc_memoria **********************************************************/
t_setup_memoria* setup_memoria_create() {
	t_setup_memoria* self = malloc(sizeof(t_setup_memoria));
	self->entradas_por_tabla = 0;
	self->tam_pagina = 0;

	return self;
}

void setup_memoria_destroy(t_setup_memoria* self) {
	free(self);
}



/************************ t_operacion_EU **********************************************************/
t_operacion_EU* operacion_eu_create() {
	t_operacion_EU* self = malloc(sizeof(t_operacion_EU));
	self->pid = -1;
	self->dato = 0;
	self->desplazamiento = -1;
	self->numero_marco = -1;

	return self;
}

void operacion_eu_destroy(t_operacion_EU* self) {
	free(self);
}



/************************ t_operacion_EU **********************************************************/
t_direccion* direccion_create() {
	t_direccion* self = malloc(sizeof(t_direccion));
	self->pid = -1;
	self->pagina = -1;
	self->tabla_primer_nivel = -1;
	self->tabla_segundo_nivel = -1;

	return self;
}

void direccion_destroy(t_direccion* self) {
	free(self);
}


/************************ t_page_entry_request **********************************************************/

t_page_entry_request* page_entry_request_create() {
	t_page_entry_request* self = malloc(sizeof(t_page_entry_request));
	self->pcb_id = 0;
	self->process_size = 0;

	return self;
}

void page_entry_request_destroy(t_page_entry_request* self) {
	free(self);
}

