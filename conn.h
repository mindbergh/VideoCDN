#ifndef _CONN_H
#define _CONN_H

#include <stdint.h>
#include <stdlib.h>


#define MAX_CONN 1024 /* The maximum num of connections */

typedef struct client_s {
    int fd;        /* client fd */
    unsigned int cur_size; /* current used size of this buf */
    unsigned int size;     /* whole size of this buf */
    uint32_t addr;
} client_t;

typedef struct server_s {
	int fd;
	unsigned int cur_size; /* current used size of this buf */
    unsigned int size;     /* whole size of this buf */ 
	uint32_t addr;
} server_t;

typedef struct conn_s {
	client_t* client;
	server_t* server;
	int thruput; /* the current thruput */
} conn_t;

conn_t* server_get_conn(int );
conn_t* client_get_conn(int ,uint32_t);
conn_t* add_conn(client_t* client, server_t* server);
void del_conn(conn_t* del_conn);
int update_thruput(size_t, struct timeval*, conn_t*);

/** @brief Close given connection
 *  @param p the Pool struct
 *         i the ith connection in the pool
 *  @return Void
 */

#endif