#include "pool.h"
#include "debug.h"


static const char *VIDEO_SERVER_ADDR = "video.cs.cmu.edu";
static const char *VIDEO_SERVER_PORT = "8080";


/** @brief Initial the pool of client fds to be select
* @param listen_sock The socket on which the server is listenning
* while initial, this should be the greatest fd
* @param p the pointer to the pool
* @return Void
*/
void init_pool(int listen_sock, pool_t *p) {
    int i;
    p->maxi = -1;
    for (i = 0; i < FD_SETSIZE; i++) {
        p->conn[i] = NULL;
    }
    p->maxfd = listen_sock;
    p->cur_conn = 0;
    p->serv_sock = -1;
    p->fake_ip = NULL;
    p->www_ip = NULL;
    p->alpha = -1;
    FD_ZERO(&p->read_set);
    FD_ZERO(&p->write_set);
    FD_SET(listen_sock, &p->read_set);
}


/** @brief Create a socket to lesten
 *  @param port The number of port to be binded
 *  @return The fd of created listenning socket
 */
int open_listen_socket(int port) {
    int listen_socket;
    int yes = 1;        // for setsockopt() SO_REUSEADDR
    struct sockaddr_in addr;

    /* all networked programs must create a socket */
    if ((listen_socket = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Failed creating socket.\n");
        return EXIT_FAILURE;
    }

    // lose the pesky "address already in use" error message
    setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* servers bind sockets to ports--notify the OS they accept connections */
    if (bind(listen_socket, (struct sockaddr *) &addr, sizeof(addr))) {
        close_socket(listen_socket);
        fprintf(stderr, "Failed binding socket for port %d.\n", port);
        return EXIT_FAILURE;
    }


    if (listen(listen_socket, LISTENQ)) {
        close_socket(listen_socket);
        fprintf(stderr, "Error listening on socket.\n");
        return EXIT_FAILURE;
    }

    return listen_socket;
}


int open_server_socket(char *fake_ip, char *www_ip) {
    int serverfd;
    struct addrinfo *result = NULL;
    struct sockaddr_in fake_addr;
    struct sockaddr_in serv_addr;
    int rc;

    assert(fake_ip != NULL);

    /* Create the socket descriptor */
    if ((serverfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    //fcntl(serverfd, F_SETFL, O_NONBLOCK);
    
    memset(&fake_addr, '0', sizeof(fake_addr)); 
    fake_addr.sin_family = AF_INET;
    inet_pton(AF_INET, fake_ip, &(fake_addr.sin_addr));
    fake_addr.sin_port = htons(0);  // let system assgin one 
    rc = bind(serverfd, (struct sockaddr *)&fake_addr, sizeof(fake_addr));
    if (rc < 0) {
        DPRINTF("Bind server sockt error!");
        return -1;
    }

    if (www_ip == NULL) {
        // server ip is not specified, ask DNS
        rc = resolve(VIDEO_SERVER_ADDR, VIDEO_SERVER_PORT, NULL, &result);
        if (rc < 0) {
            // handle error
            DPRINTF("Resolve error!\n");
            return -1;
        }
        // connect to address in result
        rc = connect(serverfd, result->ai_addr, result->ai_addrlen);
    } else {
        // server ip is specified
        memset(&serv_addr, '0', sizeof(serv_addr));
        serv_addr.sin_family = AF_INET; 
        inet_pton(AF_INET, www_ip, &(serv_addr.sin_addr));
        serv_addr.sin_port = htons(8080);
        rc = connect(serverfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    }
    if (rc < 0) {
        // handle error
        DPRINTF("Connect error!\n");
        return -1;
    }
    /* Clean up */
    if (result)
        free(result);
    return serverfd;    
}


/** @brief Add a new client fd
 *  @param conn_sock The socket of client to be added
 *  @param p the pointer to the pool
 *  @param cli_addr the struct contains addr info
 *  @param port the port of the client
 *  @return Void
 */
void add_client(int conn_sock, pool_t *p) {
    int i;
    conn_t *conni;
    p->cur_conn++;
    p->nready--;
    for (i = 0; i < FD_SETSIZE; i++)
        if (p->conn[i] == NULL) {
            p->conn[i] = (conn_t *)malloc(sizeof(conn_t));
            conni = p->conn[i];
            //conni->buf = (char *)malloc(BUF_SIZE);
            conni->cur_size = 0;
            conni->size = BUF_SIZE;
            conni->fd = conn_sock;
            conni->thruput = 0;
            FD_SET(conn_sock, &p->read_set);

            if (conn_sock > p->maxfd)
                p->maxfd = conn_sock;
            if (i > p->maxi)
                p->maxi = i;
            //log_write_string("HTTP client added: %s", bufi->addr);
            break;
        }


    if (i == FD_SETSIZE) {
        fprintf(stderr, "Too many client.\n");        
        exit(EXIT_FAILURE);
    }
}

/** @brief Free a Buff struct that represents a connection
 *  @param bufi the Buff struct to be freeed
 *  @return Void
 */
void free_buf(pool_t *p, conn_t *conni) {
    //free(conni->buf);
    free(conni);
}

/** @brief Wrapper function for closing socket
 *  @param sock The socket fd to be closed
 *  @return 0 on sucess, 1 on error
 */ 
int close_socket(int sock) {
    DPRINTF("Close sock %d\n", sock);
    //log_write_string("Close sock %d\n", sock);
    if (close(sock)) {
        DPRINTF("Failed closing socket.\n");
        return 1;
    }
    return 0;
}


/** @brief Clean up all current connected socket
 *  @param p the pointer to the pool
 *  @return Void
 */
void clean_state(pool_t *p, int listen_sock) {
    int i, conn_sock;
    for (i = 0; i <= p->maxi; i++) {
        if (p->conn[i]) {
            conn_sock = p->conn[i]->fd;
            if (close_socket(conn_sock)) {
                DPRINTF("Error closing client socket.\n");                        
            }   
            p->cur_conn--;
            FD_CLR(conn_sock, &p->read_set);
            free_buf(p, p->conn[i]);
            p->conn[i] = NULL;                               
        }
    }
    p->maxi = -1;
    p->maxfd = listen_sock;
    p->cur_conn = 0;
}

/** @brief Close given connection
 *  @param p the Pool struct
 *         i the ith connection in the pool
 *  @return Void
 */
void close_conn(pool_t *p, int i) {
    //if (p->buf[i] == NULL)
        //return;
    int conn_sock = p->conn[i]->fd;
    if (close_socket(conn_sock)) {
        fprintf(stderr, "Error closing client socket.\n");                        
    }
    p->cur_conn--;
    free_buf(p, p->conn[i]);
    FD_CLR(conn_sock, &p->read_set);
    FD_CLR(conn_sock, &p->write_set);
    p->conn[i] = NULL;
}