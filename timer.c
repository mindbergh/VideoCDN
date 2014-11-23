#include "timer.h"
#include "debug.h"


/** @brief Get a time interval between now and given start time
 *  @param start The start time point
 *  @return a double represent time interval in second.
 */
double get_time_diff(struct timeval* start) {
	struct timeval now;
	double t1 = start->tv_sec+(start->tv_usec/1000000.0);
	double t2;
	gettimeofday(&now, NULL);
	t2=now.tv_sec+(now.tv_usec/1000000.0);
	t2 = t2 - t1;
	//printf("T2 = %f", t2);
	return t2;
}

/** @brief Get a time interval between two time stamp
 *  @param start The start time point
 *  @param end The end point
 *  @return a double represent time interval in second.
 */
double get_elapsed(struct timeval* start, struct timeval* end) {
	double t1 = start->tv_sec+(start->tv_usec/1000000.0);
	double t2 = end->tv_sec+(end->tv_usec/1000000.0);
	return (t2 - t1);	
}