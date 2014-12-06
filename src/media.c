#include "media.h"


serv_list_t *serv_list;


//double get_time_diff(struct timeval* start);


/** detecting whether base is ends with str
 *  @return 1 on ends with; 0 on does not end with
 */
int endsWith(char* base, char* str) {
    int blen = strlen(base);
    int slen = strlen(str);
    return (blen >= slen) && (0 == strcmp(base + blen - slen, str));
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

void modi_path(char* path, int thruput, conn_t* conn) {
	char buffer[MAXLINE] ={0};
	char* vod_index = NULL;
	char* seg_index = NULL;
	char rate[32];
	//char* slash;

//	fprintf(stderr, "old path:%s\n",path);

	vod_index = strstr(path,"/vod/");
	//slash = vod_index ;
	seg_index = strstr(path,"Seg");
	//while( strstr(slash+1,"/") != NULL) {
	//	slash = strstr(slash+1,"/");
	//}
	/* check if need to modify bitrate in uri */
	if (seg_index != NULL) {
		//fprintf(stderr, "old path:%s\n",path );
		strncpy(buffer,path,vod_index-path + 5);
		
		sprintf(rate, "%d", thruput);
		strcat(buffer,rate);
		strcat(buffer,seg_index);
		memset(path,0,MAXLINE);
		strcpy(path,buffer);
		strcpy(conn->cur_file,path);
		conn->cur_file[strlen(path)] = '\0';
	} 
	//fprintf(stderr, "new path:%s\n",path );		
}

int isVideo(char *path) {
	if (strstr(path, "Seg") != NULL && strstr(path, "Frag") != NULL) {
		return 1;
	} else {
		return 0;
	}
}