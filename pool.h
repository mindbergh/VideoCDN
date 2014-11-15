#ifndef _POOL_H
#define _POOL_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>

typedef struct pool_s {
	int maxfd;
	fd_set read_set; /* The set of fd Liso is looking at before recving */
	fd_set ready_read; /* The set of fd that is ready to recv */
	fd_set write_set; /* The set of fd Liso is looking at before sending*/
	fd_set ready_write; /* The set of fd that is ready to write */
	int nready; /* The # of fd that is ready to recv or send */
	int cur_conn; /* The current number of established connection */
	int maxi; /* The max index of fd */
	int client_sock[FD_SETSIZE]; /* array for client fd */
	Buff *buf[FD_SETSIZE]; /* array of points to buff */
} pool_t;

void init_pool(int listen_sock, Pool *p);

#endif


