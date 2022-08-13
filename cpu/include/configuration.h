/*
 * config.h
 *
 *  Created on: 13 jul. 2022
 *      Author: utnso
 */

#ifndef CPU_CONFIGURATION_H_
#define CPU_CONFIGURATION_H_


typedef struct
{
	// CPU
	int 	retardo_NO_OP;
	char*	ip_memoria;
	char* 	puerto_memoria;
	char* 	puerto_escucha_dispath;
	char* 	puerto_escucha_interrupt;

	// TLB
	int 	cant_entradas_tlb;
	char*	algoritmo_reemplazo_tlb;
} t_config_cpu;


t_config_cpu * create_configuration(char* path);
void destroy_configuration(t_config_cpu* self);

#endif
