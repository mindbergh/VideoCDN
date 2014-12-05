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
#include <stdio.h>
#include <arpa/inet.h>

void usage();
void init_ref();
int init_udp(char* ip, int port, fd_set* read_set);
void serve(int fd, int rr_flag);
int parse(char* buf);
int gen_err(char* origin);
int gen_res(char* req_buf, char* res_buf, char* dest_addr);



#endif