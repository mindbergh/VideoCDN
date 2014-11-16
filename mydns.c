#include "mydns.h"


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
	/* struct addrinfo *tmp;
	*res = malloc(sizeof(struct addrinfo));
	tmp = *res;
	memset(tmp, 0, sizeof(struct addrinfo)); // make sure the struct is empty
	tmp->ai_flags = AI_PASSIVE;
	tmp->ai_family = AF_INET;
	tmp->ai_socktype = SOCK_STREAM;
	tmp->ai_protocol = 0;
	tmp->ai_addrlen = sizeof(ai_addr);
	tmp->ai_addr = malloc(sizeof(sockaddr_in));
	tmp->ai_canonname = NULL;
	tmp->ai_next = NULL; */
	return -1; 
}

void freeMyAddrinfo(struct addrinfo *addr) {

}
