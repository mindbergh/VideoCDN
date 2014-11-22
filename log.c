#include "log.h"
#include <stdlib.h>

extern pool_t pool;

void loggin(conn_t* conn) {
	int elapsed = (int)get_time_diff(&(pool.start));
	double secs = get_elapsed(conn->start,&(conn->end));
	FILE* logfile = pool.log_file;
	
	fprintf(logfile, "%d ", elapsed);
	fprintf(logfile, "%lf ", secs);
	fprintf(logfile, "%d ", conn->t_put);
	fprintf(logfile, "%d ", conn->avg_put);
	fprintf(logfile, "%d ", 100);
	fprintf(logfile, "%s ", "10.0.0.1");
	fprintf(logfile, "%s\n", conn->cur_file );

	fflush(pool.log_file);
}