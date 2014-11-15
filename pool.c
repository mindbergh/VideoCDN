#include "pool.h"

/** @brief Initial the pool of client fds to be select
* @param listen_sock The socket on which the server is listenning
* while initial, this should be the greatest fd
* @param p the pointer to the pool
* @return Void
*/
void init_pool(int listen_sock, Pool *p) {
int i;
	p->maxi = -1;
	for (i = 0; i < FD_SETSIZE; i++)
	p->buf[i] = NULL;
	p->maxfd = listen_sock;
	p->cur_conn = 0;
	FD_ZERO(&p->read_set);
	FD_ZERO(&p->write_set);
	FD_SET(listen_sock, &p->read_set);
}