#include "include/main.h"
#include "include/memoriaswap.h"
#include "../shared/include/utils.h"

int main(int argc, char **argv) {


	int log_level = atoi(argv[1]);
	memoria_iniciar(log_level, argv[2]);

	//pthread_exit(NULL);
}
