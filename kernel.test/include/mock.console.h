/*
 * mock.console.h
 *
 *  Created on: 25 jul. 2022
 *      Author: utnso
 */

#ifndef MOCK_CONSOLE_H_
#define MOCK_CONSOLE_H_

typedef enum {
	RANDOM,
	BASE,
	PLANIFICADOR,
	SUSPENDIDO,
	INTEGRAL
} t_console_producer;


int mock_console_run();
void mock_console_stop();


#endif /* MOCK_CONSOLE_H_ */
