#include "pool.h"



extern pool_t pool;

static const char *VIDEO_SERVER_ADDR = "video.cs.cmu.edu";
static const char *VIDEO_SERVER_PORT = "8080";


/** @brief Initial the pool of client fds to be select
* @param listen_sock The socket on which the server is listenning
* while initial, this should be the greatest fd
* @param p the pointer to the pool
* @return Void
*/
void init_pool(int listen_sock, pool_t *p, char** argv) {
    int i;
    gettimeofday(&(pool.start), NULL);
    pool.max_conn_idx = -1;
    pool.max_clit_idx = -1;
    pool.max_serv_idx = -1;
    for (i = 0; i < FD_SETSIZE; i++) {
        pool.conn_l[i] = NULL;
        pool.client_l[i] = NULL;
        pool.server_l[i] = NULL;
    }

    pool.maxfd = listen_sock;
    pool.cur_conn = 0;
    pool.log_file = fopen(argv[1],"a+");
    if (pool.log_file == NULL) {
        DPRINTF("failed to open log file!\n");
        exit(-1);
    }
    pool.fake_ip = argv[4];
    pool.alpha = -1;
    pool.cur_conn = 0;
    pool.cur_client = 0;
    pool.cur_server = 0;

    if(argv[7])
        pool.www_ip = argv[7];
    fprintf(stderr, "\n");
    FD_ZERO(&(pool.read_set));
    FD_ZERO(&(pool.write_set));
    FD_SET(listen_sock, &(pool.read_set));
    pool.maxfd = listen_sock;
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
    int nonblock_flags = fcntl(serverfd,F_GETFL,0);
    fcntl(serverfd, F_SETFL,nonblock_flags|O_NONBLOCK);
    return serverfd;    
}


/** @brief Add a new client fd
 *  @param conn_sock The socket of client to be added
 *  @param p the pointer to the pool
 *  @param cli_addr the struct contains addr info
 *  @param port the port of the client
 *  @return Void
 */
int add_client(int conn_sock, uint32_t addr) {
    int i;
    client_t* new_client;
    client_t** client_l = pool.client_l;

    for (i = 0; i < FD_SETSIZE; i++)
        if (client_l[i] == NULL) {
            new_client = (client_t*)malloc(sizeof(client_t));
            new_client->cur_size = 0;
            new_client->size = BUF_SIZE;
            new_client->fd = conn_sock;
            new_client->addr = addr;

            client_l[i] = new_client;
            pool.cur_client++;

            FD_SET(conn_sock, &(pool.read_set));
            if (conn_sock > pool.maxfd) {
                pool.maxfd = conn_sock;
            }
            if (i > pool.max_clit_idx) {
                pool.max_clit_idx = i;
            }
            return i;
        }


    /* failed to add new server */
    return -1;
}



int add_server(int sock, uint32_t addr) {
    server_t* new_server;
    server_t** serv_l = pool.server_l;
    int i = 0;

    for(; i < FD_SETSIZE;i++) {

        if (serv_l[i] == NULL) {
            new_server = (server_t*)malloc(sizeof(server_t));
            new_server->fd = sock;
            new_server->addr = addr;
            new_server->cur_size = 0;
            new_server->size = MAXBUF;

            serv_l[i] = new_server;
            pool.cur_server++;
            
            FD_SET(sock, &(pool.read_set));
            if (sock > pool.maxfd) {
                pool.maxfd = sock;
            }
            if (i > pool.max_serv_idx) {
                pool.max_serv_idx = i;
            }
            return i;
        }
    }

    /* failed to add new server */
    return -1;
}





void close_clit(int clit_idx) {
    client_t *client = GET_CLIT_BY_IDX(clit_idx);
    close_socket(client->fd);
    free(client);
    GET_CLIT_BY_IDX(clit_idx) = NULL;    
    pool.cur_client--;
}

void close_serv(int serv_idx) {
    client_t *server = GET_CLIT_BY_IDX(serv_idx);
    close_socket(server->fd);
    free(server);
    GET_CLIT_BY_IDX(serv_idx) = NULL;
    pool.cur_server--;    
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
    return;
    // rewrite whole thing
}