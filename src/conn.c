#include "conn.h"
#include "pool.h"
#include "media.h"
#include "timer.h"



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
	server_t *serv = GET_SERV_BY_IDX(serv_idx);
	client_t *clit = GET_CLIT_BY_IDX(clit_idx);
	serv->num_clit++;
	clit->num_serv++;

	/* find the first available slot to add new connection */
	for (i = 0; i < FD_SETSIZE; i++) {
		if(conn[i] == NULL) {
			new_conn = (conn_t*)malloc(sizeof(conn_t));
			new_conn->clit_idx = clit_idx;
			new_conn->serv_idx = serv_idx;
			new_conn->t_put = 0;
			new_conn->avg_put = 0;
			new_conn->cur_bitrate = 10; // The lowest bitrate
			new_conn->alive = 1;
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

int update_conn(int clit_idx, int serv_idx) {
    int i = 0 ;
    conn_t** conn_l = pool.conn_l;
    for ( i = 0 ; i < FD_SETSIZE; i++) {
        if (conn_l[i] == NULL)
            continue;
        if (conn_l[i]->serv_idx == serv_idx 
            && conn_l[i]->clit_idx == clit_idx) {
            conn_l[i]->alive = 1;
        }
    }
    DPRINTF("wants to update, but no connection found!\n");
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
    int serv_idx = del_conn->serv_idx;
    int clit_idx = del_conn->clit_idx;
   	server_t *serv = GET_SERV_BY_IDX(serv_idx);
	client_t *clit = GET_CLIT_BY_IDX(clit_idx);
	assert(serv->num_clit > 0);
	assert(clit->num_serv > 0);
	serv->num_clit--;
	clit->num_serv--;

	if (serv->num_clit == 0)
    	close_serv(del_conn->serv_idx);
    if (clit->num_serv == 0)
    	close_clit(del_conn->clit_idx);

    free(del_conn);
    pool.conn_l[conn_idx] = NULL;
    pool.cur_conn--;

    //del_conn->alive = 0;
}; 


/* Thruput is in kbps */
int update_thruput(int sum, conn_t* conn, thruputs_t* thru) {
	assert(sum > 0);
	int curr_thruput;
	double elapsed = 0.0;

	double new_thruput;
	float alpha = pool.alpha;
	
	elapsed = get_elapsed(&(conn->start),&(conn->end));
	DPRINTF("elapsed = %lf", elapsed);
	new_thruput = ((sum / 1000 * 8)) / elapsed;
	conn->t_put = (int)new_thruput;

	
	curr_thruput = thru->avg_put;
	DPRINTF(" Old:%d, New:%f, alpha:%f\n", curr_thruput, new_thruput, alpha);
	if (curr_thruput != 0) {
		new_thruput = (alpha * new_thruput + (1.0 - alpha) * curr_thruput);
	}
	thru->avg_put = (int)new_thruput;
	fprintf(stderr, " New avg_put:%d\n", thru->avg_put);
	return (int)new_thruput; 
}


int get_thru_by_addrs(uint32_t clit_addr, uint32_t serv_addr) {
	int i = 0;
	thruputs_t** thru = pool.thru_l;

	for (i = 0; i<= pool.max_thru_idx; i++) {
		if (thru[i] == NULL) 
			continue;
		
		if(thru[i]->clit_addr == clit_addr && thru[i]->serv_addr == serv_addr) {
			return i;
		}
	}
	return -1;
}

int add_thru(uint32_t clit_addr, uint32_t serv_addr) {
    thruputs_t** thru = pool.thru_l;
    thruputs_t* new_thru;
    int i = 0;

    for(; i < FD_SETSIZE;i++) {
        if (thru[i] == NULL) {
            new_thru = (thruputs_t*)malloc(sizeof(thruputs_t));
            new_thru->clit_addr = clit_addr;
            new_thru->serv_addr = serv_addr;
            new_thru->avg_put = 0;
            thru[i] = new_thru;
            
            if (i > pool.max_thru_idx) {
                pool.max_thru_idx = i;
            }
            return i;
        }
    }
    /* failed to add new server */
    return -1;
}
