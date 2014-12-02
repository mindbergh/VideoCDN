#ifndef _NAMESERVER_H
#define _NAMESERVER_H

#include <netdb.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> 
#include <string.h>
#include <netdb.h>
#include "debug.h"

void usage();
int init_udp(char* ip, int port, fd_set* read_set);
void serve(int fd);
int parse(data_packet_t* pkt);
data_packet_t* gen_err(data_packet_t* );
data_packet_t* gen_res(data_packet_t*, (struct sockaddr *) );



#endif