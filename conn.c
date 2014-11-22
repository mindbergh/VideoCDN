#include "conn.h"
#include "pool.h"
#include "media.h"

extern pool_t pool;



/** @brief 
 *	@param fd 
 *  @param 
 *  @return the index to the conn_l[], -1 on not found
 */
int client_get_conn(int fd, uint32_t addr) {
	int i = 0;
	conn_t** conns = pool.conn_l;
	server_t *server;
	client_t *client;
	for (i = 0; i<= pool.max_conn_idx; i++) {
		if (conns[i] == NULL) 
			continue;
		server = GET_SERV_BY_IDX(conns[i]->serv_idx);
		client = GET_CLIT_BY_IDX(conns[i]->clit_idx);
		if(client->fd == fd && server->addr == addr) {
			return i;
		}
	}
	return -1;
}

/** @brief 
 *	@param fd 
 *  @param 
 *  @return the index to the conn_l[], -1 on not found
 */
int server_get_conn(int fd) {
	int i = 0;
	conn_t** conns = pool.conn_l;
	server_t *server;

	for (i = 0; i <= pool.max_conn_idx; i++) {
		if (conns[i] == NULL)
			continue;
		server = GET_SERV_BY_IDX(conns[i]->serv_idx);
		if(server->fd == fd) {
			return i;
		}
	}
	return -1;
}

/** @brief 
 *	@param fd 
 *  @param 
 *  @return the index to the conn_l[], -1 on error
 */
int add_conn(int clit_idx, int serv_idx) {
	conn_t* new_conn;
	conn_t** conn = pool.conn_l;
	int i = 0;

	/* find the first available slot to add new connection */
	for (i = 0; i < FD_SETSIZE; i++) {
		if(conn[i] == NULL) {
			new_conn = (conn_t*)malloc(sizeof(conn_t));
			new_conn->clit_idx = clit_idx;
			new_conn->serv_idx = serv_idx;
			new_conn->t_put = 0;
			new_conn->avg_put = 0;
			conn[i] = new_conn;
			pool.cur_conn++;

			if (i > pool.max_conn_idx) {
				pool.max_conn_idx = i;
			}
			return i;
		}
	}
	/* failed to add connection */
	return -1;
}

/** @brief 
 *	@param fd 
 *  @param 
 *  @return Void
 */
void close_conn(int conn_idx) {
    //to do
    conn_t* del_conn= GET_CONN_BY_IDX(conn_idx);
    close_clit(del_conn->clit_idx);
    close_serv(del_conn->serv_idx);
    pool.conn_l[conn_idx] = NULL;
    pool.cur_conn--;
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