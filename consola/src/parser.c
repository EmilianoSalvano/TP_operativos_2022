#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/string.h>

#include "../../shared/include/kiss.error.h"
#include "../include/parser.h"


static int parse_instructions(char* line, t_list* list);



t_program* parse_program(char* file, int program_size) {
	FILE *fp = fopen(file, "r");

	if (fp == NULL) {
		printf("No se puede abrir el archvivo. error:[%d] msg:[%s]\n", errno, errortostr(errno));
		return NULL;
	}

	t_program* program = program_create(list_create());
	program->size = program_size;

	char* line = NULL;
	size_t len = 0;

	bool error = false;
	while(getline(&line, &len, fp) != -1) {
		if (len > 0) {
			error = (parse_instructions(line, program->instructions) != 0);
			if (error) break;
		}
	}

	fclose(fp);
	free(line);

	if (error) {
		printf("error de parseo de archivo de pseudocodigo\n");
		program_destroy(program);
		return NULL;
	}
	else {
		return program;
	}
}


static int parse_instructions(char* line, t_list* list) {
	int result = EXIT_SUCCESS;
	t_instruction* inst;

	char** parsed = string_split(line, " ");

	if (strcmp(parsed[0], "NO_OP") == 0) {
		int c = atoi(parsed[1]);

		for (int i = 0; i < c; i++)
			list_add(list, instruccion_create(ci_NO_OP, false));
	}
	else if (strcmp(parsed[0], "I/O") == 0) {
		inst = instruccion_create(ci_I_O, false);
		list_add(inst->parametros, strdup(parsed[1]));
		list_add(list, inst);
	}
	else if (strcmp(parsed[0], "WRITE") == 0) {
		inst = instruccion_create(ci_WRITE, false);
		list_add(inst->parametros, strdup(parsed[1]));
		list_add(inst->parametros, strdup(parsed[2]));
		list_add(list, inst);
	}
	else if (strcmp(parsed[0], "COPY") == 0) {
		inst = instruccion_create(ci_COPY, false);
		list_add(inst->parametros, strdup(parsed[1]));
		list_add(inst->parametros, strdup(parsed[2]));
		list_add(list, inst);
	}
	else if (strcmp(parsed[0], "READ") == 0) {
		inst = instruccion_create(ci_READ, false);
		list_add(inst->parametros, strdup(parsed[1]));
		list_add(list, inst);
	}
	else if (strcmp(parsed[0], "EXIT") == 0) {
		inst = instruccion_create(ci_EXIT, false);
		list_add(list, inst);
	}
	else {
		printf("tipo de instruccion desconocida:[%s]\n", parsed[0]);
		result = EXIT_FAILURE;
	}

	char* v;
	int i = 0;
	while((v = parsed[i]) != NULL) {
		free(v);
		i++;
	}

	free(parsed);

	return result;
}





