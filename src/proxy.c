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
#include "conn.h"
#include "timer.h"
#include "log.h"
#include "parse_xml.h"


#define FLAG_VIDEO    0
#define FLAG_LIST     1

#define TYPE_XML      1
#define TYPE_F4F      2
#define TYPE_MSC      0

/* global variable */
pool_t pool; 
bit_t* bitrates;


/* You won't lose style points for including these long lines in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept_hdr = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding_hdr = "Accept-Encoding: gzip,deflate,sdch\r\n";
static const char *connection_hdr = "Connection: Keep-Alive\r\n";
static const char *pxy_connection_hdr = "Proxy-Connection: Keep-Alive\r\n\r\n";


/* Function prototype */
int parse_uri(char *uri, char *host, int *port, char *path);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);
int read_requesthdrs(int clit_fd, char *host, int* port);
void read_responeshdrs(int serv_fd, int clit_fd, response_t* res);
void serve_clients();
void serve_servers();
void client2server(int);
void server2client(int);
bit_t* process_list(int serv_fd, int length); 
void usage();


int main(int argc, char **argv) {
    int listen_sock, client_sock;
    socklen_t cli_size;
    struct sockaddr_in cli_addr;
    sigset_t mask, old_mask;
    
    int lis_port = 0; /* The port for the HTTP server to listen on */
    unsigned int dns_port = 0;
    char *log_file = NULL; /* File to send log messages to (debug, info, error) */
    char *dns_ip = NULL;

    if (argc != 7 && argc != 8) {
        DPRINTF("%d",argc);
        usage();
        exit(EXIT_FAILURE);
    }

    
    
     /* Parse arguments */
    log_file = argv[1];
    pool.alpha = atof(argv[2]);
    lis_port = atoi(argv[3]);
    pool.fake_ip = argv[4];
    dns_ip = argv[5];
    dns_port = atoi(argv[6]);
    if (argc == 8) {
        pool.www_ip = argv[7];
    }

    

    /* deal with the case of writing into broken pipe */
    sigemptyset(&mask);
    sigemptyset(&old_mask);
    sigaddset(&mask, SIGPIPE);
    sigprocmask(SIG_BLOCK, &mask, &old_mask);

    init_mydns(dns_ip, dns_port); // currently does nothing
    init_serv_list();


    listen_sock = open_listen_socket(lis_port);
    if (listen_sock < 0) {
        DPRINTF("open_listen_socket: error!");
        exit(EXIT_FAILURE);
    }
    init_pool(listen_sock, &pool, argv);

    memset(&cli_addr, 0, sizeof(struct sockaddr));
    memset(&cli_size, 0, sizeof(socklen_t));
    DPRINTF("----- Proxy Start -----\n");
    
     while (1) {
        pool.ready_read = pool.read_set;
        pool.ready_write = pool.write_set;
        
        
        //DPRINTF("New select\n");
        
        pool.nready = select(pool.maxfd + 1, &pool.ready_read,
                             &pool.ready_write, NULL, NULL);
        
        
        //DPRINTF("nready = %d\n", pool.nready);

        if (pool.nready == -1) {
            /* Something wrong with select */
            
            DPRINTF("Select error on %s\n", strerror(errno));
            clean_state(&pool, listen_sock);
        }
        if (FD_ISSET(listen_sock, &pool.ready_read) &&
            pool.cur_conn <= FD_SETSIZE) {
            pool.nready--;
            if ((client_sock = accept(listen_sock, 
                                      (struct sockaddr *) &cli_addr,
                                      &cli_size)) == -1) {
                close_socket(listen_sock);
                DPRINTF("Error accepting connection.\n");
                continue;
            }
            
            DPRINTF("New client %d accepted\n", client_sock);
            int nonblock_flags = fcntl(client_sock,F_GETFL,0);
            fcntl(client_sock, F_SETFL,nonblock_flags|O_NONBLOCK);
            add_client(client_sock, cli_addr.sin_addr.s_addr);
        }
        if(pool.nready>0) {
            DPRINTF("About to serve client\n");
            serve_clients();
        }
        if(pool.nready>0) {
            DPRINTF("About to serve server\n");
            serve_servers();
        }
    }
    
    close_socket(listen_sock);
    return EXIT_SUCCESS;
}

void serve_servers() {
    int i;
    server_t *server;
    server_t** server_l = pool.server_l;
    for (i = 0; (i <= pool.max_serv_idx) && (pool.nready > 0); i++) {
        if (server_l[i] == NULL)
            continue;
        server = server_l[i];
        if(FD_ISSET(server->fd, &(pool.ready_read))) {
            server2client(i);
            pool.nready--;
        }
    } 
}


void serve_clients() {
    int i;
    client_t *client;
    client_t** client_l = pool.client_l;

    for (i = 0; (i <= pool.max_clit_idx) && (pool.nready > 0); i++) {
        if (client_l[i] == NULL)
            continue;

        client = client_l[i];
        if(FD_ISSET(client->fd, &(pool.ready_read))) {
            DPRINTF("Client fd = %d, index = %d\n", client->fd, i);
            client2server(i);
            pool.nready--;
        }
    } 
}

/*
 * proxy - handle one proxy request/response transaction
 */
/* $begin proxy */
void client2server(int clit_idx) 
{
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char buf_internet[MAXLINE];
    char host[MAXLINE], path[MAXLINE], path_list[MAXLINE];
    int port;
    int flag;
    size_t n;
    size_t sum;
    
    server_t* server = NULL;
    client_t* client = GET_CLIT_BY_IDX(clit_idx);
    conn_t *conn;
    int conn_idx, serv_idx;
    
    int fd = client->fd;
    int serv_fd;
    
    struct timeval start;
    struct addrinfo *servinfo;
    int lagv = -1;
    int client_close = 0;
    int new_thruput;
    serv_list_t *serv_info;
    struct sockaddr_in sa;


    /* Read request line and headers */

    io_recvline_block(fd, buf, MAXLINE);

    DPRINTF("Request: %s\n", buf);

    if (strcmp(buf, "") == 0) {
        DPRINTF("Empty buffer\n");
        return;
    }

    sscanf(buf, "%s %s %s", method, uri, version);
    if (strcasecmp(method, "GET")) { 
        DPRINTF("501 Not Implemented\n");
        clienterror(fd, method, "501", "Not Implemented",
               "Ming does not implement this method");
        return;
    }
    

    client_close = read_requesthdrs(fd, host, &port);

    /* Parse URI from GET request */
    if (!parse_uri(uri, host, &port, path)) {
        DPRINTF("404 Not found\n");
		clienterror(fd, uri, "404", "Not found",
		              "Ming couldn't parse the request");
		return;
    }
    DPRINTF("www_ip:%s\n",pool.www_ip);
    if (pool.www_ip) {
        inet_pton(AF_INET, pool.www_ip, &(sa.sin_addr));
        DPRINTF("about to get conn\n");
        if((conn_idx = client_get_conn(fd, sa.sin_addr.s_addr)) == -1) {
            serv_fd = open_server_socket(pool.fake_ip,pool.www_ip,port);
            serv_idx = add_server(serv_fd,sa.sin_addr.s_addr);
            DPRINTF("new server:%d add!\n",serv_fd);
            conn_idx = add_conn(clit_idx, serv_idx);
            DPRINTF("new connection:%d add!\n",conn_idx);
        }
    } else {
        resolve(host, port, NULL, &servinfo);
    }
    conn = GET_CONN_BY_IDX(conn_idx);
    if (client_close == -1) {
        close_conn(conn_idx);
        return;
    }
    server = GET_SERV_BY_IDX(conn->serv_idx);
    serv_fd = server->fd;

    
    if (endsWith(path, ".f4m") && !endsWith(path, "_nolist.f4m")) {
        flag = FLAG_LIST;
        strcpy(path_list, path);
        strcpy(path + strlen(path) - 4, "_nolist.f4m");
    } else if (isVideo(path)) {
        DPRINTF("This is video req: Idx:%d ;; Curr thru: %d\n",conn_idx, conn->avg_put);
        DPRINTF("Path=%s\n", path);
        bit_t *b = bitrates;
        int chosen_rate = 0;
        int thru = conn->avg_put / 1.5;
        int smallest = 100;
        while (b) {
            if (b->bitrate > chosen_rate && b->bitrate <= thru) {
                chosen_rate = b->bitrate;
            }
            if (b->bitrate < smallest) {
                smallest = b->bitrate;
            }
            //printf("bitrates: %d\n", b->bitrate);
            b = b->next;
        }
        if (chosen_rate <= 0) {
            chosen_rate = smallest;
        }
        conn->cur_bitrate = chosen_rate;
        modi_path(path, chosen_rate);
    }
    
    
	
    DPRINTF("uri = \"%s\"\n", uri);
    DPRINTF("host = \"%s\", ", host);
    DPRINTF("port = \"%d\", ", port);
    DPRINTF("path = \"%s\"\n", path);

    
    /* loggin  last finished file*/
    if(conn->start == NULL) { 
        conn->start = (struct timeval*)malloc(sizeof(struct timeval));
        gettimeofday(conn->start,NULL);
    } else {
        loggin(conn);
    }
    DPRINTF("log finished\n");
    /* update current requested file */
    strcpy(conn->cur_file,path);
    conn->cur_file[strlen(conn->cur_file)] = '\0';
    conn->cur_size = 0;
    gettimeofday(conn->start,NULL);

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

    
    if (flag != FLAG_LIST) {
        DPRINTF("This is no a f4m request, done\n");

        return;  // done with this, wait for nofication from select 
        //send pipelined request to serv
    }

    if (flag == FLAG_LIST) {
        DPRINTF("This is a f4m req, req for nolist\n");
        sprintf(buf_internet, "GET %s HTTP/1.1\r\n", path_list);
        io_sendn(serv_fd, buf_internet, strlen(buf_internet));
        sprintf(buf_internet, "Host: %s\r\n", host);
        io_sendn(serv_fd, buf_internet, strlen(buf_internet));
        io_sendn(serv_fd, user_agent_hdr, strlen(user_agent_hdr));
        io_sendn(serv_fd, accept_hdr, strlen(accept_hdr));
        io_sendn(serv_fd, accept_encoding_hdr, strlen(accept_encoding_hdr));
        io_sendn(serv_fd, connection_hdr, strlen(connection_hdr));
        io_sendn(serv_fd, pxy_connection_hdr, strlen(pxy_connection_hdr));
    }
}

void server2client(int serv_idx) {
    server_t* server;
    client_t* client;
    conn_t* conn;
    struct timeval start;
    //struct timeval end;
    int server_fd;
    int client_fd;
    int conn_idx;

    int n;
    int sum;
    char* buf_internet;

    response_t res;
    bit_t* this_bitrates;
    


    server = GET_SERV_BY_IDX(serv_idx);
    server_fd = server->fd;
    /* get connection */
    if ((conn_idx = server_get_conn(server_fd)) == -1) {
        DPRINTF("Cannot find connection from server to client! Error\n");
        close_conn(conn_idx);
        return;
        //exit(-1);
    }
    conn = GET_CONN_BY_IDX(conn_idx);
    client = GET_CLIT_BY_IDX(conn->clit_idx);
    client_fd = client->fd;

    read_responeshdrs(server_fd, client_fd, &res);
    if (res.length == 0) {
        close_conn(conn_idx);
        return;
    }
    buf_internet = (char *)malloc(res.length + 1);
    
    if (res.type == TYPE_XML) {
        n = io_recvn_block(server_fd, buf_internet, res.length);
        if (n != res.length) {
            DPRINTF("Unsuccessfully recv XML from server:%d, n = %d, length should be %d\n", server_fd, n, res.length);
            close_conn(conn_idx);
            return;
            //exit(EXIT_FAILURE);
        }
        DPRINTF("Successfully recv XML from server:%d, n = %d\n", server_fd, n);
        buf_internet[res.length] = '\0';

        this_bitrates = parse_xml(buf_internet, res.length);

        if (this_bitrates != NULL) {
            DPRINTF("This is a listed XML\n");
            bitrates = this_bitrates;
            //free(buf_internet);
            //buf_internet = realloc(buf_internet, MAXLINE);
            free(res.hdr_buf);
            res.hdr_buf = NULL;
            //DPRINTF("About to requset for nolist f4m, with cur_file_name:%s\n", conn->cur_file_name);
            /*
            int path_len = strlen(conn->cur_file_name);
            char *path_nolist = (char*)malloc(path_len + 8);
            strcpy(path_nolist, conn->cur_file_name);
            strcpy(path_nolist + path_len - 4, "_nolist.f4m");
            sprintf(buf_internet, "GET %s HTTP/1.1\r\n", path_nolist);
            io_sendn(serv_fd, buf_internet, strlen(buf_internet));
            sprintf(buf_internet, "Host: %s\r\n", host);
            io_sendn(serv_fd, buf_internet, strlen(buf_internet));
            io_sendn(serv_fd, user_agent_hdr, strlen(user_agent_hdr));
            io_sendn(serv_fd, accept_hdr, strlen(accept_hdr));
            io_sendn(serv_fd, accept_encoding_hdr, strlen(accept_encoding_hdr));
            io_sendn(serv_fd, connection_hdr, strlen(connection_hdr));
            io_sendn(serv_fd, pxy_connection_hdr, strlen(pxy_connection_hdr)); */
            free(buf_internet);
            return;
        }
        DPRINTF("This is a non-listed XML\n");
        n = io_sendn(client_fd, res.hdr_buf, res.hdr_len);  
        if (n != res.hdr_len) {
            DPRINTF("Unsuccessfully forward hdr:%d, n = %d, length should be %d\n", client_fd, n, res.hdr_len);
            close_conn(conn_idx);
            return;
            //exit(EXIT_FAILURE);
        }
        n = io_sendn(client_fd, buf_internet, res.length);  
        if (n != res.length) {
            DPRINTF("Unsuccessfully forward nolist XML:%d, n = %d, length should be %d\n", client_fd, n, res.hdr_len);
            close_conn(conn_idx);
            return;
            //exit(EXIT_FAILURE);
        }
        return;
    }
    /* This is not a XML */
    n = io_sendn(client_fd, res.hdr_buf, res.hdr_len);  
    if (n != res.hdr_len) {
        DPRINTF("Unsuccessfully forward hdr:%d, n = %d, length should be %d\n", client_fd, n, res.hdr_len);
        close_conn(conn_idx);
        return;
        //exit(EXIT_FAILURE);
    }
    /* Forward respond */
    gettimeofday(&start, NULL);
    n = io_recvn_block(server_fd, buf_internet, res.length);
    /*if (n != res.length) {
        DPRINTF("Unsuccessfully recv body from server:%d, n = %d, length should be %d\n", server_fd, n, res.length);
        exit(EXIT_FAILURE);
    }*/

    DPRINTF("Successfully recv body from server:%d, n = %d,len = %d\n", server_fd, n, res.length);
    if (n == 0) {
        close_conn(conn_idx);
        return;
    }
    if (io_sendn(client_fd, buf_internet, n) == -1) {
        close_conn(conn_idx);
        return;
    }
    
    gettimeofday(&(conn->end), NULL);  /* update conn end time */
    if (res.type == TYPE_F4F)
        update_thruput(n, conn); 
        
    free(buf_internet);
    free(res.hdr_buf);
    res.hdr_buf = NULL;
    DPRINTF("finish transimit content!\n");
    DPRINTF("Forward respond %d bytes\n", n);
}




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
    DPRINTF("About to parse URI\n");
    DPRINTF("URI: %s\n", uri);
    /* Read scheme */
    tmp = strchr(ptr, ':');
    if (NULL == tmp) { 
    	DPRINTF("No scheme exists:%s\n",ptr);
        strcpy(path, ptr);    
        return 1;   
    }
    
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

/*
int read_requesthdrs(int fd, char *host, int* port) {
    char buf[MAXLINE];
    int size = 0;
    size = io_recvline_block(fd, buf, MAXLINE);
    //if (strchr(buf,':') == NULL); return 0;

    DPRINTF("%s", buf);
    while(size < 8192 && strcmp(buf, "\r\n")) {

        if( (size = io_recvline_block(fd, buf, MAXLINE)) == 0) return -1;
        DPRINTF("This hdr line:%s,fd:%d\n", buf,fd);
    }
    return 0;
}*/

int read_requesthdrs(int clit_fd, char *host, int* port) {
    int len = 0;
    int i;
    char *tmp;
    char buf[MAXLINE];
    char key[MAXLINE];
    char value[MAXLINE];
    char port_str[MAXLINE];
    char* tmp_buf;
    int tmp_max_size = MAXLINE;
    int tmp_cur_size = 0;
    tmp_buf = (char *)malloc(MAXLINE);
    int host_flag = -1;

    DPRINTF("entering read req hdrs:%d\n", clit_fd);

    while (1) {
        io_recvline_block(clit_fd, buf, MAXLINE);
        len = strlen(buf);
        

        if (len == 0)
            return 1;

        if (buf[len - 1] != '\n') return -1;

        DPRINTF("Receive line:%s\n", buf);

        int desirable_size = tmp_cur_size + len;
        if (desirable_size > tmp_max_size) {
            tmp_buf = realloc(tmp_buf, desirable_size);
            tmp_max_size = desirable_size;
        }
        
        strncpy(tmp_buf + tmp_cur_size, buf, len);
        tmp_cur_size += len;

        if (!strcmp(buf, "\r\n"))
            break;
        tmp = strchr(buf, ':');
        if (NULL == tmp)
            continue;
        *tmp = '\0';
        strcpy(key, buf);
        strcpy(value, tmp + 2);
        value[strlen(value) - 2] = '\0';
        DPRINTF("key = %s, value = %s\n", key, value);
        *tmp = ':';
        if (!strcmp(key, "Host")) {
            host_flag = 1;
            tmp = strchr(value, ':');
            //assert(tmp != NULL);
            if (tmp == NULL) {
                DPRINTF("This host does not contain port, use 80 by default\n");
                strcpy(host, value);
                *port = 80;
                continue;
            }
            *tmp = '\0';
            strcpy(host, value);
            strcpy(port_str, tmp + 1);
            port_str[strlen(port_str)] = '\0';
            *port = atoi(port_str); 
        }
    }
    return host_flag;
}

/** @brief 
 *  @param 
 *  @param
 *  @return the type of the response
 */
void read_responeshdrs(int serv_fd, int clit_fd, response_t* res) {
    int len = 0;
    int i;
    char *tmp;
    char buf[MAXLINE];
    char key[MAXLINE];
    char value[MAXLINE];

    char* tmp_buf;
    int tmp_max_size = MAXLINE;
    int tmp_cur_size = 0;
    tmp_buf = (char *)malloc(MAXLINE);

    res->type = TYPE_MSC;
    res->length = 0;

    DPRINTF("entering read res hdrs:%d\n", serv_fd);

    while (1) {
        io_recvline_block(serv_fd, buf, MAXLINE);
        len = strlen(buf);
        

        if (len == 0)
            return 1;

        if (buf[len - 1] != '\n') return -1;

        DPRINTF("Receive line:%s\n", buf);

        int desirable_size = tmp_cur_size + len;
        if (desirable_size > tmp_max_size) {
            tmp_buf = realloc(tmp_buf, desirable_size);
            tmp_max_size = desirable_size;
        }
        
        strncpy(tmp_buf + tmp_cur_size, buf, len);
        tmp_cur_size += len;

        if (!strcmp(buf, "\r\n"))
            break;
        tmp = strchr(buf, ':');
        if (NULL == tmp)
            continue;
        *tmp = '\0';
        strcpy(key, buf);
        strcpy(value, tmp + 2);
        value[strlen(value) - 2] = '\0';
        DPRINTF("key = %s, value = %s\n", key, value);
        *tmp = ':';
        if (!strcmp(key, "Content-Type")) {
            assert(res->type == TYPE_MSC);
            if (!strcmp(value, "text/xml")) {
                res->type = TYPE_XML;
                continue;
            }
            if (!strcmp(value, "video/f4f")) {
                res->type = TYPE_F4F;
                continue;   
            }
        } else if (!strcmp(key, "Content-Length")) {
            res->length = atoi(value);
        }
    }
    res->hdr_len = tmp_cur_size;
    //res->length = ;
    res->hdr_buf = tmp_buf;
    return;
    /*
    int sendret = io_sendn(clit_fd, tmp_buf, tmp_cur_size);
    if (sendret != tmp_cur_size) {
        DPRINTF("Unsuccessfully forward repsonse\n");
    } else {
        DPRINTF("Successfully forward response\n");
    }
    free(tmp_buf);
    DPRINTF("leaving read request\n");
    return; */
}





















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


bit_t* process_list(int serv_fd, int length) {
    assert(length > 0);
    int resvret;
    char *buf;
    bit_t *bitrates;

    buf = (char *)malloc(length + 1);
    resvret = io_recvn_block(serv_fd, buf, length);
    if (resvret != length) {
        DPRINTF("Short read occured while reading list, ret = %d\n", resvret);
    } else {
        DPRINTF("Successfully read list\n");
    }


    bitrates = parse_xml(buf, length);
    free(buf);
    return bitrates;
}   
