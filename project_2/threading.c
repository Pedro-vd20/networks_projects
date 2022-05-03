#include <pthread.h>

#include "threading.h"

/**
 * @brief find index of first available thread and flags that thread as "busy"
 * 
 * @param busy list containing 1s for busy threads
 * @param size length of busy
 * @return int index of next available thread, -1 if none available 
 */
int open_thread(int* busy, int size) {
	for(int i = 0; i < size; ++i) {
		if(!busy[i]) {
			busy[i] = 1;	// flag as busy for later iterations
			return i;
		}
	}

	// if none found, return -1 (reject request)
	return -1;
}

/**
 * @brief closes currently running threads and flags closed threads as available
 * 
 * @param threads list of possibly running threads
 * @param busy list flagging currently running threads
 * @param size num of threads
 * @return int 0 if all succeeds, -1 if there was an error closing a thread
 */
int join_thread(pthread_t* threads, int* busy, int size) {
	for(int i = 0; i < size; ++i) {
		if(busy[i]) {
			if(pthread_join(threads[i], NULL) == 0) {
				busy[i] = 0;
			}
			else {
				return -1; // error joining thread
			}
		}
	}

	return 0;	// no errors happened
}

