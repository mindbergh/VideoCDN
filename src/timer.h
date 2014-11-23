#ifndef _TIMER_H
#define _TIMER_H

#include <sys/time.h>
#include <stdlib.h>

double get_time_diff(struct timeval* start);
double get_elapsed(struct timeval* start, struct timeval* end);
#endif