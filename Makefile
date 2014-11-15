################################################################################
# Makefile #
# #
# Description: This file contains the make rules for proxy #
# #
# Authors: Yao Zhou <yaozhou@cs.cmu.edu>, #
# #
################################################################################
CFLAGS = -Wall -g
CC = gcc
LDFLAGS = -lssl
objects = pool.h proxy.o

default: proxy

.PHONY: default clean clobber handin

proxy: $(objects)
		$(CC) -o $@ $^ $(LDFLAGS)

proxy.o: proxy.c pool.c
pool.o: pool.c

%.o: %.c
		$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f proxy.o pool.o

clobber: clean
	rm -f proxy

handin:
	(make clean; cd ..; tar cvf handin.tar 15-441-project-1 --exclude test --exclude cp1_checker.py --exclude README --exclude static_site --exclude dumper.py --exclude liso_prototype.py --exclude http_parser.h)