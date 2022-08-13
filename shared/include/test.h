/*
 * test.h
 *
 *  Created on: 4 jun. 2022
 *      Author: utnso
 */

#ifndef INCLUDE_TEST_H_
#define INCLUDE_TEST_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <inttypes.h>
#include <commons/temporal.h>
#include <commons/string.h>
#include <pthread.h>
#include <unistd.h>


#include "time_utils.h"


typedef enum {
	COMPARE_UNDEFINED = -1,
	COMPARE_EQUAL = 0,
	COMPARE_NOT_EQUAL = 1
} t_compare_result;


typedef struct {
	char*				name;
	uint64_t			start;
	uint64_t			end;
	t_compare_result	result;
	bool				async;
	pthread_mutex_t		mutex_print;
} t_test;


t_compare_result _test_compare_result(bool v);
t_compare_result test_compare_int(int val1, int val2);
t_compare_result test_compare_string(char* val1, char* val2);
t_compare_result test_compare_double(double val1, double val2);


/*
 *	@NAME: test_open
 *	@DESC: Inicia el estado de test. Retorna un objeto t_test con la fecha de inicio y nombre del test. Imprime el encabezado de la prueba
 */
t_test* test_open(char* test_name);
t_test* test_open_async(char* test_name);


/*
 *	@NAME: test_close
 *	@DESC: Finaliza el estado de test. Destruye el objeto t_test e imprime el pie del test
 */
void test_close(t_test* self);

/*
 *	@NAME: test_destroy
 *	@DESC: Destruye el objeto t_test
 */
void test_destroy(t_test* self);

/*
 *	@NAME: test_print_text
 *	@DESC: Imprime por pantalla el texto con la fecha actual
 */
void test_print(t_test* test, char* text);

void test_fprint(t_test* test, const char* format, ...);

/*
 *	@NAME: test_print_separator
 *	@DESC: Imprime por pantalla el texto junto a un separador de linea
 */
void test_print_separator(t_test* test, char* text);


#endif /* INCLUDE_TEST_H_ */
