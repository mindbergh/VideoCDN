#ifndef _MEDIA_H
#define _MEDIA_H


#include <sys/time.h>
#include <string.h>
#include "pool.h"


#define MAX_TITLE_LENGTH  100


extern serv_list_t *serv_list;


//#define SOCK_ADDR_IN_PTR(sa)	((struct sockaddr_in *)(sa))
//#define SOCK_ADDR_IN_ADDR(sa)	sa->sin_addr


typedef struct movie_s {
	char *title;
	int *bitrates;
	struct movies_s* next;
} movie_t;

typedef struct serv_list_s {
	uint32_t addr;
	int thruput;
	// to do: add 
	movie_t* movies;
	struct serv_info_s *next;
} serv_list_t;



int endsWith (char* base, char* str);
int update_thruput(size_t, struct timeval*, pool_t *, struct sockaddr_in*);

void init_serv_list(void);
void serv_add(sockaddr *);
void serv_del(sockaddr *);
serv_list_t* serv_get(sockaddr *);

#endif