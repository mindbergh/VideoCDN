#include "conn.h"
#include "pool.h"
#include "media.h"

extern pool_t pool;

conn_t* client_get_conn(int fd, uint32_t addr) {
	int i = 0;
	conn_t** conn = pool.conn_l;
	for( i =0; i<pool.cur_conn;i++) {
		if (conn[i] == NULL) 
			continue;
		if(conn[i]->client->fd == fd 
			&& conn[i]->server->addr == addr) {
			return conn[i];
		}
	}
	return NULL;
}

conn_t* server_get_conn(int fd) {
	int i = 0;
	conn_t** conn = pool.conn_l;
	for( i =0; i<pool.cur_conn;i++) {
		if (conn[i] == NULL) 
			continue;
		if(conn[i]->server->fd == fd) {
			return conn[i];
		}
	}
	return NULL;
}


conn_t* add_conn(client_t* client, server_t* server) {
	conn_t* new_conn;
	conn_t** conn = pool.conn_l;
	int i = 0;

	/* find the first available slot to add new connection */
	for( i =0; i<=pool.cur_conn;i++) {
		if(conn[i] == NULL) {
			new_conn = (conn_t*)malloc(sizeof(conn_t));
			new_conn->client = client;
			new_conn->server = server;
			new_conn->thruput = 0;
			conn[i] = new_conn;
			pool.cur_conn++;
			return new_conn;
		}
	}
	/* failed to add connection */
	return NULL;
}

void del_conn(conn_t* del_conn) {
    //to do
    close_client();
    close_server();
    close_socket(del_conn->server->fd);
    close_socket(del_conn->client->fd);
}; 

int update_thruput(size_t sum, struct timeval* start, 
	conn_t* conn) {
	int curr_thruput;
	double elapsed = get_time_diff(start);
	double new_thruput = sum / elapsed;
	float alpha = pool.alpha;
	serv_list_t *serv_info;
	/*
	serv_info = serv_get(conn->server->addr);
	if (serv_info == NULL) {
		serv_info = serv_add(addr);
		curr_thruput = serv_info->thruput;
	}

	if (curr_thruput != -1) {
		new_thruput = (int)(alpha * curr_thruput + (1 - alpha) * new_thruput);
		
	}
	conn->thruput = (int)new_thruput;
	*/
	return (int)new_thruput; 
}