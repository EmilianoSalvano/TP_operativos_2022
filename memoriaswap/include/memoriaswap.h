/*
 * memoriaswap.h
 *
 *  Created on: 31 jul. 2022
 *      Author: utnso
 */

#ifndef MEMORIASWAP_H_
#define MEMORIASWAP_H_


#include <stdlib.h>
#include <string.h>
#include "configuracion.h"
#include "tablas.h"
#include "tests.h"
//#include "../../shared/include/resta.h"
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>
//#include "../include/memory.h"
#include "../../shared/include/socket_handler.h"
#include "../../shared/include/kiss.serialization.h"
#include "../../shared/include/kiss.structs.h"
#include "../../shared/include/test.h"



int memoria_iniciar(t_log_level log_level, char* cfg);


#endif /* INCLUDE_MEMORIASWAP_H_ */
