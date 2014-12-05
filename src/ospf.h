#ifndef _OSPF_H
#define _OSPF_H

#include <mlib/mlist.h>
#include <mlib/mqueue.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>


#define NAME_LENGTH 20



extern MList *servs; // data is a MList* of nodes
extern MList *clits; // data is a MList* of nodes
extern MList *nodes; // data is a node_t*
extern MList *routing_table;  // data is a rt_t*

typedef struct LSA_config_s {
	char *servers;
	char *LSAs;
	uint32_t num_servs;
} LSA_config_t;

typedef struct node_s {
	char name[NAME_LENGTH];
	MList* neighbors;  // each data would be a idx of nodes
	int seq_num;
	int mark; // has bfs reached this or not?
} node_t;

typedef struct rt_s {
	node_t* clit;
	node_t* serv;
} rt_t;



void OSPF_init(char *servers, char *LSAs, int rr_flag);
char* route(char* clit_name, int rr_flag);

#endif