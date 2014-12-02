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

static const char* QUERY_HEX = "00000101011101100110100101100100011001010110111100000010011000110111001100000011011000110110110101110101000000110110010101100100011101010000000000000000000000010000000000000001";
static const char* RES_HEX = "00000101011101100110100101100100011001010110111100000010011000110111001100000011011000110110110101110101000000110110010101100100011101010000000000000000000000010000000000000001110000000000110000000000000000010000000000000001000000000000000000000000000000000000000000000100";
static const char* ERR_HEX = "00000101011101100110100101100100011001010110111100000010011000110111001100000011011000110110110101110101000000110110010101100100011101010000000000000000000000010000000000000001";

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
 *  ID - A random 16 bits number
 *  FLAG - A sequence of numbers indicate different pkts
 *  QDCOUNT - how many questions
 *  ANCOUNT - how many answers
 *  NSCOUNT - an unsigned 16 bit integer specifying the number 
 *			of name server resource records in the authority records section. 
 *  ARCOUNT - an unsigned 16 bit integer specifying the number 
 *			of resource records in the additional records section.
 */
typedef struct header_s {
    uint16_t ID;
    uint16_t FLAG;
    uint16_t QDCOUNT;
    uint16_t ANCOUNT;
    uint16_t NSCOUNT;
    uint16_t ARCOUNT;
} header_t; 

typedef struct r_attr_s {
    uint16_t TYPE;
    uint16_t CLASS;
    uint16_t TTL;
    uint16_t RDLENGTH;
} r_attr_t;

typedef struct response_s {
    unsigned char* NAME;
    struct r_attr_t ATTR;
    unsigned char* DATA;
} response_t;

typedef struct question_s {
    uint16_t QTYPE;
    uint16_t QCLASS;
} question_t;

typedef struct query_s {
    unsigned char* QNAME;
} query_t;
/**
 *  The struct for DNS packet.
 *  header - The header of dns pkt
 *  data - The content of dns pkt
 */
typedef struct data_packet {
    header_t header;
    char question[DATALEN];
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



/**
 * converts the binary char string str to ascii format. the length of 
 * ascii should be 2 times that of str
 */
void binary2hex(uint8_t *buf, int len, char *hex);

/**
 *Ascii to hex conversion routine
 */
static uint8_t _hex2binary(char hex);

/**
 * converts the ascii character string in "ascii" to binary string in "buf"
 * the length of buf should be atleast len / 2
 */
void hex2binary(char *hex, int len, uint8_t*buf);

#endif