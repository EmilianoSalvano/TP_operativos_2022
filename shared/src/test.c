/*
 * test.c
 *
 *  Created on: 4 jun. 2022
 *      Author: utnso
 */


#include "../include/test.h"



t_compare_result _test_compare_result(bool v) {
	if (v)
		return COMPARE_EQUAL;
	else
		return COMPARE_NOT_EQUAL;
}

t_compare_result test_compare_int(int val1, int val2) {
	return _test_compare_result(val1 == val2);
}

t_compare_result test_compare_string(char* val1, char* val2) {
	return _test_compare_result(string_equals_ignore_case(val1, val2));
}

t_compare_result test_compare_double(double val1, double val2) {
	return _test_compare_result(val1 == val2);
}



t_test* test_open(char* test_name) {
	//time_t seconds;
	//time(&seconds);

	t_test* test = malloc(sizeof(t_test));
	test->name = strdup(test_name);
	test->start = get_time_stamp();
	test->result = COMPARE_UNDEFINED;
	test->async = false;

	char* fecha = temporal_get_string_time("%H:%M:%S:%MS");
	char* separador = string_repeat('-', 80);
	pid_t pid = getpid();


	printf("%s\n", separador);
	printf("@TEST: %s\n", test->name);
	printf("@START: %s\n", fecha);
	printf("@PROCESS ID: %d\n", pid);
	printf("%s\n", separador);

	free(fecha);
	free(separador);
	return test;
}

t_test* test_open_async(char* test_name) {
	t_test* test = test_open(test_name);
	test->async = true;
	pthread_mutex_init(&(test->mutex_print), NULL);
	return test;
}


void test_close(t_test* self) {
	char* separador = string_repeat('-', 80);
	char* fecha = temporal_get_string_time("%H:%M:%S:%MS");

	self->end = get_time_stamp();
	uint64_t time_elapsed = self->end - self->start;

	printf("\n%s\n", separador);
	printf("@END: %s\n", fecha);
	printf("@ELAPSED: %"PRIu64" milliseconds\n", time_elapsed);

	switch(self->result) {
		case COMPARE_NOT_EQUAL:
			printf("@RESULT: Not Equal\n");
			break;
		case COMPARE_EQUAL:
			printf("@RESULT: Equal\n");
			break;
		default:
			printf("@RESULT: Undefined\n");
			break;
	}

	printf("%s\n", separador);

	free(fecha);
	free(separador);
	test_destroy(self);
}


void test_destroy(t_test* self) {
	if (self->async)
		pthread_mutex_destroy(&(self->mutex_print));

	free(self->name);
	free(self);
}


void test_fprint(t_test* test, const char* format, ...) {
	va_list arguments;
	va_start(arguments, format);
	char* str = string_from_vformat(format, arguments);
	va_end(arguments);

	test_print(test, str);
	free(str);
}

void test_print(t_test* test, char* text) {
	char* fecha = temporal_get_string_time("%H:%M:%S:%MS");

	if (test->async) {
		pthread_mutex_lock(&(test->mutex_print));
		printf("%s :: %lu -> %s", fecha, pthread_self(), text);
		pthread_mutex_unlock(&(test->mutex_print));
	}
	else {
		printf("%s -> %s", fecha, text);
	}

	/*
	if (test->async)
		pthread_mutex_unlock(&(test->mutex_print));
	*/
	free(fecha);
}

void test_print_separator(t_test* test, char* text) {
	char* separador = string_repeat('.', 80);


	if (test->async) {
		pthread_mutex_lock(&(test->mutex_print));

		printf("\n");
		printf("%s\n", text);
		printf("%s\n", separador);

		pthread_mutex_unlock(&(test->mutex_print));
	}
	else {
		printf("\n");
		printf("%s\n", text);
		printf("%s\n", separador);
	}

	free(separador);
}


