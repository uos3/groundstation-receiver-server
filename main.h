#ifndef _MAIN_H
#define _MAIN_H

#define __STDC_FORMAT_MACROS

#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <pthread.h>

typedef struct thread_t {
	
	/* pthread instance */
	pthread_t thread;
	
	/* Thread name */
	char* name;
	
	/* Target function */
	void* function;
	
	/* Target function argument */
	void* arg;
	
} thread_t;

/* Declare threads */
/** { [pthread_t], name[64], function, arg } **/
#define THREADS_NUMBER   5
#define DECLARE_THREADS() \
    thread_t threads[THREADS_NUMBER] = { \
        { 0, "SDR            ", rx_sdr,           NULL }, \
        { 0, "TCP Socket     ", rx_tcp_socket,    NULL }, \
        { 0, "RTLTCP Socket  ", rx_rtltcp_socket, NULL }, \
        { 0, "TCP Feed       ", rx_tcp_feed,      NULL }, \
        { 0, "RTLTCP Feed    ", rx_rtltcp_feed,   NULL }, \
    };

#endif