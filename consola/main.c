#include <stdlib.h>
#include <stdio.h>

#include "include/main.h"

int main(int argc, char** argv) {
	
	//argv[1] tamaño del proceso
	//argv[2] archivo de pseudocodigo

	if (argc < 3) {
		printf ("se deben especificar la ruta del archivo de pseudocodigo y el tamaño del programa");
		return EXIT_FAILURE;
	}

	char* config_file = "console.config";
	char* program_file = argv[2];
	int program_size = atoi(argv[1]);

	if (program_size <= 0 ) {
		printf("el tamaño de programa debe ser un numero mayor a 0\n");
		return EXIT_FAILURE;
	}

	console_run(config_file, program_file, program_size);
	return EXIT_SUCCESS;
}
