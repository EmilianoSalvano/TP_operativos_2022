/*
 * kiss.error.c
 *
 *  Created on: 13 jul. 2022
 *      Author: utnso
 */

#include <string.h>
#include "../include/kiss.error.h"


#define UNKNOWN_ERROR_DESC "Error desconocido"


// serializacion
#define SRL_ERR_OFFSET_DESC "El tamaño del buffer no coincide con el offset final"

// sockets
#define SKT_CONNECTION_LOST_DESC "Conexion perdida"
#define SKT_DATA_TRANF_INCOMPLETE_DESC "Transferencia de datos incompleta"


char* errortostr(int error) {
	switch (error) {
	case UNKNOWN_ERROR:
		return UNKNOWN_ERROR_DESC;
	case SRL_ERR_OFFSET:
		return SRL_ERR_OFFSET_DESC;
	case SKT_CONNECTION_LOST:
		return SKT_CONNECTION_LOST_DESC;
	case SKT_DATA_TRANF_INCOMPLETE:
		return SKT_DATA_TRANF_INCOMPLETE_DESC;
	default:
		return strerror(error);
	}
}

