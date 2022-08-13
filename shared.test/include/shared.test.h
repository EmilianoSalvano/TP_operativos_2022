/*
 * shared.test.h
 *
 *  Created on: 31 may. 2022
 *      Author: utnso
 */

#ifndef SHARED_TEST_H_
#define SHARED_TEST_H_

#include <stdlib.h>
#include <commons/collections/list.h>
#include <commons/string.h>

#include "../shared/include/kiss.global.h"
#include "../shared/include/kiss.structs.h"
#include "../shared/include/kiss.serialization.h"
#include "../shared/include/time_utils.h"
#include "../shared/include/test.h"
#include "../shared/include/conexiones.h"



void test_serializar_code();

/*
 * @NAME: test_pcb_to_string
 * @DESC: imprime por pantalla un pcb
 */
void test_pcb_to_string();

/*
 * @NAME: test_instruccion_to_string
 * @DESC: imprime por pantalla una instruccion
 */
void test_instruccion_to_string();

/*
 * @NAME: test_serializar_instrucciones
 * @DESC: Test para probar los metodos de serializacion y deserealizacion de una lista de t_instruccion
 */
void test_serializar_instrucciones();

/*
 * @NAME: test_serializar_pcb
 * @DESC: Test para probar los metodos de serializacion y deseralizacion de t_pcb
 */
void test_serializar_pcb();

/*
 * @NAME: test_sockets
 * @DESC: Test para probar el paso de mensajes entre sockets. El metodo inicia un socket cliente y otro servidor, codifica un mensaje,
 * lo envia y lo decodifica
 */
void test_sockets();


void test_selializar_paquete();


void test_serializar_programa();

#endif /* SHARED_TEST_H_ */
