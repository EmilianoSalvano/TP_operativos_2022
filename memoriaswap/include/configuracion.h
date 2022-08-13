#ifndef CONFIGURACIOn_H_
#define CONFIGURACIOn_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <semaphore.h>

#include <commons/log.h>
#include <commons/config.h>

typedef struct{
	char* puerto_escucha;
	uint32_t tam_memoria;
	uint32_t tam_pagina;
	uint32_t entradas_por_tabla;
	uint32_t retardo_memoria;
	char* algoritmo_reemplazo;
	uint32_t marcos_por_proceso;
	uint32_t retardo_swap;
	char* path_swap;
}t_config_memoria;


typedef struct{
	bool asignado;
	uint32_t tabla_tablas;
	uint32_t pagina;
}registro_EU;


extern t_config_memoria* configuracion_memoria;
extern void* espacioUsuario;
extern registro_EU* registro_espacioUsuario;
extern sem_t semaforo;
extern t_log* logger_memoria;

void configurar_memoria(char* path);


#endif