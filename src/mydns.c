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
	header_t* header = &(tmp->header);
	char data[DATALEN];
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
	hex2binary(QUERY_HEX,strlen(QUERY_HEX),tmp->data);

	return tmp;
}

int parse_res(data_packet_t* pkt, struct addrinfo* tmp, int randnum) {
	char* ip;
	char hex[DATALEN*2];
	// check random number 
	if (pkt->header.ID != randnum) 
		return -1;
	// check FLAG
	if (pkt->header.FLAG != 33792)
		return -1;
	// check question num
	if (pkt->header.QDCOUNT != 1)
		return -1;
	// check answer num
	if (pkt->header.ANCOUNT != 1)
		return -1;
	// check NSCOUNT
	if (pkt->header.NSCOUNT != 0)
		return -1;
	// check ARCOUNT
	if (pkt->header.ARCOUNT != 0)
		return -1;
	// check question
	binary2hex((char*)pkt+16,strlen((char*)pkt+16),hex);
	if (memcmp(hex,RES_HEX,strlen(RES_HEX)) != 0) {
		DPRINTF("Response doesn't match!\n");
		return -1;
	}
	// get the last 4 bytes as ip
	ip = (char*) pkt + 4*4+32*2;
	((struct sockaddr_in*)&tmp->ai_addr)->sin_addr.s_addr = htonl(ip);
	return 0;
}


/**
 * converts the binary char string str to ascii format. the length of 
 * ascii should be 2 times that of str
 */
void binary2hex(uint8_t *buf, int len, char *hex) {
	int i=0;
	for(i=0;i<len;i++) {
		sprintf(hex+(i*2), "%.2x", buf[i]);
	}
	hex[len*2] = 0;
}
  
/**
 *Ascii to hex conversion routine
 */
static uint8_t _hex2binary(char hex)
{
     hex = toupper(hex);
     uint8_t c = ((hex <= '9') ? (hex - '0') : (hex - ('A' - 0x0A)));
     return c;
}

/**
 * converts the ascii character string in "ascii" to binary string in "buf"
 * the length of buf should be atleast len / 2
 */
void hex2binary(char *hex, int len, uint8_t*buf) {
	int i = 0;
	for(i=0;i<len;i+=2) {
		buf[i/2] = 	_hex2binary(hex[i]) << 4 
				| _hex2binary(hex[i+1]);
	}
}