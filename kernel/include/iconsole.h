/*
 * console.h
 *
 *  Created on: 10 jul. 2022
 *      Author: utnso
 */

#ifndef ICONSOLE_H_
#define ICONSOLE_H_


#include "kernel_global.h"
#include "../../shared/include/kiss.structs.h"
#include "../../shared/include/kiss.serialization.h"
#include "../../shared/include/socket_handler.h"




t_console_module* console_module_create();
void console_module_destroy();

int console_module_run();
int console_module_stop();
void console_terminate(t_socket_handler* client, t_signal status);


#endif /* INCLUDE_ICONSOLE_H_ */
