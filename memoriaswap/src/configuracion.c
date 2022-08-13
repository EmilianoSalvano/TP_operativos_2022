#include <string.h>

#include "../include/configuracion.h"
#include "../include/memoria_global.h"


t_config_memoria* configuracion_memoria; 		//variables globales para no vivir pasando datos por parametro.
void* espacioUsuario;
registro_EU* registro_espacioUsuario;
sem_t semaforo;


t_config_memoria* leer_configuracion_memoria(char* path) {
	t_config* configuracion = config_create(path);
	
	configuracion_memoria = malloc(sizeof(t_config_memoria));

	configuracion_memoria -> puerto_escucha = strdup(config_get_string_value(configuracion, "PUERTO_ESCUCHA"));
	configuracion_memoria -> tam_memoria = config_get_int_value(configuracion, "TAM_MEMORIA");
	configuracion_memoria -> tam_pagina = config_get_int_value(configuracion, "TAM_PAGINA");
	configuracion_memoria -> entradas_por_tabla = config_get_int_value(configuracion, "ENTRADAS_POR_TABLA");
	configuracion_memoria -> retardo_memoria = config_get_int_value(configuracion, "RETARDO_MEMORIA");
	configuracion_memoria -> algoritmo_reemplazo = strdup(config_get_string_value(configuracion, "ALGORITMO_REEMPLAZO"));
	configuracion_memoria -> marcos_por_proceso = config_get_int_value(configuracion, "MARCOS_POR_PROCESO");
	configuracion_memoria -> retardo_swap = config_get_int_value(configuracion, "RETARDO_SWAP");
	configuracion_memoria -> path_swap = strdup(config_get_string_value(configuracion, "PATH_SWAP"));

	config_destroy(configuracion);

	return configuracion_memoria;
}


void configurar_memoria(char* path){

	leer_configuracion_memoria(path);

	espacioUsuario = malloc(configuracion_memoria->tam_memoria);

	uint32_t cantidad = configuracion_memoria->tam_memoria/configuracion_memoria->tam_pagina;

	registro_espacioUsuario = malloc(sizeof(registro_EU)*cantidad);

	for(uint32_t i = 0; i<cantidad; i++){
		(registro_espacioUsuario + i)->asignado = false;
	}

	sem_init(&semaforo, 0, 1);
}
