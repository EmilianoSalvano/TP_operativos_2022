/*
 * utils.h
 *
 *  Created on: 7 jun. 2022
 *      Author: utnso
 */

#ifndef INCLUDE_UTILS_H_
#define INCLUDE_UTILS_H_


/*
 *	@NAME: produce_num_in_range
 *	@DESC: produce un numero random entre minimo y maximo definido
 */
int produce_num_in_range(int min, int max);

/*
 *	@NAME: produce_num
 *	@DESC: produce un numero random entre 0 y el maximo definido
 */
int produce_num(int max);


/*
 *	@NAME: isNumber
 *	@DESC: comprueba si la cadena es un numero. Devuelve 1 si es numero y 0 si no lo es
 */
int isNumber(char s[]);

#endif /* INCLUDE_UTILS_H_ */
