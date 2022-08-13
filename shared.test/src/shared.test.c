/*
 ============================================================================
 Name        : test.c
 Author      : Matias
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "../include/shared.test.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

//#define _GNU_SOURCE

void test_files() {
	FILE* fd = fopen("testfile.1", "wb");
	close(fd);
	truncate("testfile.1", 256);
}


void test_serializar_direccion() {
	t_test* test = test_open("test_serializar_direccion");

	t_direccion* direccion = direccion_create();
	direccion->pid = 8;
	direccion->tabla_primer_nivel = 15;
	direccion->tabla_segundo_nivel = 2;
	direccion->pagina = 89;

	t_buffer* buffer = serializar_direccion(direccion);
	direccion_destroy(direccion);

	direccion = deserializar_direccion(buffer);
	buffer_destroy(buffer);

	test_fprint(test, "pid: %d\n", direccion->pid);
	test_fprint(test, "tabla_primer_nivel: %d\n", direccion->tabla_primer_nivel);
	test_fprint(test, "tabla_segundo_nivel: %d\n", direccion->tabla_segundo_nivel);
	test_fprint(test, "pagina: %d\n", direccion->pagina);

	direccion_destroy(direccion);

	test_close(test);
}


void test_serializar_operacion() {
	t_test* test = test_open("test_serializar_operacion");

	t_operacion_EU* operacion = operacion_eu_create();
	operacion->pid = 8;
	operacion->numero_marco = 6;
	operacion->desplazamiento = 26;
	operacion->dato = 1526;

	t_buffer* buffer = serializar_operacion_eu(operacion);
	direccion_destroy(operacion);

	operacion = deserializar_operacion_EU(buffer);
	buffer_destroy(buffer);

	test_fprint(test, "pid: %d\n", operacion->pid);
	test_fprint(test, "numero_marco: %d\n", operacion->numero_marco);
	test_fprint(test, "desplazamiento: %d\n", operacion->desplazamiento);
	test_fprint(test, "dato: %d\n", operacion->dato);

	operacion_eu_destroy(operacion);

	test_close(test);
}



void test_serializar_code() {
	t_test* test = test_open("test_serializar_code");

	t_buffer* buffer = serialize_process_code(PROCESS_CONSOLE);

	test_fprint(test, "Buffer size: %d\n", buffer->size);

	uint32_t codigo;
	memcpy(&codigo, buffer->stream, sizeof(uint32_t));

	test_fprint(test, "Codigo: %d\n", codigo);

	buffer_destroy(buffer);

	test_close(test);
}


void test_instruccion_to_string() {
	t_test* test = test_open("test_instruccion_to_string");

	t_instruction* instruccion = produce_instruccion();

	char* string = instruccion_to_string(instruccion);
	printf("%s\n", string);

	free(string);
	instruccion_destroy(instruccion);

	test_close(test);
}

void test_serializar_instrucciones() {
	t_instruction* instruccion;
	t_list* i_list;
	t_list* result_i_list;
	t_buffer* buffer;

	t_test* test = test_open("test_serializar_instrucciones");

	// Set de datos
	i_list = list_create();

	instruccion = produce_instruccion();
	list_add(i_list, instruccion);

	instruccion = produce_instruccion();
	list_add(i_list, instruccion);

	// Execute
	buffer = serializar_lista_instrucciones(i_list);
	result_i_list = deserializar_lista_instrucciones(buffer);

	// Compare
	// lista original
	test_print_separator(test, "Lista de instrucciones inicial::");
	char* str_list = instruccion_list_to_string(i_list);
	test_fprint(test, "%s\n", str_list);

	// lista serializada
	test_print_separator(test, "Lista de instrucciones final::");
	char* str_result_list = instruccion_list_to_string(result_i_list);
	test_fprint(test, "%s\n", str_result_list);

	// resultado
	test->result = test_compare_string(str_list, str_result_list);

	free(str_list);
	free(str_result_list);
	buffer_destroy(buffer);

	list_destroy_and_destroy_elements(i_list, instruccion_destroy_in_list);
	list_destroy_and_destroy_elements(result_i_list, instruccion_destroy_in_list);

	test_close(test);
}



void test_pcb_to_string() {
	t_test* test = test_open("test_pcb_to_string");

	t_pcb* pcb = produce_pcb();

	char* pcb_string = pcb_to_string(pcb);
	printf("%s\n", pcb_string);

	free(pcb_string);
	pcb_destroy(pcb);

	test_close(test);
}


void test_serializar_pcb() {
	t_test* test = test_open("test_serializar_pcb");
	t_pcb* pcb = produce_pcb();

	// Execute
	t_buffer* buffer = serializar_pcb(pcb);
	t_pcb* result_pcb = deserializar_pcb(buffer);

	// Compare
	// pcb original
	test_print_separator(test, "pcb inicial::");
	char* pcb_original = pcb_to_string(pcb);
	test_fprint(test, "%s\n", pcb_original);

	test_print_separator(test, "Lista de instrucciones final::");
	char* pcb_final = pcb_to_string(result_pcb);
	test_fprint(test, "%s\n", pcb_final);

	test->result = test_compare_string(pcb_original, pcb_final);

	free(pcb_original);
	free(pcb_final);

	buffer_destroy(buffer);
	pcb_destroy(pcb);
	pcb_destroy(result_pcb);

	test_close(test);
}



void test_serializar_programa() {
	t_test* test = test_open("test_serializar_console_data");
	t_program* data = produce_program();

	// Execute
	t_buffer* buffer = serialize_program(data);
	t_program* result_data = deserialize_program(buffer);

	// Compare
	// data original
	test_print_separator(test, "data inicial::");
	char* data_original = program_to_string(data);
	test_fprint(test, "%s\n", data_original);

	test_print_separator(test, "data final::");
	char* data_final = program_to_string(result_data);
	test_fprint(test, "%s\n", data_final);

	test->result = test_compare_string(data_original, data_final);

	free(data_original);
	free(data_final);

	buffer_destroy(buffer);
	program_destroy(data);
	program_destroy(result_data);

	test_close(test);
}



void test_selializar_paquete() {
	t_test* test = test_open("test_selializar_paquete");
	t_pcb* pcb = produce_pcb();

	t_package* package = malloc(sizeof(t_package));
	package->header = MC_PCB;
	package->payload = serializar_pcb(pcb);

	test_print_separator(test, "Paquete inicial");

	test_fprint(test, "header: %d\n", package->header);
	test_fprint(test, "payload->size: %d\n", package->payload->size);
	char* s_pcb = pcb_to_string(pcb);
	test_fprint(test, "pcb-> %s\n", s_pcb);
	free(s_pcb);

	pcb_destroy(pcb);
	uint32_t size;
	void* stream = serializar_paquete(package, &size);
	buffer_destroy(package->payload);
	package_destroy(package);

	package = deserializar_paquete(stream);
	free(stream);

	pcb = deserializar_pcb(package->payload);

	test_print_separator(test, "Paquete final");
	test_fprint(test, "header: %d\n", package->header);
	test_fprint(test, "package size: %d\n", size);
	test_fprint(test, "payload->size: %d\n", package->payload->size);
	s_pcb = pcb_to_string(pcb);
	test_fprint(test, "pcb-> %s\n", s_pcb);
	free(s_pcb);

	pcb_destroy(pcb);
	buffer_destroy(package->payload);
	package_destroy(package);

	test_close(test);
}

