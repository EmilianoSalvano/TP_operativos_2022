/*
 * kiss.error.h
 *
 *  Created on: 13 jul. 2022
 *      Author: utnso
 */

#ifndef INCLUDE_KISS_ERROR_H_
#define INCLUDE_KISS_ERROR_H_

#include <errno.h>


#define UNKNOWN_ERROR		10000

// serializacion
#define SRL_ERR_OFFSET		10100

// sockets
#define SKT_CONNECTION_LOST			10200
#define SKT_DATA_TRANF_INCOMPLETE	10201


char* errortostr(int error);


#endif /* INCLUDE_KISS_ERROR_H_ */
