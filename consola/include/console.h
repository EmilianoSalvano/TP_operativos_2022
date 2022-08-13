/*
 * console.h
 *
 *  Created on: 30 jul. 2022
 *      Author: utnso
 */

#ifndef CONSOLE_H_
#define CONSOLE_H_

typedef struct{
	char* ip_kernel;
	char* puerto_kernel;
} t_config_consola;



void console_run(char* config_file, char* program_file, int program_size);


#endif /* INCLUDE_CONSOLE_H_ */
