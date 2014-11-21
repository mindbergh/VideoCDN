#ifndef _POOL_H
#define _POOL_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include "conn.h"
#include "mydns.h"

#define BUF_SIZE 8192 /* Initial buff size */
#define MAXLINE  8192
#define MAXBUF   8192
#define MAX_SIZE_HEADER 8192 /* Max length of size info for the incomming msg */
#define LISTENQ 1024 /* second argument to listen() */
#define VERBOSE 1 /* Whether to print out debug infomations */

typedef struct pool_s {
	int maxfd;
	int serv_sock;
	float alpha;
	char *fake_ip;
	char *www_ip;
	fd_set read_set; /* The set of fd proxy is looking at before recving */
	fd_set ready_read; /* The set of fd that is ready to recv */
	fd_set write_set; /* The set of fd proxy is looking at before sending*/
	fd_set ready_write; /* The set of fd that is ready to write */
	int nready; /* The # of fd that is ready to recv or send */
	int cur_conn; /* The current number of established connection */
	int cur_client;	/* The current number of connected client */
	int cur_server; /* The current number of connected server */
	int maxi; /* The max index of fd */
	client_t* client_l[FD_SETSIZE];
 	server_t* server_l[FD_SETSIZE];
	conn_t* conn_l[FD_SETSIZE]; /* array of points to all connections */
} pool_t;

void init_pool(int, pool_t *,char**);
int open_listen_socket(int);
int open_server_socket(char *, char *);
client_t* add_client(int sock, uint32_t addr);
server_t* add_server(int sock, uint32_t addr);
client_t* get_client(int sock);
server_t* get_server(int sock);
void free_buf(pool_t *, conn_t *);
int close_socket(int);
void clean_state(pool_t *, int);
#endif




