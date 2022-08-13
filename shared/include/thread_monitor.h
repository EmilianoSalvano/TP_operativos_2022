/*
 * hilo.h
 *
 *  Created on: 18 jun. 2022
 *      Author: utnso
 */

#ifndef INCLUDE_THREAD_MONITOR_H_
#define INCLUDE_THREAD_MONITOR_H_

#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <commons/collections/list.h>



/*
 * @NAME: t_thread_monitor
 * @DESC: la estructura guarda todos los atributos administrativos para monitorear una lista de hilos
 * 	Propiedades:
 * 	1. mutex_terminated: mutex que se desbloquea cuando todos los hilos han completado su ejecucion. El programador debe invocar al
 * 		metodo "terminate" antes de ejecutar un wait sobe el mutex, para que el monitor decida
 *
 */
typedef struct t_thread_monitor {
	// private
	pthread_mutex_t		mutex_monitor;						// private
	sem_t				sem_terminated;						// private

	// public
	int					active_thread_count;				// read only
	bool				terminated;							// read only
	t_list*				threads;							// read only

	int					(*subscribe)(struct t_thread_monitor* self, pthread_t tid);
	void 				(*unsubscribe)(struct t_thread_monitor* self, pthread_t tid);
	void				(*terminate_and_wait)(struct t_thread_monitor* self);
	int					(*new)(struct t_thread_monitor* self, pthread_t* tid, const pthread_attr_t *restrict attr, void *(*start_routine)(void *), void *restrict arg);
} t_thread_monitor;


/*
 * @NAME: thread_monitor_initialize
 * @DESC: crea e inicializa la estructura t_thread_monitor y los semaforos
 *
 */
t_thread_monitor* thread_monitor_create();


/*
 * @NAME: thread_monitor_finalize
 * @DESC: destruye los semaforos y la estructura. Destruye la lista de hilos pero no el contenido de cada nodo
 *
 */
void thread_monitor_destroy(t_thread_monitor* self);














// VIEJO


/*
typedef enum {
	ths_NEW,
	ths_RUNNING,
	ths_FINISHED,
	ths_CANCELED,
	ths_ERROR
} t_thread_status;

typedef struct t_data {
	void*	info;
	t_data*	next;
} t_data;

typedef struct t_thread_handler {
	pthread_t 			thread;
	t_thread_status		status;
	bool				terminate;
	bool				has_data;
	t_data*				data;			// esto puede ser cualquier informacion generada por el hilo
	bool				execute_on_terminate;
	void				(*on_terminate)(struct t_thread_handler*);	// funcion callback que se invoca al cuando el hilo completa su ejecucion
	//bool				execute_on_data;
	//void				(*on_data)(struct t_thread_handler*); // funcion callback que se invoca cada vez que el hilo necesite pasar informacion
} t_thread_handler;
*/

/*
 * @NAME: thread_handler_create
 * @DESC: crea e inicializa la estructura t_thread_handler
 *
 */
//t_thread_handler* thread_handler_create();

//void thread_handler_run(t_thread_handler* handler, t_thread_type type);

//void thread_handler_add_data(t_thread_handler* self, void* _data);

//void thread_handler_destroy_data(t_thread_handler* self);


/*
 * @NAME: thread_handler_destroy
 * @DESC: destruye y libera la memoria de la estructura t_thread_handler
 * 			Si el hilo se encuentra en estado ths_RUNNING, no realiza ninguna accion y devuelve un error
 * 			NO libera la memoria utilizada por la variable "data"
 *
 */
//int thread_handler_destroy(t_thread_handler* self);


#endif /* INCLUDE_THREAD_MONITOR_H_ */
