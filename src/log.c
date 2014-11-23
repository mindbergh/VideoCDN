#include "log.h"
#include <stdlib.h>

extern pool_t pool;

void loggin(conn_t* conn) {
	int elapsed = (int)get_time_diff(&(pool.start));
	double secs = get_elapsed(conn->start,&(conn->end));
	FILE* logfile = pool.log_file;
	server_t* server = pool.server_l[conn->serv_idx];
	
	struct in_addr ip_addr;
    ip_addr.s_addr = server->addr;

	fprintf(logfile, "%d ", elapsed);
	fprintf(logfile, "%lf ", secs);
	fprintf(logfile, "%d ", conn->t_put);
	fprintf(logfile, "%d ", conn->avg_put);
	fprintf(logfile, "%d ", conn->cur_bitrate);
	fprintf(logfile, "%s ", inet_ntoa(ip_addr));
	fprintf(logfile, "%s\n", conn->cur_file );

	fflush(pool.log_file);
}