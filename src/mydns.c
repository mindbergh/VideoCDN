#include "mydns.h"

#define VIDEO_SERVER_QUERY "5video2cs3cmu3edu00101"

/* globle varibal for dns service */
dns_t dns;

/**
 * Initialize your client DNS library with the IP address and port number of
 * your DNS server.
 *
 * @param  dns_ip  The IP address of the DNS server.
 * @param  dns_port  The port number of the DNS server.
 *
 * @return 0 on success, -1 otherwise
 */
int init_mydns(const char *dns_ip, unsigned int dns_port) {
  int sock;
  struct sockaddr_in myaddr;
  
  if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) == -1) {
    perror("peer_run could not create socket");
    exit(-1);
  }
  
  bzero(&myaddr, sizeof(myaddr));
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = htons(0);
  
  if (bind(sock, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
    perror("peer_run could not bind socket");
    exit(-1);
  }

  /* fill the address of DNS server */
  dns.sock = sock;
  memset((char*)&(dns.servaddr), 0, sizeof(dns.servaddr));
  dns.servaddr.sin_family = AF_INET;
  dns.servaddr.sin_port = htons(dns_port);
  inet_pton(AF_INET, dns_ip,&(dns.servaddr.sin_addr));

  return 0;
}


/**
 * Resolve a DNS name using your custom DNS server.
 *
 * Whenever your proxy needs to open a connection to a web server, it calls
 * resolve() as follows:
 *
 * struct addrinfo *result;
 * int rc = resolve("video.cs.cmu.edu", "8080", null, &result);
 * if (rc != 0) {
 *     // handle error
 * }
 * // connect to address in result
 * free(result);
 *
 *
 * @param  node  The hostname to resolve.
 * @param  service  The desired port number as a string.
 * @param  hints  Should be null. resolve() ignores this parameter.
 * @param  res  The result. resolve() should allocate a struct addrinfo, which
 * the caller is responsible for freeing.
 *
 * @return 0 on success, -1 otherwise
 */

int resolve(const char *node, const char *service, 
            const struct addrinfo *hints, struct addrinfo **res) {
	int recvlen = 0;
	int randnum = 0;
	struct addrinfo *tmp;
	data_packet_t* pkt;
	char buf[BUFSIZE];

	*res = malloc(sizeof(struct addrinfo));
	tmp = *res;
	memset(tmp, 0, sizeof(struct addrinfo)); // make sure the struct is empty
	tmp->ai_flags = AI_PASSIVE;
	tmp->ai_family = AF_INET;
	tmp->ai_socktype = SOCK_STREAM;
	tmp->ai_protocol = 0;
	tmp->ai_addrlen = sizeof(struct sockaddr_in);
	tmp->ai_addr = malloc(sizeof(struct sockaddr_in));
	tmp->ai_canonname = NULL;
	tmp->ai_next = NULL;

	// generate query pkt
	if((pkt	= q_pkt_maker(node,service,&randnum)) == NULL) {
		DPRINTF("failed to generate query!\n");
		return 0;
	}

	// send query to DNS server
	sendto(dns.sock, pkt, strlen(pkt), 0, (struct sockaddr *)&dns.servaddr, 
		sizeof(dns.servaddr));

	// recv response from DNS server
	while ((recvlen = recvfrom(dns.sock,buf,
		BUFSIZE,0, (struct sockaddr *)&dns.servaddr, 
		sizeof(dns.servaddr))) == 0 );

	// parse response
	if (parse_res(buf,tmp,randnum) != 0 ) {
		DPRINTF("Fail to parse DNS response!");
		return -1;
	}
	// fill up port info
	((struct sockaddr_in*)&tmp->ai_addr)->sin_port = atoi(service);
	return 0; 
}

void freeMyAddrinfo(struct addrinfo *addr) {

}

data_packet_t* q_pkt_maker(const char* node, const char* service, int* randnum) {
	
	data_packet_t* tmp = (data_packet_t*) malloc(sizeof(data_packet_t));
	tmp->header = (header_t*) malloc(sizeof(header_t));
	tmp->query = (query_t*) malloc(sizeof(query_t));
	tmp->query.qname = (unsigned char*)malloc(sizeof(unsigned char) * strlen(QUERY)+1);
	tmp->query.question = (question_t*) malloc(sizeof(question_t));
	tmp->response = NULL;

	header_t* header = tmp->header;
	query_t* query = tmp->query;
	question_t* question = query->question;
	unsigned char* name = query->qname;

	// generate header
	srand(time(NULL));
	header->ID = (uint16_t)rand();
	*randnum = header->ID;
	header->FLAG = 0; // 0 0000 0 0 0 0 000 0000
	header->QDCOUNT = 1;
	header->ANCOUNT = 0;
	header->NSCOUNT = 0;
	header->ARCOUNT = 0;

	// generate data 
	convertName(name,QUERY);

	// set up data attribute
	question->qtype = 1
	question->qclass = 1
	
	return tmp;
}

int parse_res(data_packet_t* pkt, struct addrinfo* tmp, int random) {
	char* ip;
	// get the last 4 bytes as ip
	ip = (char*) pkt + 4*4+32*2;
	((struct sockaddr_in*)&tmp->ai_addr)->sin_addr.s_addr = htonl(ip);
	return 0;
}
  
void convertName(unsigned char* name, unsigned char* src) {
	int dot = 0;
	int i;
	int length = strlen(src);
	for(i=1; i < length; i++) {
		if (src[i] == '.') {
			name[dot] = (char)(i - dot -1);
			dot = i;
		} else {
			name[i] = src[i];
		}
	}
	name[strlen(src)] = '\0';
}

void pkt2buf(unsigned char* buf, data_packet_t* pkt) {
	int index  = 0, length = 0;
	
	length = sizeof(header_t);
	memcpy(buf+index,pkt->header,length);
	
	index += length;
	length = strlen(pkt->query->qname)+1;
	memcpy(buf+index,pkt->query->qname,length);

	index += length;
	length = sizeof(quesiton_t);
	memcpy(buf+index,pkt->query->question,length);
}