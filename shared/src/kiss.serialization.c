/*
 * kiss.serialization.c
 *
 *  Created on: 13 jul. 2022
 *      Author: utnso
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "../include/kiss.serialization.h"




t_buffer* serialize_signal(t_signal signal) {
	return serialize_uint32t((uint32_t)signal);
}


t_signal deserialize_signal(t_buffer* buffer) {
	return (t_signal)(deserialize_uint32t(buffer));
}


t_buffer* serialize_process_code(t_process_code code) {
	return serialize_uint32t(code);
}


t_process_code deserialize_process_code(t_buffer* buffer) {
	return (t_process_code)(deserialize_uint32t(buffer));
}


/*
 *	Tipo de datos		Tamaño			Descripcion
 *	uint32_t 			4 bytes			Cantidad de instrucciones
 *	uint32_t			4 bytes			Codigo de instruccion
 *	uint32_t			4 bytes			Cantidad de parametros
 *	uint32_t			4 bytes			Longitud del parametro - incluye al centinela de fin de cadena "\0"
 *	char				variable		Valor del parametro - 1 byte por caracter + centinela
 */
t_buffer* serializar_lista_instrucciones(t_list* list_instrucciones) {
	t_buffer*			buffer;			// buffer de retorno

	int 				i_count;		// cantidad total de instrucciones
	int					p_count;		// cantidad de parametros de una instruccion
	int 				stream_size;	// tamaño total del buffer
	int 				offset = 0;		// indicador de posicion del buffer
	t_list_iterator* 	i_iterator;		// iterador de instrucciones
	t_list_iterator*	p_iterator;		// iterador de parametros de una instruccion
	t_instruction*		instruccion;	// instruccion
	char*				parametro;		// parametro
	uint32_t			size_parametro;	// longitud del parametro + centinela "\0"


	i_iterator = list_iterator_create(list_instrucciones);
	stream_size = 0;
	i_count = 0;

	while (list_iterator_has_next(i_iterator)) {
		instruccion = (t_instruction*)list_iterator_next(i_iterator);

		// codigo, cantidad de parametros
		stream_size += (sizeof(uint32_t) * 2);
		p_count = list_size(instruccion->parametros);

		if (p_count > 0) {
			p_iterator = list_iterator_create(instruccion->parametros);

			while (list_iterator_has_next(p_iterator)) {
				// longitud del parametro, valor del parametro
				stream_size += sizeof(uint32_t) + strlen((char*)list_iterator_next(p_iterator)) + 1;
			}

			list_iterator_destroy(p_iterator);
		}

		i_count++;
	}

	list_iterator_destroy(i_iterator);


	buffer = buffer_create();
	// sumo 4 bytes para agregar la cantidad de instrucciones enviadas
	buffer->size = stream_size + sizeof(uint32_t);
	buffer->stream = malloc(buffer->size);

	// cantidad de insrucciones
	memcpy(buffer->stream + offset, &(i_count), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	i_iterator = list_iterator_create(list_instrucciones);

	while (list_iterator_has_next(i_iterator)) {
		instruccion = (t_instruction*)list_iterator_next(i_iterator);

		// codigo de instruccion
		memcpy(buffer->stream + offset, &(instruccion->codigo), sizeof(uint32_t));
		offset += sizeof(uint32_t);

		// cantidad de parametros
		p_count = list_size(instruccion->parametros);
		memcpy(buffer->stream + offset, &(p_count), sizeof(uint32_t));
		offset += sizeof(uint32_t);

		if (p_count > 0) {
			p_iterator = list_iterator_create(instruccion->parametros);

			while (list_iterator_has_next(p_iterator)) {
				parametro = (char*)list_iterator_next(p_iterator);
				size_parametro = strlen(parametro) + 1;

				// longitud del parametro
				memcpy(buffer->stream + offset, &(size_parametro), sizeof(uint32_t));
				offset += sizeof(uint32_t);

				// valor del parametro
				memcpy(buffer->stream + offset, parametro, size_parametro);
				offset += size_parametro;
			}

			list_iterator_destroy(p_iterator);
		}
	}

	list_iterator_destroy(i_iterator);

	if (offset != buffer->size)
		printf("serializar_lista_instrucciones: El tamaño del buffer[%d] no coincide con el offset final[%d]\n", buffer->size, offset);

	return buffer;
}




t_list* deserializar_lista_instrucciones(t_buffer* buffer) {
	t_list*			i_list;			// retorno de lista de instrucciones
	t_instruction* 	instruccion;	// instruccion
	uint32_t		i_count;		// cantidad total de instrucciones recibidas
	uint32_t		p_count;		// cantidad de parametros de una instruccion
	int 			offset;			// indicador de posicion de stream
	int				p_size;			// longitud del parametro
	char*			param;			// valor del parametro
	t_codigo_instruccion	codigo;


	i_list = list_create();
	offset = 0;

	// cantidad de instrucciones
	memcpy(&i_count, buffer->stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	for(int i=0; i < i_count; i++) {

		// codigo de la instruccion
		memcpy(&(codigo), buffer->stream + offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		instruccion = instruccion_create(codigo, false);

		// cantidad de parametros
		memcpy(&(p_count), buffer->stream + offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		if (p_count > 0) {
			for (int p = 0; p < p_count; p++) {
				// longitud del parametro
				memcpy(&p_size, buffer->stream + offset, sizeof(uint32_t));
				offset += sizeof(uint32_t);

				// valor del parametro
				param = malloc(p_size);
				memcpy(param, buffer->stream + offset, p_size);

				list_add(instruccion->parametros, param);
				offset += p_size;
			}
		}

		list_add(i_list, instruccion);
	}

	return i_list;
}


/*
 *	Tipo de datos		Tamaño			Descripcion
 *	uint32_t 			4 bytes			id
 *	uint32_t			4 bytes			process_size
 *	uint32_t			4 bytes			program_counter
 *	uint32_t			4 bytes			tabla_paginas
 *	uint32_t			4 bytes			estimacion_rafaga
 *	uint32_t			4 bytes			tamaño del stream de instrucciones
 *	void*				variable		stream de instrucciones (Ver serializar_instrucciones)
 */
t_buffer* serializar_pcb(t_pcb* pcb) {
	t_buffer*		buffer;
	t_buffer*		instrucciones;
	int				offset = 0;
	//uint32_t		estimacion_rafaga;

	instrucciones = serializar_lista_instrucciones(pcb->instructions);

	buffer = buffer_create();
	buffer->size = 8 * sizeof(uint32_t) + instrucciones->size;
	buffer->stream = malloc(buffer->size);

	// id
	memcpy(buffer->stream + offset, &(pcb->id) + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	/*
	// process_size
	memcpy(buffer->stream + offset, &(pcb->process_size), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	*/

	// program_counter
	memcpy(buffer->stream + offset, &(pcb->program_counter), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	// tabla_paginas
	memcpy(buffer->stream + offset, &(pcb->tabla_paginas), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	// cpu_dispatch_cond
	memcpy(buffer->stream + offset, &(pcb->cpu_dispatch_cond), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	// cpu_executed_burst
	memcpy(buffer->stream + offset, &(pcb->cpu_executed_burst), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	// cpu_estimated_burst
	memcpy(buffer->stream + offset, &(pcb->cpu_estimated_burst), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	// io_burst
	memcpy(buffer->stream + offset, &(pcb->io_burst), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	/*
	// estimacion_rafaga
	estimacion_rafaga = (uint32_t)(pcb->cpu_estimated_burst * 100);
	memcpy(buffer->stream + offset, &estimacion_rafaga, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	*/

	// tamaño del stream de instrucciones
	memcpy(buffer->stream + offset, &(instrucciones->size), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	// instrucciones
	if (instrucciones->size > 0) {
		memcpy(buffer->stream + offset, instrucciones->stream, instrucciones->size);
		offset += instrucciones->size;
	}

	buffer_destroy(instrucciones);

	if (buffer->size != offset)
		printf("serializar_pcb: El tamaño del buffer[%d] no coincide con el offset final[%d]\n", buffer->size, offset);

	return buffer;
}




t_pcb* deserializar_pcb(t_buffer* buffer) {
	t_pcb*		pcb;
	t_buffer*	i_buffer;
	int			offset = 0;
	//uint32_t	estimacion_rafaga;


	pcb = pcb_create(NULL);

	// id
	memcpy(&(pcb->id), buffer->stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	/*
	// process_size
	memcpy(&(pcb->process_size), buffer->stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	*/

	// program_counter
	memcpy(&(pcb->program_counter), buffer->stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	// tabla_paginas
	memcpy(&(pcb->tabla_paginas), buffer->stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	// cpu_dispatch_cond
	memcpy(&(pcb->cpu_dispatch_cond), buffer->stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	// cpu_executed_burst
	memcpy(&(pcb->cpu_executed_burst), buffer->stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	// cpu_estimated_burst
	memcpy(&(pcb->cpu_estimated_burst), buffer->stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	// io_burst
	memcpy(&(pcb->io_burst), buffer->stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	/*
	// estimacion_rafaga
	memcpy(&estimacion_rafaga, buffer->stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	pcb->cpu_estimated_burst = ((double)estimacion_rafaga) / 100.00;
	*/

	i_buffer = buffer_create();

	// tamaño de stream de instrucciones
	memcpy(&(i_buffer->size), buffer->stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	// instrucciones
	i_buffer->stream = malloc(i_buffer->size);
	memcpy(i_buffer->stream, buffer->stream + offset, i_buffer->size);
	offset += i_buffer->size;

	pcb->instructions = deserializar_lista_instrucciones(i_buffer);
	buffer_destroy(i_buffer);

	return pcb;
}


/*
 *	Tipo de datos		Tamaño			Descripcion
 *	uint32_t			4 bytes			process_size
 *	uint32_t			4 bytes			tamaño del stream de instrucciones
 *	void*				variable		stream de instrucciones (Ver serializar_instrucciones)
 */
t_buffer* serialize_program(t_program* program) {
	t_buffer*		buffer;
	t_buffer*		instrucciones;
	int				offset = 0;

	instrucciones = serializar_lista_instrucciones(program->instructions);

	buffer = buffer_create();
	buffer->size = (2 * sizeof(uint32_t)) + instrucciones->size;
	buffer->stream = malloc(buffer->size);

	// process_size
	memcpy(buffer->stream + offset, &(program->size), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	// tamaño del stream de instrucciones
	memcpy(buffer->stream + offset, &(instrucciones->size), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	// instrucciones
	if (instrucciones->size > 0) {
		memcpy(buffer->stream + offset, instrucciones->stream, instrucciones->size);
		offset += instrucciones->size;
	}

	buffer_destroy(instrucciones);

	if (buffer->size != offset)
		printf("serializar_pcb: El tamaño del buffer[%d] no coincide con el offset final[%d]\n", buffer->size, offset);

	return buffer;
}

t_program* deserialize_program(t_buffer* buffer) {
	t_program*			program;
	t_buffer*			i_buffer;
	int					offset = 0;

	program = program_create(NULL);

	// process_size
	memcpy(&(program->size), buffer->stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	i_buffer = buffer_create();

	// tamaño de stream de instrucciones
	memcpy(&(i_buffer->size), buffer->stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	// instrucciones
	i_buffer->stream = malloc(i_buffer->size);
	memcpy(i_buffer->stream, buffer->stream + offset, i_buffer->size);
	offset += i_buffer->size;

	program->instructions = deserializar_lista_instrucciones(i_buffer);
	buffer_destroy(i_buffer);

	return program;
}


t_buffer* serializar_setup_memoria(t_setup_memoria* setup) {
	t_buffer* buffer = buffer_create();
	int offset = 0;

	buffer->size = 2 * sizeof(uint32_t);
	buffer->stream = malloc(buffer->size);

	memcpy(buffer->stream, &(setup->entradas_por_tabla), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(buffer->stream + offset, &(setup->tam_pagina), sizeof(uint32_t));

	return buffer;
}


t_setup_memoria* deserializar_setup_memoria(t_buffer* buffer) {
	t_setup_memoria* setup = setup_memoria_create();
	int offset = 0;

	memcpy(&(setup->entradas_por_tabla), buffer->stream, sizeof(uint32_t));
	offset = sizeof(uint32_t);

	memcpy(&(setup->tam_pagina), buffer->stream + offset, sizeof(uint32_t));

	return setup;
}


t_buffer* serializar_operacion_eu(t_operacion_EU* operacion) {
	t_buffer* buffer = buffer_create();
	int offset = 0;

	buffer->size = 4 * sizeof(uint32_t);
	buffer->stream = malloc(buffer->size);

	memcpy(buffer->stream, &(operacion->pid), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(buffer->stream + offset, &(operacion->dato), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(buffer->stream + offset, &(operacion->desplazamiento), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(buffer->stream + offset, &(operacion->numero_marco), sizeof(uint32_t));

	return buffer;
}

t_operacion_EU* deserializar_operacion_EU(t_buffer* buffer) {
	t_operacion_EU* operacion = operacion_eu_create();
	int offset = 0;

	memcpy(&(operacion->pid), buffer->stream, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(&(operacion->dato), buffer->stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(&(operacion->desplazamiento), buffer->stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(&(operacion->numero_marco), buffer->stream + offset, sizeof(uint32_t));

	return operacion;
}

t_buffer* serializar_direccion(t_direccion* direccion) {
	t_buffer* buffer = buffer_create();
	int offset = 0;

	buffer->size = 4 * sizeof(uint32_t);
	buffer->stream = malloc(buffer->size);

	memcpy(buffer->stream, &(direccion->pid), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(buffer->stream + offset, &(direccion->pagina), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(buffer->stream + offset, &(direccion->tabla_primer_nivel), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(buffer->stream + offset, &(direccion->tabla_segundo_nivel), sizeof(uint32_t));

	return buffer;
}

t_direccion* deserializar_direccion(t_buffer* buffer) {
	t_direccion* direccion = direccion_create();
	int offset = 0;

	memcpy(&(direccion->pid), buffer->stream, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(&(direccion->pagina), buffer->stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(&(direccion->tabla_primer_nivel), buffer->stream + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(&(direccion->tabla_segundo_nivel), buffer->stream + offset, sizeof(uint32_t));

	return direccion;
}


t_buffer* serializar_page_entry_request(t_page_entry_request* request) {
	t_buffer* buffer = buffer_create();
	int offset = 0;

	buffer->size = 2 * sizeof(uint32_t);
	buffer->stream = malloc(buffer->size);

	memcpy(buffer->stream, &(request->pcb_id), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(buffer->stream + offset, &(request->process_size), sizeof(uint32_t));

	return buffer;
}


t_page_entry_request* deserializar_page_entry_request(t_buffer* buffer) {
	t_page_entry_request* request = page_entry_request_create();
	int offset = 0;

	memcpy(&(request->pcb_id), buffer->stream, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(&(request->process_size), buffer->stream + offset, sizeof(uint32_t));

	return request;
}



/*
t_buffer* serializar_respuesta_handsheke_cpu(uint32_t entradas_por_tabla, uint32_t tam_pagina){
	t_buffer* buffer = buffer_create();
	buffer->size = 2*sizeof(uint32_t);
	buffer->stream = malloc(buffer->size);

	memcpy(buffer->stream, &entradas_por_tabla, sizeof(uint32_t));

	memcpy(buffer->stream + sizeof(uint32_t), &tam_pagina, sizeof(uint32_t));

	return buffer;
}


// usar & en la declaracion de la funcion es de C++
// en C se utiliza el * que indica que lo que pasas es el puntero, luego cuando llamas a la funcion usas el & - ej: func(&valor);
//void deserializar_respuesta_handsheke_cpu(t_buffer* buffer, uint32_t &entradas_por_tabla, uint32_t &tam_pagina){
void deserializar_respuesta_handsheke_cpu(t_buffer* buffer, uint32_t* entradas_por_tabla, uint32_t* tam_pagina){
	memcpy(&entradas_por_tabla, buffer->stream, sizeof(uint32_t));

	memcpy(&tam_pagina, buffer->stream + sizeof(uint32_t), sizeof(uint32_t));

	buffer_destroy(buffer);
}

//Ahora que lo termine me doy cuenta que podria usar una estructura como esta para la respuesta del handshake, y me ahorro tener dos funciones que
//solo sirven para mandar 2 uint32_t
t_buffer* serializar_operacion_EU(t_operacion_EU operacion){
	t_buffer* buffer = buffer_create();
	buffer->size = sizeof(t_operacion_EU);
	buffer->stream = malloc(buffer->size);

	memcpy(buffer->stream, &operacion, sizeof(t_operacion_EU));

	return buffer;
}

void deserializar_operacion_EU(t_buffer* buffer, t_operacion_EU* operacion){
	memcpy(&operacion, buffer->stream, sizeof(t_operacion_EU));

	buffer_destroy(buffer);
}


t_buffer* serializar_direccion(t_direccion direccion){
	t_buffer* buffer = buffer_create();
	buffer->size = sizeof(t_direccion);
	buffer->stream = malloc(buffer->size);

	memcpy(buffer->stream, &direccion, sizeof(t_direccion));

	return buffer;
}

void deserializar_direccion(t_buffer* buffer, t_direccion* direccion){
	memcpy(&direccion, buffer->stream, sizeof(t_direccion));

	buffer_destroy(buffer);
}
*/
