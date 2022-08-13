/*
 * mmu.c
 *
 *  Created on: 24 jun. 2022
 *      Author: utnso
 */
#include <semaphore.h>

#include "../include/mmu.h"
#include "../include/cpu_global.h"
#include "../../shared/include/kiss.error.h"
#include "../../shared/include/kiss.serialization.h"


static int get_segundo_nivel(t_socket_handler* handler, t_direccion* direccion);
static int get_frame(t_socket_handler* handler, t_direccion* direccion);


t_log* logger_cpu;

// LA MMU resuelve las traducciones de memoria logica a fisica
// consulta primero la TLB, si acierta,devuelve el frame
// si no acierta, consulta a memoria para obtener el frame y luego actualiza la TLB

t_mmu * create_mmu(t_setup_memoria* config_memoria, int entradas_tlb, t_alg_reemplazo protocolo_tlb)
{
	t_mmu* self = malloc(sizeof(t_mmu));
	self->TLB = crearTLB(entradas_tlb, protocolo_tlb);
	self->config_memoria = config_memoria;

	return self;
}

void destroy_mmu(t_mmu* self)
{
	if (self->config_memoria != NULL)
		setup_memoria_destroy(self->config_memoria);

	destroyTLB(self->TLB);

	free(self);
}


// 1. busca en la TLB si esta la pagina y devuelve la direccion con el marco asociado
// 2. si no hay acierto, hace dos llamadas a la memoria. obtiene el numerod e marco y actualiza la TLB
t_dir_fisica direccionFisica(t_mmu* self, t_pcb* pcb, int dir_logica)
{
	t_dir_fisica dir_fisica;


/*
	[entrada_tabla_1er_nivel | entrada_tabla_2do_nivel | desplazamiento]

Estas traducciones, en los ejercicios prácticos que se ven en clases y se toman en los parciales,
normalmente se hacen en binario. Como en la realización del trabajo práctico es más cómodo utilizar
números enteros en sistema decimal, y tomando en cuenta que las tablas tanto de 1er nivel como de
2do nivel tendrán la misma cantidad de entradas, la operatoria sería más parecida a la siguiente:

	número_página = floor(dirección_lógica / tamaño_página)
	entrada_tabla_1er_nivel = floor(número_página / cant_entradas_por_tabla)
	entrada_tabla_2do_nivel = número_página mod (cant_entradas_por_tabla)
	desplazamiento = dirección_lógica - número_página * tamaño_página

 */
	dir_fisica.acierto = false;
	dir_fisica.num_pag = dir_logica / self->config_memoria->tam_pagina;
	dir_fisica.desplazamiento = dir_logica - (dir_fisica.num_pag * self->config_memoria->tam_pagina);
	dir_fisica.marco = buscar_marco(self->TLB, dir_fisica.num_pag);

	if (dir_fisica.marco >= 0) {
		log_debug(logger_cpu, "acierto de TLB. pagina:[%d] marco:[%d] desplazamiento:[%d]", dir_fisica.num_pag, dir_fisica.marco, dir_fisica.desplazamiento);

		dir_fisica.acierto = true;
		return dir_fisica;
	}

	// si no hay acierto en la TLB, busco en memoria
	t_direccion* direccion = direccion_create();
	direccion->pid = pcb->id;

	// TODO: si la direccion logica esta mal, esto hace cualquiera
	//direccion->tabla_primer_nivel = dir_fisica.num_pag / self->config_memoria->entradas_por_tabla;
	direccion->tabla_primer_nivel = pcb->tabla_paginas;
	//direccion->tabla_primer_nivel = dir_fisica.num_pag % self->config_memoria->entradas_por_tabla;
	direccion->pagina = dir_fisica.num_pag;

	if (get_segundo_nivel(memoria, direccion) < 0) {
		log_error(logger_cpu, "error al recibir la entrada:[%d] de pagina de segundo nivel de memoria. No hubo acierto", direccion->tabla_segundo_nivel);
	}
	else {
		dir_fisica.marco = get_frame(memoria, direccion);

		if (dir_fisica.marco >= 0) {
			dir_fisica.acierto = true;
			t_entry_tlb* entrada = entry_tlb_create();
			entrada->marco = dir_fisica.marco;
			entrada->pagina = dir_fisica.num_pag;

			reemplazar_entrada(self->TLB, entrada);
		}
	}

	direccion_destroy(direccion);
	return dir_fisica;
}


// TODO: hacer algo con la desconexion
static int get_segundo_nivel(t_socket_handler* handler, t_direccion* direccion) {

	sem_wait(&sem_memory_request);

	direccion->tabla_segundo_nivel = -1;

	t_buffer* buffer = serializar_direccion(direccion);
	send_message(handler, MC_BUSCAR_TABLA_DE_PAGINAS, buffer);
	buffer_destroy(buffer);

	if (handler->error != 0) {
		log_error(logger_cpu, "error al solicitar la entrada de pagina de segundo nivel a memoria. primer nivel:[%d] pagina:[%d], error[%d] msg:[%s]",
				direccion->tabla_primer_nivel, direccion->pagina, handler->error, errortostr(handler->error));

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
			log_error(logger_cpu, "error al recibir la entrada de pagina de segundo nivel de memoria. error:[%d] msg:[%s]",
					handler->error, errortostr(handler->error));

			/*
			if (self->memoria->error == SKT_CONNECTION_LOST)
				kernel->error();
			*/
		}
		else if (package->header == MC_BUSCAR_TABLA_DE_PAGINAS) {
			direccion->tabla_segundo_nivel = deserialize_uint32t(package->payload);
		}
		else {
			log_error(logger_cpu, "error al recibir la entrada de pagina de segundo nivel de memoria. Codigo de header:[%d] incorrecto", package->header);
		}

		buffer_destroy(package->payload);
		package_destroy(package);
	}

	sem_post(&sem_memory_request);

	return direccion->tabla_segundo_nivel;
}


static int get_frame(t_socket_handler* handler, t_direccion* direccion) {
	int frame = -1;

	sem_wait(&sem_memory_request);

	t_buffer* buffer = serializar_direccion(direccion);
	send_message(handler, MC_BUSCAR_MARCO, buffer);
	buffer_destroy(buffer);

	if (handler->error != 0) {
		log_error(logger_cpu, "error al solicitar el numero de marco a memoria. segundo nivel:[%d] pagina:[%d], error[%d] msg:[%s]",
				direccion->tabla_segundo_nivel, direccion->pagina, handler->error, errortostr(handler->error));

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
			log_error(logger_cpu, "error al recibir la entrada de pagina de segundo nivel de memoria. error:[%d] msg:[%s]",
					handler->error, errortostr(handler->error));

			/*
			if (self->memoria->error == SKT_CONNECTION_LOST)
				kernel->error();
			*/
		}
		else if (package->header != MC_BUSCAR_MARCO) {
			log_error(logger_cpu, "error al recibir el marco de memoria. Codigo de header:[%d] incorrecto", package->header);
		}
		else {
			frame = deserialize_uint32t(package->payload);
		}

		buffer_destroy(package->payload);
		package_destroy(package);
	}

	sem_post(&sem_memory_request);

	return frame;
}

