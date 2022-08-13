/*
 * kiss.serialization.h
 *
 *  Created on: 13 jul. 2022
 *      Author: utnso
 */

#ifndef INCLUDE_KISS_SERIALIZATION_H_
#define INCLUDE_KISS_SERIALIZATION_H_

#include <commons/collections/list.h>

#include "serialization.h"
#include "kiss.structs.h"


typedef enum {
	// señales para envio de paquetes (600-699)
	SG_CODE_NOT_EXPECTED = 100,
	SG_ERROR_PACKAGE,

	// señales comunes para operaciones (600-699)
	SG_OPERATION_SUCCESS,
	SG_OPERATION_FAILURE,

	// señales para comunicacion entre modulos
	// cpu (700-719)
	SG_CPU_INTERRUPT = 700,

	// memoria (720-739)

	// consola (740-759)

	// señales para control de conexiones (800-899)
	SG_CONNECTION_ACCEPTED = 800,
	SG_CONNECTION_REJECTED,
	SG_CLOSING_CONNECTION,
	//SG_CPU_CONNECTION,
	//SG_KERNEL_CONNECTION
} t_signal;



// Headers de mensajes
typedef enum {
	// los estados 0 y 1 no debe utilizarse como encabezado para envio de mensajes.
	// son estados para serializacion de paquetes
	MC_EMPTY_PACKAGE = 0,
	MC_ERROR_PACKAGE = 1,
	// codigos para envio de estructuras (900-999)
	MC_UINT32T_CODE = 900,
	MC_SIGNAL,
	MC_PCB,
	MC_PROCESS_CODE,
	MC_PROGRAM,

	// Operaciones CPU-MEMORIA
	MC_MEMORY_SETUP,
	MC_ESCRIBIR_MARCO,
	MC_LEER_MARCO,
	MC_BUSCAR_TABLA_DE_PAGINAS,
	MC_BUSCAR_MARCO,

	//OPERACIONES KERNEL-MEMORIA
	MC_NUEVO_PROCESO,
	MC_SUSPENDER_PROCESO,
	MC_FINALIZAR_PROCESO,
} t_message_code;


/*
 * @NAME: serializar_codigo
 * @DESC: serializa un codigo de tamaño uint32_t
*/
t_buffer* serialize_process_code(t_process_code code);

/*
 * @NAME: deserializar_codigo
 * @DESC: deserializa un codigo de tamaño uint32_t
*/
t_process_code deserialize_process_code(t_buffer* buffer);

/*
 * @NAME: serialize_signal
 * @DESC: serializa un señal uint32_t
*/
t_buffer* serialize_signal(t_signal signal);

/*
 * @NAME: deserialize_signal
 * @DESC: deserializa una señal uint32_t
*/
t_signal deserialize_signal(t_buffer* buffer);

/*
 *	@NAME: serializar_lista_instrucciones
 *	@DESC: Serializa una lista "t_list" de instrucciones de tipo "t_instruccion"
 *
 *	Tipo de datos		Tamaño			Descripcion
 *	uint32_t 			4 bytes			Cantidad de instrucciones
 *	uint32_t			4 bytes			Codigo de instruccion
 *	uint32_t			4 bytes			Cantidad de parametros
 *	uint32_t			4 bytes			Longitud del parametro - incluye al centinela de fin de cadena "\0"
 *	char				variable		Valor del parametro - 1 byte por caracter + centinela
 */
t_buffer* serializar_lista_instrucciones(t_list* instrucciones);

/*
 *	@NAME: deserializar_lista_instrucciones
 *	@DESC: deerializa un buffer de una lista de instrucciones
*/
t_list* deserializar_lista_instrucciones(t_buffer* buffer);

/*
 *	@NAME: serializar_pcb
 *	@DESC: serializa en un stream de bytes a la estructura t_pcb y sus instrucciones
 *
 *	Tipo de datos		Tamaño			Descripcion
 *	uint32_t 			4 bytes			id
 *	uint32_t			4 bytes			process_size
 *	uint32_t			4 bytes			program_counter
 *	uint32_t			4 bytes			tabla_paginas
 *	uint32_t			4 bytes			estimacion_rafaga
 *	uint32_t			4 bytes			tamaño del stream de instrucciones
 *	void*				variable		stream de instrucciones (Ver serializar_instrucciones)
 */
t_buffer* serializar_pcb(t_pcb* pcb);

/*
 *	@NAME: deserializar_pcb
 *	@DESC: deserializa un stream de bytes en una estructura t_pcb
*/
t_pcb* deserializar_pcb(t_buffer* buffer);



t_buffer* serialize_program(t_program* program);
t_program* deserialize_program(t_buffer* buffer);


t_buffer* serializar_setup_memoria(t_setup_memoria* setup);
t_setup_memoria* deserializar_setup_memoria(t_buffer* buffer);

t_buffer* serializar_operacion_eu(t_operacion_EU* operacion);
t_operacion_EU* deserializar_operacion_EU(t_buffer* buffer);

t_buffer* serializar_direccion(t_direccion* direccion);
t_direccion* deserializar_direccion(t_buffer* buffer);

t_buffer* serializar_page_entry_request(t_page_entry_request* request);
t_page_entry_request* deserializar_page_entry_request(t_buffer* buffer);



/*
t_buffer* serializar_respuesta_handsheke_cpu(uint32_t entradas_por_tabla, uint32_t tam_pagina);

void deserializar_respuesta_handsheke_cpu(t_buffer* buffer, uint32_t* entradas_por_tabla, uint32_t* tam_pagina);

t_buffer* serializar_operacion_EU(t_operacion_EU operacion);

void deserializar_operacion_EU(t_buffer* buffer, t_operacion_EU* operacion);


t_buffer* serializar_direccion(t_direccion direccion);

void deserializar_direccion(t_buffer* buffer, t_direccion* direccion);
*/

#endif /* INCLUDE_KISS_SERIALIZATION_H_ */
