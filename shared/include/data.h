/*
 * data.h
 *
 *  Created on: 13 jul. 2022
 *      Author: utnso
 */

#ifndef INCLUDE_DATA_H_
#define INCLUDE_DATA_H_



typedef struct t_data {
	void*			info;
	struct t_data*	next;
} t_data;


/*************************** t_data *****************************************************************/
t_data* data_create(void* _info);
void data_add(t_data** head, void* _info);
void data_destroy(t_data** head);
void data_destroy_and_destroy_info(t_data** head, void(*destroy_info)(void*));


#endif /* INCLUDE_DATA_H_ */
