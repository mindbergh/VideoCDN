#include "nameserver.h"
#include "mydns.h"
#include "pool.h"
#include "ospf.h"

static char ref_pkt[BUFSIZE];
static int ref_pkt_len;
static const char* ref_host = "video.cs.cmu.edu";
static FILE* flog;

static int buf_len(char* buf);
static void ns_log_init(char* log_file);
static void ns_log(char* clit_ip, char* host_name, char* res_ip);

int main(int argc, char* argv[]) {
	
	int fd;
	fd_set read_set;
	fd_set ready_read;
	int nready = 0;

	char* log_file;
	char* ip;
	int port;
	char* serv_file;
	char* lsa;

	int rr_flag = 0;

	if (argc == 7) {
		assert(strcmp(argv[1], "-r") == 0);
		rr_flag = 1;
		log_file = argv[2];
		ip = argv[3];
		port = atoi(argv[4]);
		serv_file = argv[5];
		lsa = argv[6];
	} else if (argc == 6) {
		log_file = argv[1];	
		ip = argv[2];
		port = atoi(argv[3]);
		serv_file = argv[4];
		lsa = argv[5];
	} else {
		usage();
		return(-1);
	}
	
	ns_log_init(log_file);


	if ((fd = init_udp(ip,port,&read_set)) == -1) {
		DPRINTF("fail to initialize UDP!\n");
		exit(-1);
	}

	OSPF_init(serv_file, lsa, rr_flag);
	
	while(1) {
		ready_read = read_set;
		nready = select(fd+1,&ready_read,NULL,NULL,NULL);
		
		if (nready == -1) {
			DPRINTF("Select error on %s\n", strerror(errno));
            close(fd);
            exit(-1);
		}

		if (nready == 1)
			serve(fd, rr_flag);
		
		FD_CLR(fd,&read_set);
		FD_SET(fd,&read_set);
	}

}

void init_ref() {
	data_packet_t* pkt = q_pkt_maker(ref_host);
	ref_pkt_len = pkt2buf(ref_pkt, pkt);
	free_pkt(pkt);
}

int init_udp(char* ip, int port, fd_set* read_set) {
  int sock;
  struct sockaddr_in myaddr;
  int yes = 1;
  if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) == -1) {
    DPRINTF("init_udp could not create socket");
    exit(-1);
  }
  
  // lose the pesky "address already in use" error message
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));


  bzero(&myaddr, sizeof(myaddr));
  myaddr.sin_family = AF_INET;
  inet_pton(AF_INET, ip, &(myaddr.sin_addr));
  //myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = htons(port);
  
  if (bind(sock, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
    DPRINTF("init_udp could not bind socket\n");
    exit(-1);
  }
  FD_SET(sock,read_set);
  return sock;
}

void serve(int fd, int rr_flag) {
	char req_buf[BUF_SIZE];
	char res_buf[BUF_SIZE];
	struct sockaddr_in from;
    int res = 0;
    int parse_ret = 0;
    int pkt_len;
  	char from_str[INET_ADDRSTRLEN];
 	char* res_ip_str;
    socklen_t fromlen = sizeof(from);

	if((res = recvfrom(fd, req_buf,
		BUFSIZE,0, (struct sockaddr *) &from, &fromlen)) != -1) {
		inet_ntop(AF_INET, &(from.sin_addr), from_str, INET_ADDRSTRLEN); // get the clent address;
		if ((parse_ret = parse(req_buf)) == -1) {
			// invalid query
			pkt_len = gen_err(req_buf);
			sendto(fd, req_buf, pkt_len, 0, (struct sockaddr *)&from, fromlen);
		} else {
			// generate response
			res_ip_str = route(from_str, rr_flag);
			ns_log(from_str, ref_host, res_ip_str);
			pkt_len = gen_res(req_buf, res_buf, res_ip_str);
			// send response back to client
			sendto(fd, res_buf, pkt_len, 0, (struct sockaddr *)&from, fromlen);
		}
	} else {
		// read error from udp
		DPRINTF("read error from UDP!\n");
		exit(-1);
	}
}

static int buf_len(char* buf) {
	int length = sizeof(header_t);

	char* qname = (char*)(buf + length);
	length += strlen(qname) + 1 + sizeof(question_t);
	return length;
}

int gen_err(char* origin) {
	int len = buf_len(origin);
	header_t* hdr = (header_t*)origin;
	hdr->rcode = htonl(3);

	return len;
}

int gen_res(char* req_buf, char* res_buf, char* dest_addr) {
	int len = buf_len(req_buf);
	memcpy(res_buf, req_buf, len);
	header_t* hdr = (header_t*)res_buf;
	hdr->qr = 1;
	hdr->aa = 1;
	hdr->ancount = htons(1);
    
    // get the qname
    char* qname = (char*)(res_buf + sizeof(header_t));

	//dns_response_t* resp = (dns_response_t*)(res_buf + len);
	memcpy(res_buf + len, qname, strlen(qname) + 1);
	len += (strlen(qname) + 1);
	
	//set answer_t
	answer_t* ans = (answer_t*)(res_buf + len);
	
    ans->atype = htons(1);
    ans->aclass = htons(1);
    ans->attl = 0;
    ans->ardlength = htons(4);   //ipv4

	len += (sizeof(answer_t)) - 2; // padding
    //set ip
    struct sockaddr_in sa;
    inet_pton(AF_INET, dest_addr, &(sa.sin_addr));
    *((uint32_t*)(res_buf+len)) = sa.sin_addr.s_addr;
    len += 4; // ipv4
    return len;
}

int parse(char* buf) {
	/*
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
	}*/
	int offset = sizeof(uint16_t);
	if (!memcmp(buf+offset, ref_pkt+offset, ref_pkt_len-offset)) {
		return -1;
	}
	return 0;
}


void usage() {
    fprintf(stderr, "usage: ./nameserver [-r] <log> <ip> <port> <servers> <LSAs>\n");
    exit(0);
}





static void ns_log_init(char* log_file) {
	flog = fopen(log_file, "a");
	return;
}

static void ns_log(char* clit_ip, char* host_name, char* res_ip) {
	time_t since_epoch;
	since_epoch = time(NULL);
	fprintf(flog, "%ld ", since_epoch);
	fprintf(flog, "%s ", clit_ip);
	fprintf(flog, "%s ", host_name);
	fprintf(flog, "%s\n", res_ip);
	fflush(flog);
	return;
}