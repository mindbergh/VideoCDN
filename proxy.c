/*
 * proxy.c - A proxy server for CMU 15-213 proxy lab
 * Author: Ming Fang
 * Email:  mingf@andrew.cmu.edu
 */

#include <stdio.h>
#include "csapp.h"
#include "cache.h"
#include "mio.h"

#define CACHE_ENABLE   1
#define VERBOSE        0

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

void sigpipe_handler(int sig) {
    printf("SIGPIPE handled\n");
    return;
}

int main(int argc, char **argv) {
    int listenfd, *connfdp, port;
    socklen_t clientlen = sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    pthread_t tid; 

    if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
    }
    port = atoi(argv[1]);


    Signal(SIGPIPE, sigpipe_handler);
    cache_init();

    listenfd = Open_listenfd(port);
    while (1) {
		connfdp = Malloc(sizeof(int));
		*connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen);
		Pthread_create(&tid, NULL, thread, connfdp);
    }

    return 0;
}


/* thread routine */
void *thread(void *vargp) 
{  
    int connfd = *((int *)vargp);
    printf("New thread created! fd = %d >>>>>>>>>>>>>>>>>>\n", connfd);
    Pthread_detach(pthread_self()); 
    Free(vargp);
    Signal(SIGPIPE, sigpipe_handler);
    proxy(connfd);
    Close(connfd);
    printf("Thread is closing! fd = %d <<<<<<<<<<<<<<<<<<<<<<<<\n", connfd);
    return NULL;
}
/* $end echoservertmain */

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