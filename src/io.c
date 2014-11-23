/** @file io.c                                                               
 *  @brief The I/O package 
 *         modified from 15213 to handle EPIPE and support NONBLOCK socket
 *  @author Ming Fang - mingf@cs.cmu.edu
 *  @bug I am finding
 */

#include "io.h"


/** @brief Send n bytes to a socket or ssl
 *	@param fd the fd to send to
 *  @param ubuf the buf containing things to be sent
 *	@param n the number of bytes to sent
 *  @return -1 on error
 *	@return 0 on EOF
 *  @return the number of bytes sent
 */
ssize_t io_sendn(int fd, const char *ubuf, size_t n) {
    size_t nleft = n;
    ssize_t nsend;
    const char *buf = ubuf;

    while (nleft > 0) {
	if ((nsend = send(fd, buf, nleft, 0)) <= 0) {
	    if (errno == EINTR)  /* interrupted by sig handler return */
			nsend = 0;    /* and call send() again */
		else if (errno == EPIPE) {
			DPRINTF("EPIPE handled\n");
			return -1;
		} else if (errno == EAGAIN) {
			nsend = 0;
		} else {
			DPRINTF("send error on %s\n", strerror(errno));
			return -1;       /* errorno set by send() */
		}
	}
	nleft -= nsend;
	buf += nsend;
    }
    return n;
}

/** @brief Read n bytes from a socket or ssl
 *	@param fd the fd to read from
 *  @param ubuf the buf to store things
 *	@param n the number of bytes to reads
 *  @return -1 on error
 *  @return other the number of bytes reqad
 */
ssize_t io_recvn(int fd, char *buf, size_t n) {
	size_t res = 0;
	int nread;
	size_t nleft = n;

	while (nleft > 0 && (nread = recv(fd, buf + res, nleft, 0)) > 0) {
		nleft -= nread;
		res += nread;
	}
	if (nread == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK){
			DPRINTF("read entire buffer\n");
			DPRINTF("read:%d\n",res );
			return res;
		}
		else {
			DPRINTF("recv error on %s\n", strerror(errno));
			return -1;
		}
	}
	if (nread == 0) {
		DPRINTF("recv error on 0\n");
		return -1;
	}

	return res;
}

/** @brief Read n bytes from a socket or ssl
 *	@param fd the fd to read from
 *  @param ubuf the buf to store things
 *	@param n the number of bytes to reads
 *  @return -1 on error
 *  @return other the number of bytes reqad
 */
ssize_t io_recvn_block(int fd, char *buf, int n) {
	size_t res = 0;
	int nread;
	size_t nleft = n;

	while (1) {
		nread = recv(fd, buf + res, nleft, 0);
		if (nread == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
			//DPRINTF("BLOCK!\n");
			continue;
		} else if (nread == -1) {
			DPRINTF("BLock recv error on %s\n", strerror(errno));
			return -1;
		}
		res += nread;
		if (res == n)
			return res;
	}
}

/** @brief Recv a line from a socket or ssl
 *	@param fd the fd to read from
 *  @param ubuf the buf to store things
 *	@param n the max number of bytes to read
 *  @return -1 on error
 *	@return 0 on EOF
 *  @return the number of bytes read
 */
ssize_t io_recvlineb(int fd, void *usrbuf, size_t maxlen) {
    int n, rc;
    char c, *bufp = usrbuf;

    for (n = 1; n < maxlen; n++) { 
		if ((rc = recv(fd, &c, 1, 0)) == 1) {
		    *bufp++ = c;
		    if (c == '\n')
			break;
		} else if (rc == 0) {
			DPRINTF("recv hdr = 0!\n");	
		    if (n == 1)
				return 0; /* EOF, no data read */
		    else
				return n; /* EOF, some data was read */
		} else {
			if (errno == EWOULDBLOCK || errno == EAGAIN) {
				DPRINTF("read entire buffer once");
				break;
			}
			DPRINTF("recv error on %s\n", strerror(errno));
		    return -1;	  /* error */
		}
    }
    *bufp = 0;
    DPRINTF("read buffer correctly,rc=%d\n",rc);
    DPRINTF("bad bad bad :%s!!!\n",strerror(errno));
    return n;
}


