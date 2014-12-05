#ifndef _MYDNS_H
#define _MYDNS_H

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

#define DATALEN 34 // The longest data length for this proj
#define BUFSIZE 8192

static const unsigned char* QUERY = ".video.cmu.edu";
static const unsigned char* RES = "00000101011101100110100101100100011001010110111100000010011000110111001100000011011000110110110101110101000000110110010101100100011101010000000000000000000000010000000000000001110000000000110000000000000000010000000000000001000000000000000000000000000000000000000000000100";
static const unsigned char* ERR = "00000101011101100110100101100100011001010110111100000010011000110111001100000011011000110110110101110101000000110110010101100100011101010000000000000000000000010000000000000001";

/**
 *  The struct for DNS service on client side.
 *  dns_sock - a socket used for UDP communication
 *  servaddr - the address of DNS server. 
 */
typedef struct dns_s{
	int sock;
	struct sockaddr_in servaddr;
}dns_t;


/**
 *  The struct for DNS packet header.
 *  ID     - A random 16 bits number
 *  QR     - 0 for query and 1 for response
 *  Opcode - 0 for standard query
 *  AA     - 0 for requests, 1 for response
 *  TC     - 0
 *  RD     - 0
 *  RA     - 0
 *  Z      - 0
 *  Rcode  - 0 for no error, 3 for name error
 *  QDCOUNT - how many questions
 *  ANCOUNT - how many answers
 *  NSCOUNT - an unsigned 16 bit integer specifying the number 
 *			of name server resource records in the authority records section. 
 *  ARCOUNT - an unsigned 16 bit integer specifying the number 
 *			of resource records in the additional records section.
 */
typedef struct header_s {
    uint16_t id;

    uint8_t rd :1;
    uint8_t tc :1;  
    uint8_t aa :1;  
    uint8_t opcode :4; 
    uint8_t qr :1;
    
    uint8_t rcode:4;
    uint8_t z :3;
    uint8_t ra :1;

    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} header_t;

typedef struct question_s {
    uint16_t qtype;
    uint16_t qclass;
} question_t;

typedef struct answer_s {
    uint16_t atype;
    uint16_t aclass;
    uint16_t attl;
    uint16_t ardlength;
} answer_t;

typedef struct dns_response_s {
    unsigned char* name;
    struct answer_t* answer;
    unsigned char* data;
} dns_response_t;

typedef struct query_s {
    unsigned char* qname;
    struct question_t* question;
} query_t;
/**
 *  The struct for DNS packet.
 *  header - The header of dns pkt
 *  data - The content of dns pkt
 */
typedef struct data_packet {
    header_t* header;
    query_t* query;
    dns_response_t* response;
} data_packet_t;


/**
 * Initialize your client DNS library with the IP address and port number of
 * your DNS server.
 *
 * @param  dns_ip  The IP address of the DNS server.
 * @param  dns_port  The port number of the DNS server.
 *
 * @return 0 on success, -1 otherwise
 */
int init_mydns(const char *dns_ip, unsigned int dns_port);


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
            const struct addrinfo *hints, struct addrinfo **res);

/**
 * Generate a DNS query. It is used to ask DNS server for a specific domain address
 *
 * @param  node  The hostname to resolve.
 * @param  service  The desired port number as a string.
 *
 * @return a query pkt if succeed, NULL if failed 
 */
data_packet_t* q_pkt_maker(const char* node, const char* service,int*);


/**
 * Parse the DNS response. Fill the address struct
 *
 * @param  pkt  The original response packet
 * @param  tmp	The address info struct which requires to be filled up
 *
 * @return 0 if the response is valid with correct result, -1 if query failed somehow
 */
int parse_res(data_packet_t* pkt, struct addrinfo* tmp, int);
void convertName(unsigned char* name, unsigned char* src);

#endif