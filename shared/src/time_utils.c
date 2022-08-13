/*
 * time_utils.c
 *
 *  Created on: 4 jun. 2022
 *      Author: utnso
 */

#include "../include/time_utils.h"


/**
 * @return milliseconds
 */
uint64_t get_time_stamp() {
  struct timespec spec;

  if (clock_gettime(CLOCK_REALTIME/*1*/, &spec) == -1) { /* 1 is CLOCK_MONOTONIC */
    return -1;
  }

  return spec.tv_sec * 1000 + spec.tv_nsec / 1e6;
}


/*
uint64_t get_time_stamp() {
	struct timespec* log_timespec = malloc(sizeof(struct timespec));
	//struct tm* log_tm = malloc(sizeof(struct tm));
	//char* milisec;

	if(clock_gettime(CLOCK_REALTIME, log_timespec) == -1) {
		return -1;
	}

	uint64_t milisec = log_timespec->tv_nsec / 1000000;
	free(log_timespec);

	return milisec;

	//milisec = string_from_format("%03ld", log_timespec->tv_nsec / 1000000);

}
*/
