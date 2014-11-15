/*
 * proxy.c - A proxy server for CMU 15-213 proxy lab
 * Author: Ming Fang
 * Email:  mingf@andrew.cmu.edu
 */

#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include "pool.h"
#include "mio.h"


#define BUF_SIZE 8192 /* Initial buff size */
#define MAX_SIZE_HEADER 8192 /* Max length of size info for the incomming msg */
#define ARG_NUMBER 2 /* The number of argument lisod takes*/
#define LISTENQ 1024 /* second argument to listen() */
#define VERBOSE 1 /* Whether to print out debug infomations */


/* You won't lose style points for including these long lines in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept_hdr = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding_hdr = "Accept-Encoding: gzip, deflate\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *pxy_connection_hdr = "Proxy-Connection: close\r\n\r\n";


/* Function prototype */
void echo(int connfd);
void *thread(void *vargp);
void proxy(int fd);
int parse_uri(char *uri, char *host, int *port, char *path);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);
void read_requesthdrs(mio_t *rp);
void usage();

void sigpipe_handler(int sig) {
    printf("SIGPIPE handled\n");
    return;
}

int main(int argc, char **argv) {
    int listen_sock, client_sock;
    socklen_t cli_size;
    struct sockaddr cli_addr;
    sigset_t mask, old_mask;
    
    int http_port; /* The port for the HTTP server to listen on */
    char *log_file; /* File to send log messages to (debug, info, error) */

    /* all activate connection pool */
    pool_t pool;

    if (argc != 7 || argc != 8) {
        usage();
    }

    port = atoi(argv[1]);
    
     /* Parse arguments */
    http_port = atoi(argv[1]);
    //log_file = argv[3];

    /* deal with the case of writing into broken pipe */
    sigemptyset(&mask);
    sigemptyset(&old_mask);
    sigaddset(&mask, SIGPIPE);
    sigprocmask(SIG_BLOCK, &mask, &old_mask);

    listen_sock = open_listen_socket(http_port);
    init_pool(listen_sock, &pool);
    memset(&cli_addr, 0, sizeof(struct sockaddr));
    memset(&cli_size, 0, sizeof(socklen_t));
    fprintf(stdout, "----- Proxy Start -----\n");
    
     while (1) {
        pool.ready_read = pool.read_set;
        pool.ready_write = pool.write_set;
        
        if (VERBOSE)
            printf("New select\n");
        
        pool.nready = select(pool.maxfd + 1, &pool.ready_read,
            &pool.ready_write, NULL, NULL);
        
        if (VERBOSE)
            printf("nready = %d\n", pool.nready);

        if (pool.nready == -1) {
            /* Something wrong with select */
            if (VERBOSE)
                printf("Select error on %s\n", strerror(errno));
            clean_state(&pool, listen_sock);
        }
        if (FD_ISSET(listen_sock, &pool.ready_read) &&
            pool.cur_conn <= FD_SETSIZE) {
        
            if ((client_sock = accept(listen_sock, 
                (struct sockaddr *) &cli_addr,
                &cli_size)) == -1) {
                close(listen_sock);
                fprintf(stderr, "Error accepting connection.\n");
                continue;
            }
            if (VERBOSE)
                printf("New client %d accepted\n", client_sock);
            fcntl(client_sock, F_SETFL, O_NONBLOCK);
            add_client(client_sock, &pool);
        }
        serve_clients(&pool);
    }
    close_socket(listen_sock);
    return EXIT_SUCCESS;
}


void serve_clients( pool_t* pool) {
    int i = 0;
    int max_ready = pool->nready;
    int* client_socket = pool->client_sock;

    for(i = 1; i <= pool->maxi && max_ready > 0; i++) {
        if(FD_ISSET(client_socket[i],pool->read_read)) {
            proxy(client_socket[i]);
            max_ready--;
        }
    }
}

/*
 * proxy - handle one proxy request/response transaction
 */
/* $begin proxy */
void proxy(int fd) 
{
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char buf_internet[MAXLINE], payload[MAX_OBJECT_SIZE];
    char host[MAXLINE], path[MAXLINE];
    int port, fd_internet;
    int found = 0;
    size_t n;
    size_t sum = 0;
    mio_t mio_user, mio_internet;
    cnode_t * node;


  
    /* Read request line and headers */
    Mio_readinitb(&mio_user, fd);
    Mio_readlineb(&mio_user, buf, MAXLINE);

    printf("Request: %s\n", buf);

    if (strcmp(buf, "") == 0)
        return;

    sscanf(buf, "%s %s %s", method, uri, version);
    if (strcasecmp(method, "GET")) { 
        clienterror(fd, method, "501", "Not Implemented",
                "Ming does not implement this method");
        return;
    }

    read_requesthdrs(&mio_user);

    /* Parse URI from GET request */
    if (!parse_uri(uri, host, &port, path)) {
		clienterror(fd, uri, "404", "Not found",
		    "Ming couldn't parse the request");
		return;
    }

    printf("uri = \"%s\"\n", uri);
	if (VERBOSE) {

        printf("host = \"%s\", ", host);
        printf("port = \"%d\", ", port);
        printf("path = \"%s\"\n", path);
    }

    if (CACHE_ENABLE) {
        /* Critcal readcnt section begin */
        P(&mutex);
        readcnt++;
        if (readcnt == 1)  // First in
            P(&w);
        V(&mutex);
        /* Critcal readcnt section end */

        /* Critcal reading section begin */
        Cache_check();
        if ((node = match(host, port, path)) != NULL) {
            printf("Cache hit!\n");
            delete(node);
            enqueue(node);
            Mio_writen(fd, node->payload, node->size);
            printf("Senting respond %u bytes from cache\n",
                   (unsigned int)node->size);
            //fprintf(stdout, node->payload);
            found = 1;
        }        
        /* Critcal reading section end */  

        /* Critcal readcnt section begin */    
        P(&mutex);
        readcnt--;
        if (readcnt == 0)
            V(&w);
        V(&mutex);
        /* Critcal readcnt section end */

        if (found == 1) {
            printf("Proxy is exiting\n\n");
            return;
        }

        printf("Cache miss!\n");
    }

    fd_internet = Open_clientfd_r(host, port);
	Mio_readinitb(&mio_internet, fd_internet);

	/* Forward request */
    sprintf(buf_internet, "GET %s HTTP/1.0\r\n", path);
    Mio_writen(fd_internet, buf_internet, strlen(buf_internet));
	sprintf(buf_internet, "Host: %s\r\n", host);
    Mio_writen(fd_internet, buf_internet, strlen(buf_internet));
    Mio_writen(fd_internet, user_agent_hdr, strlen(user_agent_hdr));
    Mio_writen(fd_internet, accept_hdr, strlen(accept_hdr));
    Mio_writen(fd_internet, accept_encoding_hdr, strlen(accept_encoding_hdr));
    Mio_writen(fd_internet, connection_hdr, strlen(connection_hdr));
    Mio_writen(fd_internet, pxy_connection_hdr, strlen(pxy_connection_hdr));

	/* Forward respond */

    strcpy(payload, "");
    while ((n = Mio_readlineb(&mio_internet, buf_internet, MAXLINE)) != 0) {

        //printf("Fd = %d, Sum = %d, n = %d\n", mio_internet.mio_fd, sum, n);
        sum += n;
        if (sum <= MAX_OBJECT_SIZE)
            strcat(payload, buf_internet);
		Mio_writen(fd, buf_internet, n);
	}

    printf("Forward respond %d bytes\n", sum);


    if (CACHE_ENABLE) {
        if (sum <= MAX_OBJECT_SIZE) {
            node = new(host, port, path, payload, sum);

            /* Critcal write section begin */
            P(&w);
            Cache_check();            
            while (cache_load + sum > MAX_CACHE_SIZE) {
                printf("!!!!!!!!!!!!!!!!!Cache evicted!!!!!!!!!!!!!!!!!!\n");
                dequeue();
            }
            enqueue(node);
            printf("The object has been cached\n");
            printf("Current cache size is %d \n", cache_count);            
            printf("Current cache load is %d bytes\n", cache_load);
            //fprintf(stdout, payload);
            Cache_check();
            V(&w);
            /* Critcal write section end */
        }
    }
    
    printf("Proxy is exiting\n\n");
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
/* $end parse_uri */

/*
 * read_requesthdrs - read and parse HTTP request headers
 */
/* $begin read_requesthdrs */
void read_requesthdrs(mio_t *rp) {
    char buf[MAXLINE];

    Mio_readlineb(rp, buf, MAXLINE);
    if (VERBOSE)
        printf("%s", buf);
    while(strcmp(buf, "\r\n")) {
        Mio_readlineb(rp, buf, MAXLINE);
        if (VERBOSE)
            printf("%s", buf);
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
    Mio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Mio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Mio_writen(fd, buf, strlen(buf));
    Mio_writen(fd, body, strlen(body));
}
/* $end clienterror */


void usage() {
    fprintf(stderr, "usage: %s <log> <alpha> <listen-port> <fake-ip> <dns-ip> <dns-port> [<www-ip>]\n", argv[0]);
    exit(0);
}