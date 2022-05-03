#ifndef THREADING
#define THREADING

#include <pthread.h>

int open_thread(int*, int);
int join_thread(pthread_t*, int*, int);

#endif