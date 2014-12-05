#include "ospf.h"
#include <assert.h>
#include <string.h>

#define BUFSIZE 100

static void print_topo(void* data, void* func_data);
static void print_table(void* data, void* func_data);
static void shortest_path(void* data, void* func_data);
static void unmark(void* data, void* func_data);
static int parse_servs(char *servers);
static int parse_LSA(char *LSAs);
static MList* add_new_node(const char* name);
static void add_new_neighbor(node_t* node, const MList* neighbor);
static int find_node_by_name(const void* a, const void* b);
static int find_entry_by_name(const void* a, const void* b);
static int startsWith(char* base, char* str);

MList *servs; // data is a MList* of nodes
MList *clits; // data is a MList* of nodes
MList *nodes; // data is a node_t*
MList *routing_table;  // data is a rt_t*
uint32_t query_count;

void OSPF_init(char *servers, char *LSAs, int rr_flag) {
	parse_servs(servers);
	if (!rr_flag)
		parse_LSA(LSAs);
	else
		return;
#ifdef DEBUG1
	mlist_foreach(nodes, print_topo, NULL);
#endif	

	mlist_foreach(clits, shortest_path, NULL);

#ifdef DEBUG1
	mlist_foreach(routing_table, print_table, NULL);
#endif
}


char* route(char* clit_name, int rr_flag) {
	
	if (rr_flag) {
		uint32_t size = mlist_length(nodes);
		node_t* node = (node_t*)mlist_getdata(nodes, query_count % size);
		query_count++;
		return node->name;
	} else {
		MList* list = mlist_find_custom(routing_table, clit_name, find_entry_by_name);
		rt_t* entry = (rt_t*)list->data;
		return entry->serv->name;
	}
	return NULL;	
}


static void print_table(void* data, void* func_data) {
	rt_t* table = (rt_t*)(data);
	fprintf(stderr, "%s  --->  %s\n", table->clit->name, table->serv->name);
	return;
}

static void print_topo(void* data, void* func_data) {
	node_t* node = (node_t*)data;
	fprintf(stderr, "%s : ", node->name);
	MList* neighbor_iter = node->neighbors;
	while (neighbor_iter) {
		node_t* tmp = (node_t*)((MList*)neighbor_iter->data)->data;
		fprintf(stderr, "\t%s", tmp->name);
		neighbor_iter = neighbor_iter->next;
	}
	fprintf(stderr, "\n");
}


// a func that is for foreach
static void shortest_path(void* data, void* func_data) {
	node_t* clit = (node_t*)((MList*)data)->data;
	node_t* this_node = NULL;
	mlist_foreach(nodes, unmark, NULL);

	MQueue* q = mqueue_new();
	mqueue_push_tail(q, (void*)clit);

	rt_t* new_entry = (rt_t*)malloc(sizeof(rt_t));
	routing_table = mlist_append(routing_table, (void*)new_entry);
	new_entry->clit = clit;
	new_entry->serv = NULL;

	clit->mark = 1; // this node has reached
	while (!mqueue_is_empty(q)) {
		this_node = (node_t*)mqueue_pop_head(q);
		MList* neighbor_iter = this_node->neighbors;
		while (neighbor_iter) {
			node_t* tmp = (node_t*)((MList*)neighbor_iter->data)->data;

			if (tmp->mark) {	
				// this node has reached before
				neighbor_iter = neighbor_iter->next;
				continue;
			}
			if (mlist_find(servs, neighbor_iter->data)) {
				// a server find, search complete
				new_entry->serv = tmp;
				return;
			}
			mqueue_push_tail(q, (void*)tmp);
			tmp->mark = 1;
		}
	}
	// should never reach here
	assert(0);
}

// a func that is for foreach
static void unmark(void* data, void* func_data) {
	((node_t*)data)->mark = 0;
	return;
}


static int parse_servs(char *servers) {
	FILE *fserv;
	char name[BUFSIZE];
	MList* tmp;
	
	fserv = fopen(servers, "r");
	if (fserv == NULL) return -1;

	while (fgets(name, BUFSIZE, fserv)) {
		//char *this_addr = (char *)malloc(strlen(buf) + 1);
		//strcmp(this_addr, buf);
		name[strlen(name) - 1] = '\0'; //remove ending '\n'
		tmp = add_new_node(name);
		servs = mlist_append(servs, (void*)tmp);
	}
	fclose(fserv);
	return 0;
}


static int parse_LSA(char *LSAs) {
	FILE *fLSA;
	char buf[BUFSIZE];
	MList* tmp;
	node_t* tmp_node;

	char name[NAME_LENGTH];
	int seq_num;
	char neighbors[BUFSIZE];
	char* neighbor;


	fLSA = fopen(LSAs, "r");
	if (fLSA == NULL) return -1;

	while (fgets(buf, BUFSIZE, fLSA)) {
		sscanf(buf, "%s %d %s", name, &seq_num, neighbors);

		tmp = add_new_node(name);
		tmp_node = (node_t*)tmp->data;

		/* make client list if necessary */
		if (!mlist_find(servs, (void*)tmp)) {
			// this is not a server
			if (!startsWith(name, "router")) {
				// this is not a router either, then this is a client for sure
				if (!mlist_find(clits, (void*)tmp)) {
					// do I add this already?
					clits = mlist_append(clits, (void*)tmp);
				}
			}
		}

		if (tmp_node->seq_num >= seq_num) continue; // skip if the seq is not more recent
		tmp_node->seq_num = seq_num;

		mlist_free(tmp_node->neighbors);
		tmp_node->neighbors = NULL;  // free all and re-add all neighbors

		neighbor = strtok(neighbors, ",");
		while (neighbor) {
			tmp = add_new_node(neighbor); // tmp would be a MList* of node_t*
			add_new_neighbor(tmp_node, tmp);
			neighbor = strtok(NULL, ",");
		}
	}
	fclose(fLSA);
	return 0;
}

/** add a node with the given name in the list or return it if it exsits
 */
static MList* add_new_node(const char* name) {
	MList* tmp;

	tmp = mlist_find_custom(nodes, name, find_node_by_name);
	if (tmp == NULL) {
		node_t* new_node = (node_t*)malloc(sizeof(node_t));
		strcpy(new_node->name, name);
		new_node->neighbors = NULL;
		new_node->seq_num = -1;
		nodes = mlist_append(nodes, (void*)new_node);
		tmp = mlist_find_custom(nodes, name, find_node_by_name);
	}
	return tmp;
}

static void add_new_neighbor(node_t* node, const MList* neighbor) {
	MList* tmp;

	tmp = mlist_find(node->neighbors, (void*)neighbor);
	if (tmp == NULL) {
		node->neighbors = mlist_append(node->neighbors, (void*)neighbor);
	}
	return;
}


static int find_node_by_name(const void* a, const void* b) {
	// a would be a node_t
	// b would be a char*
	const node_t* this_node = (const node_t*)a;
	const char* that_name = (const char*)b;
	return strcmp(this_node->name, that_name);
}

static int find_entry_by_name(const void* a, const void* b) {
	// a woule be a rt_t*
	// b would be a char*
	const rt_t* entry = (rt_t*)a;
	const char* name = (char*)b;
	return strcmp(entry->clit->name, name);
}


static int startsWith(char* base, char* str) {
    int blen = strlen(base);
    int slen = strlen(str);
    return (blen >= slen) && (0 == strncmp(base, str, slen));
}



/*
int main(int argc, char** argv) {
	OSPF_init(argv[1], argv[2]);	
#ifdef DEBUG
	fprintf(stderr, "\nRouting test:\n");
	fprintf(stderr, "route: 1.0.0.1  --->  %s\n", route("1.0.0.1"));
	fprintf(stderr, "route: 2.0.0.1  --->  %s\n", route("2.0.0.1"));
	fprintf(stderr, "route: 3.0.0.1  --->  %s\n", route("3.0.0.1"));
#endif
	return 0;
}

*/