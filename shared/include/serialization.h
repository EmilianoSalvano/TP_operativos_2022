/*
 * serializacion.h
 *
 *  Created on: 29 may. 2022
 *      Author: utnso
 */

#ifndef INCLUDE_SERIALIZACION_H_
#define INCLUDE_SERIALIZACION_H_


#include <stdint.h>
#include <commons/collections/list.h>

//#include "estructuras.h"
#include "kiss.global.h"


#define EMPTY_PACKAGE 0
#define ERROR_PACKAGE 1



typedef struct {
	uint32_t	size;
	void*		stream;
} t_buffer;


typedef struct {
	uint32_t	header;
	t_buffer* 	payload;
} t_package;




t_package*  package_create();
void package_destroy(t_package* self);

t_buffer*  buffer_create();
void buffer_destroy(t_buffer* self);


/*
 * @NAME: serializar_paquete
 * @DESC: serializa un buffer y agrega en la cabeza del stream el identificador de paquete
 *
 * Tipo de datos		Tamaño			Descripcion
 * uint32_t				4 bytes			Identificador de pauquete
 * uint32_t				4 bytes			Tamaño del buffer de datos (mensaje)
 * void*				variable		stream de datos del mensaje
 */
void* serializar_paquete(t_package* package, uint32_t *size);

/*
 * @NAME: deserializar_paquete
 * @DESC: deserializa un paquete de datos recibidos a traves de un stream
*/
t_package* deserializar_paquete(void* stream);

/*
 * @NAME: serializar_codigo
 * @DESC: serializa un codigo de tamaño uint32_t
*/
t_buffer* serialize_uint32t(uint32_t code);

/*
 * @NAME: deserializar_codigo
 * @DESC: deserializa un codigo de tamaño uint32_t
*/
uint32_t deserialize_uint32t(t_buffer* buffer);




#endif /* INCLUDE_SERIALIZACION_H_ */
