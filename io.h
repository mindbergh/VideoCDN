#ifndef _IO_H
#define _IO_H

#include "debug.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

ssize_t io_sendn(int fd, const char *ubuf, size_t n);
ssize_t io_recvn(int fd, char *buf, size_t n);
ssize_t io_recvlineb(int fd, void *usrbuf, size_t maxlen);

#endif