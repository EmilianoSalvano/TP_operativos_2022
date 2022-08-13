/*
 * estructuras.h
 *
 *  Created on: 29 may. 2022
 *      Author: utnso
 */

#ifndef INCLUDE_ESTRUCTURAS_H_
#define INCLUDE_ESTRUCTURAS_H_

#include <time.h>
#include <commons/collections/list.h>
#include "socket_handler.h"
#include "timer.h"




/*
usar void *calloc(size_t nitems, size_t size) para reservar memoria para cadenas de texto porque las inicializa en "/0"

char** es un puntero a puntero char. Se puede ver como un array de cadenas o array de array de char
para asignar valores

int rows = 3;
char **c = calloc(rows,sizeof(char*));
c[0] = "cat";
c[1] = "dog";
c[2] = "mouse";

la signacion c[i] = "un valor"; agrega el fin de linea "/0" automaticamente. por lo tanto se estan utilizando
9 espacios de tamanio char
*/


typedef enum {
    ci_NO_OP = 0,
	ci_I_O,
	ci_WRITE,
	ci_COPY,
	ci_READ,
	ci_EXIT,
} t_codigo_instruccion;


typedef enum {
	PCB_NEW,
	PCB_READY,
	PCB_BLOCKED,
	PCB_EXEC,
	PCB_EXIT,
	PCB_SUSPENDED_READY,
	PCB_SUSPENDED_BLOCKED
} t_process_status;


typedef enum {
	CPU_NULL_CONDITION,
	CPU_EXIT_PROGRAM,
	CPU_INTERRUPTED_BY_KERNEL,
	CPU_IO_SYSCALL
} t_dispatch_cond;


typedef enum {
	ti_SOLICITUD_DESALOJO,
	ti_ACCESO_INVALIDO_MEMORIA,
	ti_INSTRUCCION_INVALIDA
} t_tipo_interrupcion;


typedef struct {
	t_codigo_instruccion	codigo;
	t_list*					parametros;
} t_instruction;


typedef struct {
	int			size;
	t_list*		instructions;
} t_program;


typedef struct {
	int					id;
	t_process_status	status;
	t_process_status	prev_status;					// este estado solo se utiliza para algoritmos de planificacion con desalojo
													// la idea es conservar el estado anterior a READY, para luego priorizar cuando se incluyan en esta lista
	int					process_size;
	int					program_counter;
	int					tabla_paginas;
	t_list*				instructions;
	t_socket_handler*	socket_handler;

	// parametros de ejecucion
	// las rafagas estan en milisegundos
	t_dispatch_cond		cpu_dispatch_cond;			// indica porque se desaloja del CPU
	int					cpu_estimated_burst;		// estimacion de la proxima rafaga
	int					cpu_executed_burst;			// duracion real de la rafaga ejecurada en CPU
	int					io_burst;					// duracion de la rafaga de E/S
	t_timer*			timer_blocked;				// timer para ejecutar el tiempo de bloqueo
	t_timer*			timer_suspended_blocked;	// timer para controlar el tiempo de permanencia maximo en bloqueado

	// estadisticas
	bool				error;						// indica si el pcb se ejecuto con error
	int					cpu_execution_count;		// cantidad de veces que utilizo el cpu
	int					total_cpu_burst_elapsed;	// tiempo total de utilizacion de cpu
	int					io_execution_count;			// cantidad de veces que ejecuto en E/S
	int					total_io_burst_elapsed;		// tiempo total de ejecucion de E/S

	uint64_t			start_time;					// momento de inicio de proceso (time stamp)

} t_pcb;


// para operaciones de lectura y escritura en memoria
typedef struct {
	uint32_t	pid;
	uint32_t 	numero_marco;
	uint32_t 	desplazamiento;
	uint32_t 	dato;
} t_operacion_EU;

// para busqueda de marco.
// el cpu hace dos envios, primer nivel y segundo, y la memoria devuelve un entero (numero de marco)
typedef struct {
	uint32_t	pid;
	uint32_t 	tabla_primer_nivel;
	uint32_t 	tabla_segundo_nivel;
	uint32_t 	pagina;
} t_direccion;

// configuracion de la meoria que usa el cpu para la traduccion de direcciones
typedef struct {
	uint32_t	entradas_por_tabla;
	uint32_t	tam_pagina;
} t_setup_memoria;

// la utilza el kernel para solicitar a memoria el espacio para un nuevo pcb
typedef struct {
	uint32_t	pcb_id;
	uint32_t	process_size;
} t_page_entry_request;

/*************************************************************************************************/
/****************** Funciones utiles para mantenimiento de estructuras ***************************/
/*************************************************************************************************/


char* prcstatustostr(t_process_status  status);

char* dispatchcondtostr(t_dispatch_cond cond);

/************************ t_instruccion **********************************************************/
/*
 *	@NAME: instruccion_create
 *	@DESC: instancia una nueva instruccion
 */
t_instruction* instruccion_create(t_codigo_instruccion codigo, bool empty);

/*
 *	@NAME: instruccion_destroy
 *	@DESC: destruye los parametros asociados y a la instruccion misma
 */
void instruccion_destroy(t_instruction* self);

/*
 *	@NAME: instruccion_destroy_in_list
 *	@DESC: destruye solo a los parametros asociados. La instruccion se destruye en el codigo de t_list. Utilizar esta funcion solo con listas
 */
void instruccion_destroy_in_list(void* instruccion);


/*
 *	@NAME: instruccion_to_string
 *	@DESC: serializa la estructura en un string
 */
char* instruccion_to_string(t_instruction* self);

/*
 *	@NAME: instruccion_list_to_string
 *	@DESC: serializa la lista de instrucciones en un string
 */
char* instruccion_list_to_string(t_list* instrucciones);


/*
 *	@NAME: param_destroy_in_list
 *	@DESC:
 */
void param_destroy_in_list(void* param);

/************************ t_pcb **********************************************************/
/*
 *	@NAME: pcb_create
 *	@DESC: crear y devuelve un nuevo pcb inicializado
 */
t_pcb* pcb_create(t_list* instructions);

/*
 *	@NAME: pcb_destroy
 *	@DESC: destruye a la lista de instrucciones asociadas y luego al pcb
 */
void pcb_destroy(t_pcb* self);

/*
 *	@NAME: pcb_destroy_in_list
 *	@DESC: callback para liberar el pcb contenido en una lista
 */
void pcb_destroy_in_list(void* pcb);

/*
 *	@NAME: pcb_to_string
 *	@DESC: serializa la estructura en un string
 */
char* pcb_to_string(t_pcb* self);


/*
 *	@NAME: produce_pcb
 *	@DESC: produce un pcb random
 */
t_pcb* produce_pcb();


/*
 *	@NAME: produce_instruccion
 *	@DESC: produce una instruccion random
 */
t_instruction* produce_instruccion();


t_program* program_create(t_list* instructions);
void program_destroy(t_program* self);
char* program_to_string(t_program* self);
t_program* produce_program();


t_setup_memoria* setup_memoria_create();
void setup_memoria_destroy(t_setup_memoria* self);

t_operacion_EU* operacion_eu_create();
void operacion_eu_destroy(t_operacion_EU* self);

t_direccion* direccion_create();
void direccion_destroy(t_direccion* self);

t_page_entry_request* page_entry_request_create();
void page_entry_request_destroy(t_page_entry_request* self);


#endif /* INCLUDE_ESTRUCTURAS_H_ */



