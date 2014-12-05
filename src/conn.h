#ifndef _CONN_H
#define _CONN_H

#include <stdint.h>
#include <stdlib.h>


#define MAX_CONN 1024 /* The maximum num of connections */
#define MAX_FILE_NAME 8192 /* The maximum length of file name */


#define GET_CONN_BY_IDX(idx) (pool.conn_l[idx]);

typedef struct client_s {
    int fd;        /* client fd */
    unsigned int cur_size; /* current used size of this buf */
    unsigned int size;     /* whole size of this buf */
    uint32_t addr;
    int num_serv;
} client_t;

typedef struct server_s {
	int fd;
	unsigned int cur_size; /* current used size of this buf */
    unsigned int size;     /* whole size of this buf */ 
	uint32_t addr;
	int num_clit;
} server_t;

typedef struct conn_s {
	int serv_idx;
	int clit_idx;
	int t_put; /* the current thruput */
	int avg_put; /* current EWMA thruput estimate in Kbps */
	int cur_bitrate;
	char cur_file[MAX_FILE_NAME];
	char cur_size;
	struct timeval start;
	struct timeval end;
	int alive; /* 1 when connection is alive; 0 when connection is closed */
} conn_t;

typedef struct response_s {
	int length;
	int type;
	char *hdr_buf;
	int hdr_len;
} response_t;
	


int server_get_conn(int );
int client_get_conn(int ,uint32_t);
int add_conn(int, int);
int update_conn(int clit_idx, int serv_idx);
void close_conn(int);
int update_thruput(int, conn_t*);

#endif