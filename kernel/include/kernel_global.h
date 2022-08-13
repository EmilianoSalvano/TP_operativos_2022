/*
 * servicios.h
 *
 *  Created on: 28 may. 2022
 *      Author: utnso
 */

#ifndef KERNEL_GLOBAL_H_
#define KERNEL_GLOBAL_H_


#include <stdlib.h>
#include <commons/log.h>
//#include <commons/config.h>
//#include <commons/string.h>

//#include "../../shared/include/conexiones.h"
#include "../../shared/include/kiss.structs.h"
#include "../../shared/include/socket_handler.h"
//#include "../../shared/include/slog.h"
#include "../../shared/include/squeue.h"
#include "../../shared/include/thread_monitor.h"



#define NOMBRE_PROCESO "kernel"
#define ARCHIVO_LOG "kernel.log"
#define ARCHIVO_CONFIG "kernel.config"

#define KEY_IP_MEMORIA "IP_MEMORIA"
#define KEY_PUERTO_MEMORIA "PUERTO_MEMORIA"
#define KEY_IP_CPU "IP_CPU"
#define KEY_PUERTO_CPU_DISPATCH "PUERTO_CPU_DISPATCH"
#define KEY_PUERTO_CPU_INTERRUPT "PUERTO_CPU_INTERRUPT"
#define KEY_PUERTO_ESCUCHA "PUERTO_ESCUCHA"
#define KEY_ALGORITMO_PLANIFICACION "ALGORITMO_PLANIFICACION"
#define KEY_ESTIMACION_INICIAL "ESTIMACION_INICIAL"
#define KEY_ALFA "ALFA"
#define KEY_GRADO_MULTIPROGRAMACION "GRADO_MULTIPROGRAMACION"
#define KEY_TIEMPO_MAXIMO_BLOQUEADO "TIEMPO_MAXIMO_BLOQUEADO"

#define CFG_VAL_ALGORITMO_FIFO "FIFO"
#define CFG_VAL_ALGORITMO_SRT "SRT"



typedef enum {
	pln_FIFO,
	pln_SRT
} t_pln_algorithm;


typedef enum {
	MS_NOT_INITIALIZED,
	MS_INITIALIZED,
	MS_ERROR
} t_module_status;


typedef struct {
	char*		ip_memoria;
	char*		puerto_memoria;
	char*		ip_cpu;
	char*		puerto_cpu_dispatch;
	char*		puerto_cpu_interrupt;
	char*		puerto_escucha;				// servidor de consola
	t_pln_algorithm	algoritmo_planificacion;
	double		estimacion_inicial;
	double		alfa;
	int			grado_multiprogramacion;
	int			tiempo_maximo_bloqueado;
	bool		loaded;
} t_kernel_config;


typedef struct {
	t_socket_handler*	server;			// servidor para atencion de conexiones de consolas
	//t_list*				clients;		// lista de socket_handlers conectados
} t_console_module;


typedef struct {
	t_socket_handler*	socket_handler;		// conexion al modulo de memoria
	pthread_mutex_t		mutex_access;		// mutex para cceso secuencial a memoria
} t_memory_module;


typedef struct {
	t_socket_handler*	interrupt;		// conexion al modulo CPU - solo para interrupciones
	t_socket_handler*	dispatch;		// conexion al modulo CPU - paso de PCB (ida y vuelta)
	bool				busy;			// indica si el cpu esta en uso
	bool				interrupted;	// indica que el cpu ha sido interrupido por el kernel
} t_cpu_module;


typedef struct {
	t_pcb*			exec;
	t_squeue* 		new;
	t_squeue*		ready;
	t_squeue*		blocked;			// la cola de bloqueados contiene a los procesos BLOCKED y SUSPENDED_BLOCKED
	t_squeue*		suspended_ready;
	t_squeue* 		exit;

	// planificador de largo plazo
	pthread_t		ltp_new_controller_tid;			// hilo controlador de planificador de procesos en estado NEW y SUSPENDED_READY
	pthread_t		ltp_exit_controller_tid;		// hilo controlador de planificador de procesos en EXIT
	pthread_mutex_t ltp_mutex_pcb_id;				// controla el acceso a la generacion de un nuevo id de pcb
	sem_t 			ltp_sem_plan;					// semaforo contador de procesos en la cola READY + SUSPENDED_READY
	sem_t 			ltp_sem_exit;					// semaforo contador de procesos en la cola EXIT
	sem_t 			ltp_sem_multiprogramming_level;	// grado de multiprogramacion de procesos en ejecucion (EXEC + READY + BLOCKED)

	// planificador de mediano plazo
	pthread_t		mtp_blocked_controller_tid;		// hilo controlador de planificador de procesos BLOCKED, SUSPENDED_BLOCKED y SUSPENDED_READY
	sem_t 			mtp_sem_planner;				// semaforo contador de procesos en la cola BLOCKED + SUSPENDED_BLOCKED
	sem_t			mtp_sem_io;						// semaforo que sertializa la entrada de procesos bloqueados al dispositivo de E/S
	pthread_mutex_t	mtp_mutex_blocking_process;		// mutex para proteger la ejecucion de codigo de bloque y desbloqueo de procesos en E/S

	// planificador de corto plazo
	pthread_t		stp_controller_tid;				// hilo controlador de planificador de procesos en estado READY
	sem_t			stp_sem_plan;					// semaforo contador de procesos en la cola READY
	//sem_t			stp_sem_next_pcb;				// semaforo para serializar la ejecucion de procesos en el CPU
	//sem_t			stp_sem_interrupt;				// semaforo para sincronizar lña interrupcion
	sem_t			stp_sem_dispatch;				// semaforo que sincroniza el envio y retorno del dispatch
	//sem_t			stp_cpu_busy;					// semaforo que indica que el cpu esta en uso
	//sem_t			stp_sem_interrupt_queue;		// cola de eventos de interrupciones
	pthread_mutex_t	stp_mutex_ready;
	sem_t			stp_sem_interrupt;
	//int				stp_interrupt_counter;

	void			(*move_pcb_to_ready)(t_pcb*);	// metodo para enviar un proceso a la cola READY. Determina que algoritmo utilizar, segun la configuracion
	void 			(*incomming_dispatch)(t_pcb*);	// metodo para recibir un proceso por dispatch. Determina que algoritmo utilizar, segun la configuracion
	void			(*dispatch_error)();			// metodo para informar un error de dispatch. Determina que algoritmo utilizar, segun la configuracion
} t_planner;


typedef struct {
	t_console_module*	console;
	t_memory_module*	memory;
	t_cpu_module*		cpu;
	t_planner*			planner;

	t_kernel_config*	config;						// configuracion del modulo kernel
	bool				halt;						// indica el cierre del proceso
	t_thread_monitor*	monitor;					// monitor de hilos

	void				(*error)();
} t_kernel;


// variables globales
extern t_log* logger;
extern t_kernel* kernel;



#endif /* KERNEL_GLOBAL_H_ */
