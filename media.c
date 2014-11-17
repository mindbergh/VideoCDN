#include "media.h"


double get_time_diff(struct timeval* start);


/** detecting whether base is ends with str
 *  @return 1 on ends with; 0 on does not end with
 */
int endsWith(char* base, char* str) {
    int blen = strlen(base);
    int slen = strlen(str);
    return (blen >= slen) && (0 == strcmp(base + blen - slen, str));
}



void update_thruput(size_t sum, struct timeval* start, pool_t* p, int i) {
	conn_t *conni = p->conn[i];
	unsigned int curr_thruput = conni->thruput;
	double elapsed = get_time_diff(start);
	double new_thruput = sum / elapsed;
	float alpha = p->alpha;

	if (curr_thruput == 0) {
		conni->thruput = (unsigned int)new_thruput;
		return;
	} else {
		conni->thruput = (unsigned int)(alpha * curr_thruput + (1 - alpha) * new_thruput);
	}
 

}

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
	return (t2 - t1);
}
