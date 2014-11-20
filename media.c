#include "media.h"
#include "debug.h"
#include <string.h>

serv_list_t *serv_list;


double get_time_diff(struct timeval* start);


/** detecting whether base is ends with str
 *  @return 1 on ends with; 0 on does not end with
 */
int endsWith(char* base, char* str) {
    int blen = strlen(base);
    int slen = strlen(str);
    return (blen >= slen) && (0 == strcmp(base + blen - slen, str));
}



int update_thruput(size_t sum, struct timeval* start, pool_t* p, struct sockaddr_in *addr) {
	int curr_thruput;
	double elapsed = get_time_diff(start);
	double new_thruput = sum / elapsed;
	float alpha = p->alpha;
	serv_list_t *serv_info;

	serv_info = serv_get(addr);
	if (serv_info == NULL) {
		serv_info = serv_add(addr);
		curr_thruput = serv_info->thruput;
	}

	if (curr_thruput != -1) {
		new_thruput = (int)(alpha * curr_thruput + (1 - alpha) * new_thruput);
		
	}
	serv_info->thruput = (int)new_thruput;
	return (int)new_thruput; 
 

}

/** @brief Get a time interval between now and given start time
 *  @param start The start time point
 *  @return a double represent time interval in second.
 */
double get_time_diff(struct timeval* start) {
	struct timeval now;
	double t1 = start->tv_sec+(start->tv_usec/1000000.0);
	double t2;
	gettimeofday(&now, NULL);
	t2=now.tv_sec+(now.tv_usec/1000000.0);
	return (t2 - t1);
}


void init_serv_list() {
	serv_list = NULL;
}



serv_list_t* serv_add(struct sockaddr_in *serv) {
	serv_list_t *ptr = serv_list;
	serv_list_t *tmp;

	tmp = (serv_list_t*)malloc(sizeof(serv_list_t));
	tmp->addr = serv->sin_addr.s_addr;
	tmp->thruput = 0;
	tmp->next = NULL;
	if(ptr!= NULL) {
		while (ptr->next != NULL)
			ptr = ptr->next;
		ptr->next = tmp;
	} else {
		serv_list = tmp;
	}
	return tmp;
}


void serv_del(struct sockaddr_in *serv) {
	serv_list_t *curr = serv_list;
	serv_list_t *prev = NULL;
	uint32_t addr = serv->sin_addr.s_addr;


	for (curr = serv_list;
			curr != NULL;
			prev = curr, curr = curr->next) {

    if (curr->addr == addr) {  /* Found it. */
      if (prev == NULL) {
        /* Fix beginning pointer. */
        serv_list = curr->next;
      } else {
        /*
         * Fix previous node's next to
         * skip over the removed node.
         */
        prev->next = curr->next;
      }

      /* Deallocate the node. */
      free(curr);

      /* Done searching. */
      return;
    }
  }
}


serv_list_t* serv_get(struct sockaddr_in *serv) {
	serv_list_t *ptr;
	uint32_t addr = serv->sin_addr.s_addr;
	for (ptr = serv_list; ptr != NULL; ptr = ptr->next) {
		if (ptr->addr == addr) {
			break;
		}
	}
	if (ptr != NULL) {
		assert(ptr->thruput != -1);
		return ptr;
	}
	return NULL;
}

void modi_path(char* path, int thruput) {
	char buffer[MAXLINE] ={0};
	char* vod_index = NULL;
	char* seg_index = NULL;

	vod_index = strstr(path,"/vod/");
	seg_index = strstr(path,"Seg");

	/* check if need to modify bitrate in uri */
	if( seg_index != NULL) {
		fprintf(stderr, "old path:%s\n",path );
		strncpy(buffer,path,vod_index-path+5);
		strcat(buffer,"100");
		strcat(buffer,seg_index);
		memset(path,0,MAXLINE);
		strcpy(path,buffer);
		fprintf(stderr, "new path:%s\n",path );
	} 
}