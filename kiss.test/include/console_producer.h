/*
 * console_producer.h
 *
 *  Created on: 3 ago. 2022
 *      Author: utnso
 */

#ifndef CONSOLE_PRODUCER_H_
#define CONSOLE_PRODUCER_H_




typedef enum {
	RANDOM,
	BASE,
	PLANIFICADOR,
	SUSPENDIDO,
	INTEGRAL
} t_console_test;


typedef struct {
	char*	path;
	int 	program_size;
} t_console_parameter;

int mock_console_run();


#endif /* CONSOLE_PRODUCER_H_ */
