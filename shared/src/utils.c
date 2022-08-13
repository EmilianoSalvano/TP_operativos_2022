/*
 * utils.c
 *
 *  Created on: 7 jun. 2022
 *      Author: utnso
 */

#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#include "../include/utils.h"



int produce_num(int max) {
	return produce_num_in_range(0, max);
}


int produce_num_in_range(int min, int max) {
	int num = max + 1;

	/* Intializes random number generator */

	struct timespec ts;
	timespec_get(&ts, TIME_UTC);
	srand(ts.tv_nsec);

	//srand(time(0)+clock());
	//srand(time(NULL));

	while (num < min || num > max)
		num = rand() % max + 1;

	return num;
}


int isNumber(char s[])
{
    for (int i = 0; s[i]!= '\0'; i++)
    {
        if (isdigit(s[i]) == 0)
              return 0;
    }
    return 1;
}
