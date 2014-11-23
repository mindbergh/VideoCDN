#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include "io.h"

int main() {

	int serverfd;
	struct sockaddr_in fake_addr;
	struct sockaddr_in serv_addr;
	int rc;
	const char *fake_ip = "1.0.0.1";
	const char *www_ip = "4.0.0.1";
	const char *test = "GET / HTTP/1.1\r\n\r\n";
	/* Create the socket descriptor */
	if ((serverfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		return -1;
	}


	memset(&fake_addr, '0', sizeof(fake_addr)); 
	fake_addr.sin_family = AF_INET;
	inet_pton(AF_INET, fake_ip, &(fake_addr.sin_addr));
	fake_addr.sin_port = htons(0);  // let system assgin one 
	rc = bind(serverfd, (struct sockaddr *)&fake_addr, sizeof(fake_addr));
	if (rc < 0) {
		fprintf(stderr,"Bind server sockt error!");
		return -1;
	}

	// server ip is specified
	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET; 
	inet_pton(AF_INET, www_ip, &(serv_addr.sin_addr));
	serv_addr.sin_port = htons(8080);
	rc = connect(serverfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if (rc < 0) {
		// handle error
		fprintf(stderr,"Connect error!\n");
		return -1;
	}
	/* Clean up */
	
	io_sendn(serverfd, test, strlen(test));
   	
	int n = 0;
	char buf[8196];
	n = io_recvn(serverfd, buf, 8196);
	fprintf(stderr, "%s", buf);
	return 0;
}
