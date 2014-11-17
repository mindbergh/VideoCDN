#ifndef _MEDIA_H
#define _MEDIA_H


#include <sys/time.h>
#include <string.h>
#include "pool.h"



int endsWith (char* base, char* str);
void update_thruput(size_t, struct timeval*, pool_t *, int);


#endif