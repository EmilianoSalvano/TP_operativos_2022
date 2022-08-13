/*
 * serializacion.c
 *
 *  Created on: 29 may. 2022
 *      Author: utnso
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "../include/serialization.h"



t_package* package_create() {
	t_package* self = malloc(sizeof(t_package));
	self->header = EMPTY_PACKAGE;
	self->payload = NULL;

	return self;
}

void package_destroy(t_package* self) {
	free(self);
}

t_buffer*  buffer_create() {
	t_buffer* self = malloc(sizeof(t_buffer));
	self->size = 0;
	self->stream = NULL;

	return self;
}

void buffer_destroy(t_buffer* self) {
	free(self->stream);
	free(self);
}



/*
 * Tipo de datos		Tamaño			Descripcion
 * uint32_t				4 bytes			Identificador de paquete
 * uint32_t				4 bytes			Tamaño del buffer de datos (mensaje)
 * void*				variable		stream de datos del mensaje
 */
void* serializar_paquete(t_package* package, uint32_t *size) {

	// header del paquete + tamaño del buffer + buffer
	*size = (sizeof(uint32_t) * 2) + package->payload->size;
	void* stream = malloc(*size);
	int offset = 0;

	// header
	memcpy(stream + offset, &(package->header), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	// buffer size
	memcpy(stream + offset, &(package->payload->size), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	if (package->payload->size > 0) {
		// payload
		memcpy(stream + offset, package->payload->stream, package->payload->size);
		offset += package->payload->size;
	}

	if (offset != *size)
		printf("serializar_paquete: El tamaño del buffer[%d] no coincide con el offset final[%d]\n", *size, offset);

	//package_destroy(package);
	return stream;
}

// esta funcion no creo que se use, porque cuando el mensaje llega al socket, se va leyendo de a poco
t_package* deserializar_paquete(void* stream) {
	t_package* package = package_create();
	int offset = 0;

	// header
	memcpy(&(package->header), stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	// buffer size
	package->payload = buffer_create();
	memcpy(&(package->payload->size), stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	// stream de datos (mensaje)
	package->payload->stream = malloc(package->payload->size);
	memcpy(package->payload->stream, stream + offset, package->payload->size);

	return package;
}


t_buffer* serialize_uint32t(uint32_t code) {
	t_buffer* buffer = buffer_create();

	buffer->size = sizeof(uint32_t);
	buffer->stream = malloc(buffer->size);
	memcpy(buffer->stream, &code, sizeof(uint32_t));
	return buffer;
}

uint32_t deserialize_uint32t(t_buffer* buffer) {
	uint32_t code;

	memcpy(&code, buffer->stream, sizeof(uint32_t));
	return code;
}


