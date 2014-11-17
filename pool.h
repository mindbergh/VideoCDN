#ifndef _POOL_H
#define _POOL_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include "mydns.h"

#define BUF_SIZE 8192 /* Initial buff size */
#define MAXLINE  8192
#define MAXBUF   8192
#define MAX_SIZE_HEADER 8192 /* Max length of size info for the incomming msg */
#define LISTENQ 1024 /* second argument to listen() */
#define VERBOSE 1 /* Whether to print out debug infomations */

/** @brief The buff struct that keeps track of current 
 *         used size and whole size
 *
 */
typedef struct conn_s {
    
    int fd;        /* client fd */
    unsigned int cur_size; /* current used size of this buf */
    unsigned int size;     /* whole size of this buf */
    unsigned int thruput;
} conn_t;

typedef struct pool_s {
	int maxfd;
	int serv_sock;
	float alpha;
	fd_set read_set; /* The set of fd Liso is looking at before recving */
	fd_set ready_read; /* The set of fd that is ready to recv */
	fd_set write_set; /* The set of fd Liso is looking at before sending*/
	fd_set ready_write; /* The set of fd that is ready to write */
	int nready; /* The # of fd that is ready to recv or send */
	int cur_conn; /* The current number of established connection */
	int maxi; /* The max index of fd */
	conn_t *conn[FD_SETSIZE]; /* array of points to buff */
} pool_t;



void init_pool(int, pool_t *);
int open_listen_socket(int);
int open_server_socket(char *, char *);
void add_client(int conn_sock, pool_t *p);
void free_buf(pool_t *, conn_t *);
int close_socket(int);
void clean_state(pool_t *, int);
#endif


