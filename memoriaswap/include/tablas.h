#ifndef TABLAS_H_
#define TABLAS_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <commons/string.h>
#include <unistd.h>

#include "../include/configuracion.h"
#include "../../shared/include/list.h"

extern List* lista_tablas_de_paginas;
extern List* lista_tablas_de_tablas;


typedef struct{
	uint32_t numero_marco;

	bool modificado;
	bool uso;
	bool presencia;
}t_pagina;

typedef struct{
	uint32_t numero_pagina; 
	t_pagina* pagina_asignada;
}t_registros_asignada;


//Tabla de tablas
typedef struct{
	List* tablas_segundo_nivel;
	List* paginas_asignadas;
	uint32_t puntero_marcos;
	uint32_t pcb_id;
}t_tabla_primer_nivel;


//Tabla de paginas
typedef struct{
	List* lista_paginas;
}t_tabla_segundo_nivel;




void crear_tabla_paginas(List* lista, uint32_t cantidadPaginas);

uint32_t crear_tabla_primer_nivel(uint32_t tam_proceso, uint32_t id_proceso);

uint32_t numero_de_tabla(uint32_t numero);

uint32_t escribir_marco(uint32_t numero_marco, uint32_t desplazamiento, uint32_t dato);

void leer_marco(uint32_t numero_marco, uint32_t desplazamiento, uint32_t* dato);




//SWAP quiza convenga meterlo en otro archivo.

char* numero_tabla_a_path(uint32_t numero_tabla_tablas);

void crear_archivo_swap(uint32_t numero_tabla, uint32_t tam_proceso);

void escribir_pagina_swap(uint32_t numero_tabla_tablas, uint32_t numpero_tabla_paginas, uint32_t numero_pagina, void* dato);

void* leer_pagina_swap(uint32_t numero_tabla_tablas, uint32_t numpero_tabla_paginas, uint32_t numero_pagina);

void eliminar_archivo_swap(uint32_t numero_tabla);

void* seleccionar_pagina_victima_clock(uint32_t numero_tabla_tablas);

void* seleccionar_pagina_victima_clock_M(uint32_t numero_tabla_tablas);


uint32_t escribir_en_pagina(uint32_t numero_tabla_tablas, uint32_t numero_pagina, uint32_t desplazamiento, uint32_t dato);

uint32_t leer_en_pagina(uint32_t numero_tabla_tablas, uint32_t numero_pagina, uint32_t desplazamiento);

bool suspender_proceso(uint32_t numero_tabla_tablas);

bool finalizar_proceso(uint32_t numero_tabla_tablas);

uint32_t buscar_marco_de_pagina(uint32_t numero_tabla_tablas, uint32_t numero_pagina);

uint32_t iniciar_nuevo_proceso(uint32_t tam_proceso, uint32_t id_proceso);

void borrar_tablas_proceso(uint32_t numero_tabla_tablas);

#endif