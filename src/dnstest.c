#include "mydns.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>




int main() {
	//data_packet_t* pkt = q_pkt_maker("video.cs.cmu.edu");
	//char buf[BUFSIZE];
	//pkt2buf(buf, pkt);
	int res;
	struct addrinfo *servinfo;
	char ipstr[INET_ADDRSTRLEN];

	res = init_mydns("127.0.0.1", 9999);

	if (res == -1) {
		fprintf(stderr, "Init dns error\n");
	}

	res = resolve("video.cs.cmu.edu", "8888", NULL, &servinfo);

	if (res == -1) {
		fprintf(stderr, "resolve error\n");
	}
 
	struct sockaddr_in *ipv4 = (struct sockaddr_in*)servinfo->ai_addr;

	inet_ntop(AF_INET, &(ipv4->sin_addr), ipstr, sizeof(ipstr));
	fprintf(stderr, "%s\n", ipstr);
	
	return 0;
}