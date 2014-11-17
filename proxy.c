/*
 * proxy.c - A proxy server for CMU 15-213 proxy lab
 * Author: Ming Fang
 * Email:  mingf@andrew.cmu.edu
 */

#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <sys/time.h>
#include "pool.h"
#include "io.h"
#include "media.h"




/* You won't lose style points for including these long lines in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept_hdr = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding_hdr = "Accept-Encoding: gzip,deflate,sdch\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *pxy_connection_hdr = "Proxy-Connection: close\r\n\r\n";


/* Function prototype */
void proxy(pool_t *, int);
int parse_uri(char *uri, char *host, int *port, char *path);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);
void read_requesthdrs(int);
void serve_clients(pool_t*);
void usage();


int main(int argc, char **argv) {
    int listen_sock, client_sock;
    socklen_t cli_size;
    struct sockaddr cli_addr;
    sigset_t mask, old_mask;
    
    int lis_port = 0; /* The port for the HTTP server to listen on */
    unsigned int dns_port = 0;
    char *log_file = NULL; /* File to send log messages to (debug, info, error) */
    float alpha = 0;
    char *fake_ip = NULL;
    char *dns_ip = NULL;
    char *www_ip = NULL;

    /* all activate connection pool */
    pool_t pool;

    if (argc != 7 && argc != 8) {
        DPRINTF("%d",argc);
        usage();
        exit(EXIT_FAILURE);
    }

    
    
     /* Parse arguments */
    log_file = argv[1];
    pool.alpha = atof(argv[2]);
    lis_port = atoi(argv[3]);
    fake_ip = argv[4];
    dns_ip = argv[5];
    dns_port = atoi(argv[6]);
    if (argc == 8) {
        www_ip = argv[7];
    }

    

    /* deal with the case of writing into broken pipe */
    sigemptyset(&mask);
    sigemptyset(&old_mask);
    sigaddset(&mask, SIGPIPE);
    sigprocmask(SIG_BLOCK, &mask, &old_mask);

    init_mydns(dns_ip, dns_port);


    listen_sock = open_listen_socket(lis_port);
    if (listen_sock < 0) {
        DPRINTF("open_listen_socket: error!");
        exit(EXIT_FAILURE);
    }
    init_pool(listen_sock, &pool);
    /*
    pool.serv_sock = open_server_socket(fake_ip, www_ip);
    if (pool.serv_sock < 0) {
        DPRINTF("open_server_socket: error!\n");
        exit(EXIT_FAILURE);
    }*/

    

    

    memset(&cli_addr, 0, sizeof(struct sockaddr));
    memset(&cli_size, 0, sizeof(socklen_t));
    DPRINTF("----- Proxy Start -----\n");
    
     while (1) {
        pool.ready_read = pool.read_set;
        pool.ready_write = pool.write_set;
        
        
        DPRINTF("New select\n");
        
        pool.nready = select(pool.maxfd + 1, &pool.ready_read,
                             &pool.ready_write, NULL, NULL);
        
        
        DPRINTF("nready = %d\n", pool.nready);

        if (pool.nready == -1) {
            /* Something wrong with select */
            
            DPRINTF("Select error on %s\n", strerror(errno));
            clean_state(&pool, listen_sock);
        }
        if (FD_ISSET(listen_sock, &pool.ready_read) &&
            pool.cur_conn <= FD_SETSIZE) {
        
            if ((client_sock = accept(listen_sock, 
                                      (struct sockaddr *) &cli_addr,
                                      &cli_size)) == -1) {
                close(listen_sock);
                DPRINTF("Error accepting connection.\n");
                continue;
            }
            
            DPRINTF("New client %d accepted\n", client_sock);
            fcntl(client_sock, F_SETFL, O_NONBLOCK);
            add_client(client_sock, &pool);
        }
        serve_clients(&pool);
    }
    close_socket(listen_sock);
    return EXIT_SUCCESS;
}


void serve_clients(pool_t* p) {
    int i;
    conn_t *conni;
    int conn_sock = -1;
    int serv_sock = p->serv_sock;

    for(i = 0; (i <= p->maxi) && (p->nready > 0); i++) {
        if (p->conn[i] == NULL)
            continue;
        conni = p->conn[i];
        conn_sock = conni->fd;
        if(FD_ISSET(conn_sock, &p->ready_read)) {
            proxy(p, i);
            //close_conn(p, i);
            p->nready--;
        }
    }
}

/*
 * proxy - handle one proxy request/response transaction
 */
/* $begin proxy */
void proxy(pool_t *p, int i) 
{
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char buf_internet[MAXLINE];
    char host[MAXLINE], path[MAXLINE];
    int port;
    size_t n;
    size_t sum = 0;
    conn_t *conni = p->conn[i];
    int fd = conni->fd;
    int serv_fd;
    struct timeval start;
  
    /* Read request line and headers */
    io_recvlineb(fd, buf, MAXLINE);

    DPRINTF("Request: %s\n", buf);

    if (strcmp(buf, "") == 0)
        return;

    sscanf(buf, "%s %s %s", method, uri, version);
    if (strcasecmp(method, "GET")) { 
        clienterror(fd, method, "501", "Not Implemented",
                "Ming does not implement this method");
        return;
    }

    read_requesthdrs(fd);

    /* Parse URI from GET request */
    if (!parse_uri(uri, host, &port, path)) {
		clienterror(fd, uri, "404", "Not found",
		              "Ming couldn't parse the request");
		return;
    }

    if (endsWith(path, ".f4m")) {
        //strcpy(path + strlen(path) - 4, "_nolist.f4m");
        
    }
    fprintf(stderr, "Curr Thruput = %u\n", conni->thruput); 
    
	if (VERBOSE) {
        printf("uri = \"%s\"\n", uri);
        printf("host = \"%s\", ", host);
        printf("port = \"%d\", ", port);
        printf("path = \"%s\"\n", path);
    }

    //exit(0);
    serv_fd = open_server_socket("1.0.0.1", "4.0.0.1");
	/* Forward request */
    sprintf(buf_internet, "GET %s HTTP/1.1\r\n", path);
    io_sendn(serv_fd, buf_internet, strlen(buf_internet));
	sprintf(buf_internet, "Host: %s\r\n", host);
    io_sendn(serv_fd, buf_internet, strlen(buf_internet));
    io_sendn(serv_fd, user_agent_hdr, strlen(user_agent_hdr));
    io_sendn(serv_fd, accept_hdr, strlen(accept_hdr));
    io_sendn(serv_fd, accept_encoding_hdr, strlen(accept_encoding_hdr));
    io_sendn(serv_fd, connection_hdr, strlen(connection_hdr));
    io_sendn(serv_fd, pxy_connection_hdr, strlen(pxy_connection_hdr));

	/* Forward respond */
    //exit(0);
    gettimeofday(&start, NULL);
    while ((n = io_recvn(serv_fd, buf_internet, MAXLINE)) > 0) {
        sum += n; /*
        if (VERBOSE) {
            fprintf(stderr, "This line %zu:\n", n);
            write(STDOUT_FILENO, buf_internet, n);
            fprintf(stderr, "\n");
        }*/
		io_sendn(fd, buf_internet, n);
	}
    update_thruput(sum, &start, p, i);

    close_socket(serv_fd);

    DPRINTF("Forward respond %zu bytes\n", sum);    
    DPRINTF("Proxy is exiting\n\n");
    //exit(0);
    FD_CLR(fd, &p->read_set);
}
/* $end proxyd */


/*
 * parse_uri - parse URI into filename and CGI args
 *             return 1 on good, 0 on error
 */
/* $begin parse_uri */
int parse_uri(char *uri, char *host, int *port, char *path) 
{
    const char *ptr;
    const char *tmp;
    char scheme[10];
    char port_str[10];
    int len;
    int i;

    ptr = uri;

    /* Read scheme */
    tmp = strchr(ptr, ':');
    if (NULL == tmp) 
    	return 0;   // Error.
    
    len = tmp - ptr;
    (void)strncpy(scheme, ptr, len);
    scheme[len] = '\0';
    for (i = 0; i < len; i++)
    	scheme[i] = tolower(scheme[i]);
    if (strcasecmp(scheme, "http"))
    	return 0;   // Error, only support http

    // Skip ':'
    tmp++;
    ptr = tmp;

    /* Read host */
    // Skip "//"
    for ( i = 0; i < 2; i++ ) {
        if ( '/' != *ptr ) {
            return 0;
        }
        ptr++;
    }

    tmp = ptr;
    while ('\0' != *tmp) {
    	if (':' == *tmp || '/' == *tmp)
    		break;
    	tmp++;
    }
    len = tmp - ptr;
    (void)strncpy(host, ptr, len);
    host[len] = '\0';

    ptr = tmp;

    // Is port specified?
    if (':' == *ptr) {
    	ptr++;
    	tmp = ptr;
    	/* Read port */
    	while ('\0' != *tmp && '/' != *tmp)
    		tmp++;
    	len = tmp - ptr;
    	(void)strncpy(port_str, ptr, len);
    	port_str[len] = '\0';
    	*port = atoi(port_str);
    	ptr = tmp;
    } else {
    	// port is not specified, use 80 since scheme is 'http' 
    	*port = 80;
    }

    /* If this is the end of url */
    if ('\0' == *ptr) {
    	strcpy(path, "/");
    	return 1;
    }

    tmp = ptr;
    while ('\0' != *tmp)
    	tmp++;
    len = tmp - ptr;
    (void)strncpy(path, ptr, len);
    path[len] = '\0';

    return 1;
}


/*
 * read_requesthdrs - read and parse HTTP request headers
 */
/* $begin read_requesthdrs */
void read_requesthdrs(int fd) {
    char buf[MAXLINE];

    io_recvlineb(fd, buf, MAXLINE);
    DPRINTF("%s", buf);
    while(strcmp(buf, "\r\n")) {
        io_recvlineb(fd, buf, MAXLINE);
        DPRINTF("%s", buf);
    }
    return;
}
/* $end read_requesthdrs */

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) {
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Proxy Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Ming proxy server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    io_sendn(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    io_sendn(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    io_sendn(fd, buf, strlen(buf));
    io_sendn(fd, body, strlen(body));
}
/* $end clienterror */


void usage() {
    fprintf(stderr, "usage: proxy <log> <alpha> <listen-port> <fake-ip> <dns-ip> <dns-port> [<www-ip>]\n");
    exit(0);
}