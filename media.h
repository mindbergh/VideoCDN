#ifndef _MEDIA_H
#define _MEDIA_H


#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>
#include "pool.h"


#define MAX_TITLE_LENGTH  100
#define MAX_BITRATE_LENGTH 10

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
	movie_t* movies;
	struct serv_list_s *next;
} serv_list_t;



int endsWith (char* base, char* str);
void init_serv_list(void);
serv_list_t* serv_add(struct sockaddr_in *);
void serv_del(struct sockaddr_in *);
serv_list_t* serv_get(struct sockaddr_in *);
void modi_path(char* path, int thruput);
int isVedio(char *);



#endif