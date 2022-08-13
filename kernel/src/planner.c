/*
 * planificador.c
 *
 *  Created on: 28 may. 2022
 *      Author: utnso
 */

#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <commons/log.h>

#include <commons/collections/list.h>

#include "../../shared/include/kiss.structs.h"
#include "../../shared/include/squeue.h"
#include "../../shared/include/timer.h"
#include "../../shared/include/kiss.error.h"
#include "../../shared/include/time_utils.h"

#include "../include/planner.h"
#include "../include/iconsole.h"
#include "../include/icpu.h"
#include "../include/imemory.h"


t_kernel* kernel;
t_planner* planner;


// *********** Declaracion de meotodos privados *********** /
// manejo de colas
static void move_pcb_to_ready__fifo(t_pcb* pcb);
static void move_pcb_to_ready__srt(t_pcb* pcb);
static void move_pcb_to_exec(t_pcb* pcb);
static void move_pcb_to_new(t_pcb* pcb);
static void move_pcb_to_exit(t_pcb* pcb);
static void move_pcb_to_blocked(t_pcb* pcb);
static void move_pcb_to_suspended_blocked(t_pcb* pcb);
static void move_pcb_to_suspended_ready(t_pcb* pcb);
static void stp_print_queue(t_squeue* q, t_pcb* pcb);

static int compare_ready_pcb__str(void* pcb1, void* pcb2);
static bool compare_pcb_by_id(void* pcb1, void* pcb2);
static bool compare_pcb_by_socket(void* pcb1, void* pcb2);

// planificador de largo plazo
static int ltp_planner_start();
static void ltp_new_controller();
static void ltp_exit_controller();
static void ltp_print_stats(t_pcb* pcb);

//planificador de mediano plazo
static int mtp_planner_start();
static void mtp_blocked_controller();
static int mtp_block_pcb(t_pcb* pcb);
static void mtp_unblock_pcb(void* vpcb);
static void mtp_plan_pcb(t_pcb* pcb);
static int mtp_set_suspend_timer(t_pcb* pcb, int milliseconds);
static void mtp_suspend_pcb(void* vpcb);

// planificador de corto plazo
static int stp_planner_start();
static void stp_srt_controller();
//static void stp_srt_interrupt_controller();
static void stp_fifo_controller();
static void stp_incomming_dispatch_fifo(t_pcb* pcb_dispatch);
static void stp_incomming_dispatch_str(t_pcb* pcb_dispatch);
static void stp_dispatch_error();


// otros
static int get_next_pcb_id();


// ****************************************************************************************** //
// ********************************* PUBLIC ************************************************* //
// ****************************************************************************************** //


t_planner* planner_create() {
	planner = malloc(sizeof(t_planner));

	planner->exec = NULL;
	planner->new = squeue_create();
	planner->ready = squeue_create();
	planner->blocked = squeue_create();
	planner->suspended_ready = squeue_create();
	planner->exit = squeue_create();

	// planificador de largo plazo
	pthread_mutex_init(&(planner->ltp_mutex_pcb_id), NULL);
	sem_init(&(planner->ltp_sem_plan), 0, 0);
	sem_init(&(planner->ltp_sem_exit), 0, 0);
	sem_init(&(planner->ltp_sem_multiprogramming_level), 0, kernel->config->grado_multiprogramacion);


	// planificador de mediano plazo
	sem_init(&(planner->mtp_sem_planner), 0, 0);
	sem_init(&(planner->mtp_sem_io), 0, 1);
	pthread_mutex_init(&(planner->mtp_mutex_blocking_process), NULL);

	// planificador de corto plazo
	sem_init(&(planner->stp_sem_plan), 0, 0);
	//sem_init(&(planner->stp_sem_next_pcb), 0, 1);
	sem_init(&(planner->stp_sem_interrupt), 0, 1);
	//sem_init(&(planner->stp_sem_interrupt_queue), 0, 0);
	sem_init(&(planner->stp_sem_dispatch), 0, 1);
	pthread_mutex_init(&(planner->stp_mutex_ready), NULL);

	//planner->stp_interrupt_counter = 0;

	return planner;
}

void planner_destroy() {

	if (planner->exec != NULL)
		pcb_destroy(planner->exec);

	squeue_destroy_and_destroy_elements(planner->new, pcb_destroy_in_list);
	squeue_destroy_and_destroy_elements(planner->ready, pcb_destroy_in_list);
	squeue_destroy_and_destroy_elements(planner->blocked, pcb_destroy_in_list);
	squeue_destroy_and_destroy_elements(planner->suspended_ready, pcb_destroy_in_list);
	squeue_destroy_and_destroy_elements(planner->exit, pcb_destroy_in_list);


	// planificador de largo plazo
	pthread_mutex_destroy(&(planner->ltp_mutex_pcb_id));
	sem_destroy(&(planner->ltp_sem_plan));
	sem_destroy(&(planner->ltp_sem_exit));
	sem_destroy(&(planner->ltp_sem_multiprogramming_level));


	// planificador de mediano plazo
	sem_destroy(&(planner->mtp_sem_planner));
	sem_destroy(&(planner->mtp_sem_io));
	pthread_mutex_destroy(&(planner->mtp_mutex_blocking_process));

	// planificador de corto plazo
	sem_destroy(&(planner->stp_sem_plan));
	//sem_destroy(&(planner->stp_sem_next_pcb));
	sem_destroy(&(planner->stp_sem_interrupt));
	//sem_destroy(&(planner->stp_sem_interrupt_queue));
	sem_destroy(&(planner->stp_sem_dispatch));
	pthread_mutex_destroy(&(planner->stp_mutex_ready));


	free(planner);
}


int planner_run() {
	if (planner == NULL) {
		log_warning(logger, "No se puede iniciar el planificador porque aun no fue creado");
		return EXIT_FAILURE;
	}

	log_info(logger, "Iniciando planificador ...");

	switch (kernel->config->algoritmo_planificacion) {
	case pln_FIFO:
		planner->move_pcb_to_ready = move_pcb_to_ready__fifo;
		planner->incomming_dispatch = stp_incomming_dispatch_fifo;

		break;
	case pln_SRT:
		planner->move_pcb_to_ready = move_pcb_to_ready__srt;
		planner->incomming_dispatch = stp_incomming_dispatch_str;
		break;
	default:
		log_error(logger, "El algoritmo de planificacion de corto plazo no esta definido");
		return EXIT_FAILURE;
	}

	planner->dispatch_error = stp_dispatch_error;


	if (ltp_planner_start() != 0) {
		log_error(logger, "Error iniciando el planificador");
		return EXIT_FAILURE;
	}

	if (mtp_planner_start() != 0) {
		log_error(logger, "Error iniciando el planificador");
		return EXIT_FAILURE;
	}

	if (stp_planner_start() != 0) {
		log_error(logger, "Error iniciando el planificador");
		return EXIT_FAILURE;
	}

	//log_info(logger, "Planificador iniciado");
	return EXIT_SUCCESS;
}


int planner_stop() {

	// envio la señal a cada semaforo y dejo que evalue el kernel->halt

	pthread_mutex_unlock(&(planner->ltp_mutex_pcb_id));
	sem_post(&(planner->ltp_sem_plan));
	sem_post(&(planner->ltp_sem_exit));
	sem_post(&(planner->ltp_sem_multiprogramming_level));


	// planificador de mediano plazo
	sem_post(&(planner->mtp_sem_planner));
	sem_post(&(planner->mtp_sem_io));
	pthread_mutex_unlock(&(planner->mtp_mutex_blocking_process));

	// planificador de corto plazo
	sem_post(&(planner->stp_sem_plan));
	//sem_post(&(planner->stp_sem_next_pcb));
	//sem_post(&(planner->stp_interrupt));
	sem_post(&(planner->stp_sem_dispatch));

	//sem_getvalue()  devuelve la cantidad de hilos esperando

	return EXIT_SUCCESS;
}



// recibe la entrada de consola
void ltp_new_process(t_socket_handler* handler, t_program* program) {

	t_pcb* pcb = pcb_create(program->instructions);
	pcb->id = get_next_pcb_id();
	pcb->cpu_estimated_burst = kernel->config->estimacion_inicial;
	pcb->process_size = program->size;
	pcb->tabla_paginas = -1;
	pcb->socket_handler = handler;
	pcb->start_time = get_time_stamp();

	log_info(logger, "pcb:[%d] :: nuevo proceso ingresado al planificador de largo plazo. process-size:[%d]",
						pcb->id, pcb->process_size);

	move_pcb_to_new(pcb);
	program->instructions = NULL;
}



// ****************************************************************************************** //
// ********************************* PRIVATE ************************************************ //
// ****************************************************************************************** //


// ******* 	Manejo de colas	******** //

/*
 * tiene que cumplir con el orden
 * 1. clock (exec)
 * 2. finalizacion de evento (blocked)
 * 3. proceso suspended_ready/new
 *
 * devuelve:
 * 1: cuando pcb1 tiene mayor prioridad que pcb2
 * 0: cuando pcb1 es igual a pcb2 (no lo utilizo aca)
 * -1: cuando pcb2 tiene mayor prioridad que pcb1
 *
 * pcb1: es el nuevo pcb que se intenta insertar en  la lista
 * pcb2: un pcb de la lista, tomado desde el inicio de la cola
 *
 */
static int compare_ready_pcb__str(void* pcb1, void* pcb2) {
	t_pcb* new_pcb = (t_pcb*)pcb1;
	t_pcb* list_pcb = (t_pcb*)pcb2;

	log_debug(logger, "compare_ready_pcb__str :: comparo pcb1:[%d] burst:[%d] || pcb2:[%d] burst:[%d]", new_pcb->id, new_pcb->cpu_estimated_burst, list_pcb->id, list_pcb->cpu_estimated_burst);

	if (new_pcb->cpu_estimated_burst < list_pcb->cpu_estimated_burst) {
		log_debug(logger, "compare_ready_pcb__str :: pcb1:[%d] burst:[%d] < pcb2:[%d] burst:[%d]", new_pcb->id, new_pcb->cpu_estimated_burst, list_pcb->id, list_pcb->cpu_estimated_burst);
		return 1;
	}
	else if (new_pcb->cpu_estimated_burst == list_pcb->cpu_estimated_burst) {
		log_debug(logger, "compare_ready_pcb__str :: pcb1:[%d] burst:[%d] = pcb2:[%d] burst:[%d]", new_pcb->id, new_pcb->cpu_estimated_burst, list_pcb->id, list_pcb->cpu_estimated_burst);
		// cuando son iguales:
		// 1. el que sale de EXEC tiene mayor prioridad
		// 2. el que viene de BLOCKED tiene mayor prioridad de un NEW o SUSPENDED_READY pero menos que un anterior EXEC o BLOCKED (uno que ya estaba en la lista)
		// 3. el que viene de NEW o SUSPENDED_READY es el que menor prioridad tiene
		if (new_pcb->status == PCB_EXEC) {
			log_debug(logger, "compare_ready_pcb__str :: pcb1:[%d] status EXEC", new_pcb->id);
			return 1;
		}
		else if (new_pcb->status == PCB_BLOCKED) {
			log_debug(logger, "compare_ready_pcb__str :: pcb1:[%d] status BLOCKED", new_pcb->id);
			// substatus : es el estado previo a ingresar a READY
			if (list_pcb->prev_status == PCB_EXEC || list_pcb->prev_status == PCB_BLOCKED)
				return -1;
			else
				return 1;
		}
		else // NEW, SUSPENDED_READY
		{
			log_debug(logger, "compare_ready_pcb__str :: pcb1:[%d] status NEW", new_pcb->id);
			return -1;
		}
	}
	else {
		log_debug(logger, "compare_ready_pcb__str :: pcb1:[%d] burst:[%d] > pcb2:[%d] burst:[%d]", new_pcb->id, new_pcb->cpu_estimated_burst, list_pcb->id, list_pcb->cpu_estimated_burst);
		return -1;
	}
}




static void stp_print_queue(t_squeue* q, t_pcb* pcb) {
	pthread_mutex_lock(&(q->mutex));

	log_debug(logger, "**************** READY ***********************");
	log_debug(logger, "pcb:[%d] entrante", pcb->id);

	t_list_iterator* iterator = list_iterator_create(q->queue->elements);

	t_pcb* aux;
	while(list_iterator_has_next(iterator)) {
		aux = list_iterator_next(iterator);
		log_debug(logger, "pcb:[%d] cpu_estimated_burst:[%d]", aux->id, aux->cpu_estimated_burst);
	}

	list_iterator_destroy(iterator);
	log_debug(logger, "**********************************************");

	pthread_mutex_unlock(&(q->mutex));
}



// inserta el pcb segun el orden definido en la funcion de comparacion
static void move_pcb_to_ready__srt(t_pcb* pcb) {
	//pthread_mutex_lock(&(planner->stp_mutex_ready));

	log_info(logger, "pcb:[%d] :: [%s] => [READY], con sub-estado:[%s]", pcb->id, prcstatustostr(pcb->status), prcstatustostr(pcb->prev_status));

	squeue_insert_in_order(planner->ready, (void*)pcb, compare_ready_pcb__str);
	stp_print_queue(planner->ready, pcb);

	pcb->prev_status = pcb->status;
	pcb->status = PCB_READY;

	//pthread_mutex_unlock(&(planner->stp_mutex_ready));

	if (pcb->status != PCB_EXEC && kernel->cpu->busy) {
		log_debug(logger, "move_pcb_to_ready__srt :: pcb:[%d] wait :: stp_sem_interrupt", pcb->id);
		sem_wait(&(planner->stp_sem_interrupt));
		log_debug(logger, "move_pcb_to_ready__srt :: pcb:[%d] interrupcion", pcb->id);
		cpu_interrupt();
	}

	log_debug(logger, "move_pcb_to_ready__srt :: incremento stp_sem_plan");
	sem_post(&(planner->stp_sem_plan));

	//pthread_mutex_unlock(&(planner->stp_mutex_ready));
}

// insrta el nuevo pcb al final de la lista
static void move_pcb_to_ready__fifo(t_pcb* pcb) {
	log_info(logger, "pcb:[%d] :: [%s] => [READY]", pcb->id, prcstatustostr(pcb->status));

	pcb->status = PCB_READY;
	squeue_push(planner->ready, pcb);
	sem_post(&(planner->stp_sem_plan));
}

static void move_pcb_to_exec(t_pcb* pcb) {
	log_info(logger, "pcb:[%d] :: [%s] => [EXEC]", pcb->id, prcstatustostr(pcb->status));
	pcb->status = PCB_EXEC;
	planner->exec = pcb;
}

static void move_pcb_to_new(t_pcb* pcb) {
	log_info(logger, "pcb:[%d] :: [NULL] => [NEW]", pcb->id);

	pcb->status = PCB_NEW;
	squeue_push(planner->new, pcb);
	sem_post(&(planner->ltp_sem_plan));
}

static void move_pcb_to_exit(t_pcb* pcb) {
	log_info(logger, "pcb:[%d] :: [%s] => [EXIT]", pcb->id, prcstatustostr(pcb->status));

	if (pcb->status == PCB_EXEC || pcb->status == PCB_READY || pcb->status == PCB_BLOCKED) {
		log_debug(logger, "pcb:[%d] :: incrementa el grado de multiprogramacion por [EXIT]", pcb->id);
		sem_post(&(planner->ltp_sem_multiprogramming_level));
	}

	pcb->status = PCB_EXIT;
	squeue_push(planner->exit, pcb);
	sem_post(&(planner->ltp_sem_exit));
}

static void move_pcb_to_blocked(t_pcb* pcb) {
	log_info(logger, "pcb:[%d] :: [%s] => [BLOCKED]", pcb->id, prcstatustostr(pcb->status));

	pcb->status = PCB_BLOCKED;
	squeue_push(planner->blocked, pcb);
	mtp_plan_pcb(pcb);
}


static void move_pcb_to_suspended_blocked(t_pcb* pcb) {
	log_info(logger, "pcb:[%d] :: [%s] => [SUSPENDED-BLOCKED]", pcb->id, prcstatustostr(pcb->status));

	pcb->status = PCB_SUSPENDED_BLOCKED;
	sem_post(&(planner->ltp_sem_multiprogramming_level));
}


static void move_pcb_to_suspended_ready(t_pcb* pcb) {
	log_info(logger, "pcb:[%d] :: [%s] => [SUSPENDED-READY]", pcb->id, prcstatustostr(pcb->status));

	pcb->status = PCB_SUSPENDED_READY;
	squeue_push(planner->suspended_ready, pcb);
	sem_post(&(planner->ltp_sem_plan));
}

static bool compare_pcb_by_id(void* pcb1, void* pcb2) {
	return ((t_pcb*)pcb1)->id == ((t_pcb*)pcb2)->id;
}

static bool compare_pcb_by_socket(void* pcb1, void* pcb2) {
	return ((t_pcb*)pcb1)->socket_handler->socket == ((t_pcb*)pcb2)->socket_handler->socket;
}




// ******* 	Planificador de largo plazo	******** //

// se activa con:
// susp_blocked -> susp_ready
// null -> nuevo
// cuando disminuye el grado de multiprogramacion

static int ltp_planner_start() {
	int err;

	log_info(logger, "Iniciando planificador de largo plazo ...");

	if (pthread_create(&(planner->ltp_new_controller_tid), NULL, (void*)ltp_new_controller, NULL) != 0) {
		err = errno;
		log_error(logger, "No se pudo iniciar el planificador de largo plazo (new). error:[%d] msg:[%s]", err, errortostr(err));
		return EXIT_FAILURE;
	}

	pthread_detach(planner->ltp_new_controller_tid);


	if (pthread_create(&(planner->ltp_exit_controller_tid), NULL, (void*)ltp_exit_controller, NULL) != 0) {
		err = errno;
		log_error(logger, "No se pudo iniciar el planificador de largo plazo (exit). error:[%d] msg:[%s]", err, errortostr(err));
		return EXIT_FAILURE;
	}

	pthread_detach(planner->ltp_exit_controller_tid);

	//log_info(logger, "Planificador de largo plazo iniciado");
	return EXIT_SUCCESS;
}


static void ltp_new_controller() {
	t_pcb* pcb;

	log_info(logger, "[PLP] :: controlador de procesos 'NEW' y 'SUSPENDED_READY' iniciado");

	while(true) {
		// este tiene que ser activado cada ver que hay un evento null->new o susp_block -> susp_ready
		sem_wait(&planner->ltp_sem_plan);
		sem_wait(&planner->ltp_sem_multiprogramming_level);

		if (kernel->halt)
			break;

		if (!squeue_is_empty(planner->suspended_ready)) {
			pcb = squeue_pop(planner->suspended_ready);
		}
		else if (!squeue_is_empty(planner->new)) {
			pcb = squeue_pop(planner->new);
			pcb->tabla_paginas = get_entry_page_table(pcb);

			log_debug(logger, "pcb:[%d] :: tabla de paginas:[%d] asignada", pcb->id, pcb->tabla_paginas);

			if (pcb->tabla_paginas < 0) {
				log_error(logger, "pcb:[%d] :: fallo la asignacion de paginas", pcb->id);
				pcb->error = true;
				move_pcb_to_exit(pcb);

				// incremento el grado de multiprogramacion porque descarto el pcb
				log_debug(logger, "pcb:[%d] :: incremento el grado de multiprogramacion por fallo de asignacion de pagina", pcb->id);

				sem_post(&(planner->ltp_sem_multiprogramming_level));
				continue;
			}
		}
		else {
			log_warning(logger, "Ejecucion de controlador de procesos 'NEW' y 'SUSPENDED_READY' con cola de vacia");
			continue;
		}

		planner->move_pcb_to_ready(pcb);
	}

	log_info(logger, "[PLP] :: controlador de procesos 'NEW' y 'SUSPENDED_READY' detenido");
}


static void ltp_exit_controller() {

	log_info(logger, "[PLP] :: controlador de procesos 'EXIT' iniciado");

	while (true) {
		sem_wait(&planner->ltp_sem_exit);

		if (kernel->halt)
			break;

		if (squeue_is_empty(planner->exit)) {
			log_warning(logger, "Ejecucion de controlador de procesos 'EXIT' con cola de vacia");
			continue;
		}

		t_pcb* pcb = squeue_pop(planner->exit);

		// libero la memoria del proceso
		free_allocated_memory(pcb);

		// doy aviso a la consola
		if (pcb->error)
			console_terminate(pcb->socket_handler, SG_OPERATION_FAILURE);
		else
			console_terminate(pcb->socket_handler, SG_OPERATION_SUCCESS);


		ltp_print_stats(pcb);
		log_info(logger, "pcb:[%d] :: finalizado", pcb->id);

		pcb_destroy(pcb);
	}

	log_info(logger, "[PLP] :: Controlador de procesos 'EXIT' detenido");
}


static void ltp_print_stats(t_pcb* pcb) {
	const char* complete = "COMPLETADO";
	const char* error = "error";

	const char* format =
			"pcb:[%d] :: estadisticas de procesamiento\n"
			"> instrucciones: %d\n"
			"> program counter: %d\n"
			"> estado final: %s\n"
			"> uso de cpu: %d, tiempo total: %d msec\n"
			"> uso E/S: %d, tiempo total: %d msec\n"
			"> tiempo en ejecucion: %d msec\n"
			"> tiempo total de proceso: %d msec";

	uint64_t end = get_time_stamp();
	int pcb_total_time = end - pcb->start_time;
	int	pcb_exec_time = pcb->total_cpu_burst_elapsed + pcb->total_io_burst_elapsed;


	log_info(logger, format, pcb->id, list_size(pcb->instructions), pcb->program_counter, (pcb->error ? error : complete),
			pcb->cpu_execution_count, pcb->total_cpu_burst_elapsed, pcb->io_execution_count, pcb->total_io_burst_elapsed,
			pcb_exec_time, pcb_total_time);

	/*
	char* spcb = pcb_to_string(pcb);
	log_debug(logger, "pcb :: %s", spcb);
	free(spcb);
	*/
}


// ******* 	Planificador de mediano plazo	******** //

static int mtp_planner_start() {
	int err;

	log_info(logger, "Iniciando planificador de mediano plazo ...");

	if (pthread_create(&(planner->mtp_blocked_controller_tid), NULL, (void*)mtp_blocked_controller, NULL) != 0) {
		err = errno;
		log_error(logger, "No se pudo iniciar planificador de mediano plazo. error:[%d] msg:[%s]", err, errortostr(err));
		return EXIT_FAILURE;
	}

	pthread_detach(planner->mtp_blocked_controller_tid);

	//log_info(logger, "Planificador de mediano plazo iniciado");
	return EXIT_SUCCESS;
}


// la cola de bloqueados es FIFO, siempre va el que esta primero
// el metodo se aciva cuando:
//	1. entra un nuevo proceso a la cola de bloqueados
//	2. se desbloquea uno bloqueado
static void mtp_blocked_controller() {

	log_info(logger, "[PMP] :: controlador de procesos 'BLOCKED' y 'SUSPENDED_BLOCKED' iniciado");

	while (true) {
		sem_wait(&(planner->mtp_sem_planner));
		sem_wait(&(planner->mtp_sem_io));

		if (kernel->halt)
			break;

		t_pcb* pcb = squeue_pop(planner->blocked);

		if (mtp_block_pcb(pcb) != 0) {
			move_pcb_to_exit(pcb);
			sem_post(&(planner->mtp_sem_io));
		}
	}

	log_info(logger, "[PMP] :: controlador de procesos 'BLOCKED' y 'SUSPENDED_BLOCKED' detenido");
}


static void mtp_plan_pcb(t_pcb* pcb) {
	mtp_set_suspend_timer(pcb, kernel->config->tiempo_maximo_bloqueado);
	sem_post(&(planner->mtp_sem_planner));
}


static int mtp_block_pcb(t_pcb* pcb) {
	pcb->io_execution_count++;
	pcb->total_io_burst_elapsed += pcb->io_burst;

	t_timer* timer = timer_create_t(pcb->io_burst);
	timer->data = (void*)pcb;
	timer->callback = mtp_unblock_pcb;
	pcb->timer_blocked = timer;

	if (timer_start(timer) != 0) {
		log_error(logger, "pcb:[%d] :: Error al crear el hilo para proceso de E/S. error:[%d] msg:[%s]", pcb->id, timer->error, errortostr(timer->error));
		pcb->timer_blocked = NULL;
		timer_destroy(timer);
		return EXIT_FAILURE;
	}

	log_info(logger, "pcb:[%d] :: en ejecucion de E/S con duracion:[%d msec]", pcb->id, pcb->io_burst);

	return EXIT_SUCCESS;
}


static void mtp_unblock_pcb(void* vpcb) {
	pthread_mutex_lock(&(planner->mtp_mutex_blocking_process));

	sem_post(&(planner->mtp_sem_io));

	t_pcb* pcb = (t_pcb*)(vpcb);

	// cancelo el timer de suspendido, si es que todavia esta activo
	if (pcb->timer_suspended_blocked != NULL)
		timer_cancel(pcb->timer_suspended_blocked);

	pcb->timer_blocked = NULL;

	switch(pcb->status) {
	case PCB_BLOCKED:
		planner->move_pcb_to_ready(pcb);
		break;
	case PCB_SUSPENDED_BLOCKED:
		move_pcb_to_suspended_ready(pcb);
		break;
	default:
		pcb->error = true;
		log_error(logger, "pcb:[%d] :: con estado:[%d] erroneo para operacion de desbloqueo", pcb->id, pcb->status);
		move_pcb_to_exit(pcb);
	}

	pthread_mutex_unlock(&(planner->mtp_mutex_blocking_process));
}


static int mtp_set_suspend_timer(t_pcb* pcb, int milliseconds) {

	t_timer* timer = timer_create_t(milliseconds);
	timer->data = (void*)pcb;
	timer->callback = mtp_suspend_pcb;
	pcb->timer_suspended_blocked = timer;

	if (timer_start(timer) != 0) {
		log_error(logger, "pcb:[%d] :: error al crear el hilo para proceso de E/S. error:[%d] msg:[%s]", pcb->id, timer->error, errortostr(timer->error));
		pcb->timer_suspended_blocked = NULL;
		timer_destroy(timer);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}


static void mtp_suspend_pcb(void* vpcb) {

	pthread_mutex_lock(&(planner->mtp_mutex_blocking_process));

	t_pcb* pcb = (t_pcb*)(vpcb);
	pcb->timer_suspended_blocked = NULL;

	// Es un timer que quedo mal, no pudo ser cancelado o hubo un error en el paso del pcb a otra cola
	if (pcb->status != PCB_BLOCKED) {
		log_warning(logger, "pcb:[%d] :: intento de suspension con estado:[%d], El pcb no pertenece mas a la cola de bloqueados", pcb->id, pcb->status);
	}
	// Si no puedo paginar, el pcb se queda como bloqueado y no decremento el grado de multiprogramacion
	else if (swapp_process(pcb) == EXIT_SUCCESS) {
		move_pcb_to_suspended_blocked(pcb);
	}
	else {
		log_warning(logger, "pcb:[%d] :: fallo en operacion de swapping para suspension de proceso", pcb->id);
		pcb->error = true;
		move_pcb_to_exit(pcb);
	}

	pthread_mutex_unlock(&(planner->mtp_mutex_blocking_process));
}



// ******* 	Planificador de corto plazo	******** //

// planificador de corto plazo
// envia el pcb a la CPU
// se activa cuando llega algo de dispatch - que pasa cuando recien arranca??

static int stp_planner_start() {

	void (*controller)();
	int err;

	log_info(logger, "Iniciando planificador de corto plazo ...");


	switch (kernel->config->algoritmo_planificacion) {
	case pln_FIFO:
		controller = stp_fifo_controller;
		break;
	case pln_SRT:
		controller = stp_srt_controller;
		break;
	default:
		log_error(logger, "Error iniciando el planificador de corto plazo. Tipo de algoritmo de planificacion:[%d] invalido", kernel->config->algoritmo_planificacion);
		return EXIT_FAILURE;
	}

	if (pthread_create(&(planner->stp_controller_tid), NULL, (void*) controller, NULL) != 0) {
		err = errno;
		log_error(logger, "Error iniciando el planificador de corto plazo. error:[%d] msg:[%s]", err, errortostr(err));
		return EXIT_FAILURE;
	}

	if (pthread_detach(planner->stp_controller_tid) != 0) {
		err = errno;
		log_error(logger, "Error iniciando el planificador de corto plazo. error:[%d] msg:[%s]", err, errortostr(err));
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}



static void stp_srt_controller() {
	log_info(logger, "[PCP] :: planificador de corto plazo iniciado con algoritmo 'SRT'");

	while (!kernel->halt) {

		sem_wait(&(planner->stp_sem_plan));
		sem_wait(&(planner->stp_sem_dispatch));

		if (kernel->halt)
			break;

		t_pcb* pcb = squeue_pop(planner->ready);
		move_pcb_to_exec(pcb);
		cpu_dispatch_pcb(pcb);
		cpu_listen_dispatch_async();

		log_debug(logger, "envio pcb:[%d] a cpu", pcb->id);
	}

	log_info(logger, "[PCP] :: planificador de corto plazo detenido");
}


static void stp_incomming_dispatch_str(t_pcb* pcb_dispatch) {

	if (kernel->halt)
		return;

	log_info(logger, "pcb:[%d] :: procesando respuesta de dispatch en condicion de desalojo:[%s]",
			pcb_dispatch->id, dispatchcondtostr(pcb_dispatch->cpu_dispatch_cond));

	t_pcb* pcb = planner->exec;
	planner->exec = NULL;

	pcb->cpu_execution_count++;
	pcb->cpu_dispatch_cond = pcb_dispatch->cpu_dispatch_cond;
	pcb->program_counter = pcb_dispatch->program_counter;
	pcb->cpu_executed_burst = pcb_dispatch->cpu_executed_burst;
	pcb->io_burst = pcb_dispatch->io_burst;
	pcb->total_cpu_burst_elapsed += pcb_dispatch->cpu_executed_burst;

	// TODO: creo que si sale por interrupcion, tengo que calcular el estimado como lo que tenia - lo que proceso
	// si sale por otra razon se hace de nuevo el calculo de estimacion
	double next_estimated_burst = ((double)(pcb_dispatch->cpu_estimated_burst) * kernel->config->alfa) + ((double)(pcb_dispatch->cpu_executed_burst) * (1.0 - kernel->config->alfa));

	log_debug(logger, "pcb:[%d] cpu_estimated_burst:[%d] cpu_executed_burst:[%d] next_estimated_burst:[%f] alfa:[%f]",
			pcb->id, pcb_dispatch->cpu_estimated_burst, pcb_dispatch->cpu_executed_burst, next_estimated_burst, kernel->config->alfa);

	pcb->cpu_estimated_burst = (int)next_estimated_burst;

	switch (pcb->cpu_dispatch_cond) {
		case CPU_EXIT_PROGRAM:
			move_pcb_to_exit(pcb);
			break;
		case CPU_INTERRUPTED_BY_KERNEL:
			planner->move_pcb_to_ready(pcb);
			break;
		case CPU_IO_SYSCALL:
			move_pcb_to_blocked(pcb);
			break;
		default:
			pcb->error = true;
			log_error(logger, "pcb:[%d] :: recibido por dispatch con condicion:[%d] de desalojo incorrecta", pcb_dispatch->id, pcb_dispatch->cpu_dispatch_cond);
			move_pcb_to_exit(pcb);
			break;
	}

	pcb->cpu_dispatch_cond = CPU_NULL_CONDITION;
	pcb_destroy(pcb_dispatch);

	if (kernel->cpu->interrupted) {
		kernel->cpu->interrupted = false;

		log_debug(logger, "stp_incomming_dispatch_str :: pcb:[%d] post : stp_sem_interrupt", pcb->id);
		sem_post(&(planner->stp_sem_interrupt));
	}


	//log_debug(logger, "stp_incomming_dispatch_str :: pcb:[%d] post planner->stp_sem_dispatch", pcb->id);
	sem_post(&(planner->stp_sem_dispatch));
}


static void stp_fifo_controller() {
	log_info(logger, "[PCP] :: planificador de corto plazo iniciado con algoritmo 'FIFO'");

	while (!kernel->halt) {

		log_debug(logger, "wait :: planner->stp_sem_plan");
		sem_wait(&(planner->stp_sem_plan));

		log_debug(logger, "wait :: planner->stp_sem_dispatch");
		sem_wait(&(planner->stp_sem_dispatch)); // estaba comentado esto

		if (kernel->halt)
			break;

		t_pcb* pcb = squeue_pop(planner->ready);
		move_pcb_to_exec(pcb);
		cpu_dispatch_pcb(pcb);
		cpu_listen_dispatch_async();
	}

	log_info(logger, "[PCP] :: planificador de corto plazo detenido");
}


static void stp_incomming_dispatch_fifo(t_pcb* pcb_dispatch) {

	if (kernel->halt)
		return;

	log_info(logger, "pcb:[%d] :: procesando respuesta de dispatch en condicion de desalojo:[%s]",
			pcb_dispatch->id, dispatchcondtostr(pcb_dispatch->cpu_dispatch_cond));

	t_pcb* pcb = planner->exec;
	planner->exec = NULL;

	pcb->cpu_execution_count++;
	pcb->cpu_dispatch_cond = pcb_dispatch->cpu_dispatch_cond;
	pcb->program_counter = pcb_dispatch->program_counter;
	pcb->cpu_executed_burst = pcb_dispatch->cpu_executed_burst;
	pcb->io_burst = pcb_dispatch->io_burst;
	pcb->total_cpu_burst_elapsed += pcb_dispatch->cpu_executed_burst;

	switch (pcb->cpu_dispatch_cond) {
		case CPU_EXIT_PROGRAM:
			move_pcb_to_exit(pcb);
			break;
		case CPU_IO_SYSCALL:
			move_pcb_to_blocked(pcb);
			break;
		default:
			pcb->error = true;
			log_error(logger, "pcb:[%d] :: recibido por dispatch con condicion:[%d] de desalojo incorrecta", pcb_dispatch->id, pcb_dispatch->cpu_dispatch_cond);
			move_pcb_to_exit(pcb);
	}

	pcb->cpu_dispatch_cond = CPU_NULL_CONDITION;
	pcb_destroy(pcb_dispatch);

	log_debug(logger, "post :: planner->stp_sem_dispatch");
	sem_post(&(planner->stp_sem_dispatch));
}


static void stp_dispatch_error() {

	t_pcb* pcb = planner->exec;
	planner->exec = NULL;

	if (pcb != NULL) {
		log_error(logger, "pcb:[%d] :: recibido por dispatch con error", pcb->id);

		pcb->error = true;
		move_pcb_to_exit(pcb);
	}
	else {
		log_error(logger, "pcb:[%d] recibido por dispatch con error", pcb->id);
	}

	log_debug(logger, "post :: planner->stp_sem_dispatch");
	sem_post(&(planner->stp_sem_dispatch));
}



// ******* 	Otros	******** //

static int get_next_pcb_id() {
	static int pcb_id = 0;

	pthread_mutex_lock(&planner->ltp_mutex_pcb_id);
	int id = ++pcb_id;

	//log_debug(logger, "proximo pcb_id:[%d]", id);

	pthread_mutex_unlock(&planner->ltp_mutex_pcb_id);

	return id;
}
