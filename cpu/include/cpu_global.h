/*
 * global.h
 *
 *  Created on: 20 jul. 2022
 *      Author: utnso
 */

#ifndef CPU_GLOBAL_H_
#define CPU_GLOBAL_H_


#include <commons/log.h>
#include <semaphore.h>

#include "../../shared/include/kiss.structs.h"
#include "../../shared/include/socket_handler.h"
#include "../../shared/include/time_utils.h"
#include "../include/configuration.h"


extern t_log* logger_cpu;


//structs de configuracion
/*
typedef struct
{
	int tamanio_pag;
	int cant_entradas;
	int id;
}t_config_memoria;
*/
// cambia por kiss.structs.t_setup_memoria

/*
typedef struct
{
	// CPU
	int 	retardo_NO_OP;
	char*	ip_memoria;
	char* 	puerto_memoria;
	char* 	puerto_escucha_dispath;
	char* 	puerto_escucha_interrumpt;

	// TLB
	int 	cant_entradas;
	char*	algoritmo_reemplazo;
} t_config_cpu;
*/
/*
typedef struct
{
	int cant_entradas;
	char* reemplazo;
} t_config_tlb;
*/

/*
typedef struct
{
	t_config_cpu config_cpu;
	t_config_tlb config_tlb;
} t_configuracion;
*/

//structs para MMU
/*
typedef struct
{
	t_tlb * TLB;
	int puerto_memoria; //socket_memoria
}t_mmu;

typedef struct
{
	int marco;
	int desplazamiento;
	int num_pag;
}t_dir_fisica;

typedef struct
{
	int num_pag;
	int primer_nivel;
	int segundo_nivel;
}t_datos_para_marco;
*/

//variables globales
//t_configuracion * configuracion;

t_config_cpu* config_cpu;
t_socket_handler* memoria;
sem_t sem_memory_request;



//t_config_memoria * config_memory;





//t_socket_handler * socket_memoria;
//t_socket_handler * socket_kernel;
//t_socket_handler * socket_interrupcion;

int hay_interrupcion;
int enviar_pcb_o_exit;

//test
#endif /* CPU_GLOBAL_H_ */
