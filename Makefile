################################################################################
# Makefile  																   #
#                                 											   #
# Description: This file contains the make rules for proxy                     #
#                                                                              #
# Authors: Ming Fang <mingf@cs.cmu.edu>                                        #
#          Yao Zhou <yaozhou@cs.cmu.edu>                                       #
# 																			   #
################################################################################
CFLAGS = -Wall -g
CC = gcc
objects = pool.o io.o mydns.o proxy.o

default: proxy

.PHONY: default clean clobber handin

proxy: $(objects)
		$(CC) -o $@ $^ $(LDFLAGS)

proxy.o: proxy.c pool.h mydns.h debug.h io.h
pool.o: pool.c pool.h
io.o: io.c io.h
mydns.o: mydns.c mydns.h

%.o: %.c
		$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f proxy.o pool.o

clobber: clean
	rm -f proxy

handin:
	(make clean; cd ..; tar cvf handin.tar 15-441-project-1 --exclude test --exclude cp1_checker.py --exclude README --exclude static_site --exclude dumper.py --exclude liso_prototype.py --exclude http_parser.h)