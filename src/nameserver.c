#include "nameserver.h"
#include "mydns.h"
#include "pool.h"

int main(int argc, char* argv[]) {
	
	if (argc < 7) {
		usage();
	}
	char* ip = argv[3];
	int port = atoi(argv[4]);
	char* serv_file = argv[5];
	char* lsa = argv[6];
	int fd;
	fd_set read_set;
	fd_set ready_read;
	int nready = 0;

	if ((fd = init_udp(ip,port,&read_set) == -1) {
		DPRINTF("fail to initialize UDP!\n");
		exit(-1);
	}
	
	while(1) {
		ready_read = read_set;
		nready = select(fd+1,ready_read,NULL,NULL,NULL);
		
		if (nready == -1) {
			DPRINTF("Select error on %s\n", strerror(errno));
            close(fd);
            exit(-1);
		}

		if (nready == 1)
			serve(fd);
		
		FD_CLR(fd,&read_set);
		FD_SET(fd,&read_set);
	}

}

int init_udp(char* ip, int port, fd_set* read_set) {
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
  FD_SET(sock,read_set);
  return sock;
}

void serve(int fd) {
	char* buf[BUF_SIZE];
	struct sockaddr_in from;
    int res = 0;
    uint16_t ID;
    data_packet_t* pkt;
    socklen_t fromlen = sizeof(from);

	if((res = recvfrom(fd,buf,
		BUFSIZE,0, (struct sockaddr *) &from,
                            fromlen)) != -1 ) {
		if ((parse((data_packet_t*) buf)) == -1) {
			// invalid query
			pkt = gen_err((data_packet_t*)buf);
			sendto(fd, pkt, strlen(pkt), 0, (struct sockaddr *)&from, fromlen);
		} else {
			// generate response
			pkt = gen_res((data_packet_t*)buf, (struct sockaddr *) &from);
			// send response back to client
			sendto(fd, pkt, strlen(pkt), 0, (struct sockaddr *)&from, fromlen);
		}
	} else {
		// read error from udp
		DPRINTF("read error from UDP!\n");
		exit(-1);
	}
}

data_packet_t* gen_err(data_packet_t* origin) {
	data_packet_t* pkt = (data_packet_t*)malloc(struct data_packet_t);
	header_t* header = pkt->header;
	char* data = pkt->data;

	// generate header
	header->ID = origin->ID;
	header->FALG = 67590;
	header->QDCOUNT = 1;
	header->ANCOUNT = 0;
	header->NSCOUNT = 0;
	header->ARCOUNT = 0;

	// generate data 
	strcpy(data,origin->data);
	return pkt;
}


int parse(data_packet_t* pkt) {
	header_t* header = pkt->header;
	char* data = pkt->data;
	char buf[DATALEN*2] = {0};
	// check headers
	if(header->FLAG != 0) {
		DPRINTF("wrong flag!\n");
		return -1;
	}
	if(header->QDCOUNT != 1) {
		DPRINTF("wrong flag!\n");
		return -1;
	}
	if(header->ANCOUNT != 0) {
		DPRINTF("wrong flag!\n");
		return -1;
	}
	if(header->NSCOUNT != 0) {
		DPRINTF("wrong flag!\n");
		return -1;
	}
	if(header->ARCOUNT != 0) {
		DPRINTF("wrong flag!\n");
		return -1;
	}

	// check question
	binary2hex(data,DATALEN,buf);
	if(memcmp(buf,QUERY_HEX,strlen(QUERY_HEX)) != 0) {
		DPRINTF("wrong query!\n");
		return -1;
	}
}
void usage() {
    fprintf(stderr, "usage: ./nameserver [-r] <log> <ip> <port> <servers> <LSAs>\n");
    exit(0);
}