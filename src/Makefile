################################################################################
# Makefile  																   #
#                                 											   #
# Description: This file contains the make rules for proxy                     #
#                                                                              #
# Authors: Ming Fang <mingf@cs.cmu.edu>                                        #
#          Yao Zhou <yaozhou@cs.cmu.edu>                                       #
# 																			   #
################################################################################
CFLAGS = -Wall -g -Ilibxml2
LDFLAGS = -Llibs -lxml2 -lm
CC = gcc
objects = pool.o io.o mydns.o proxy.o media.o conn.o parse_xml.o timer.o log.o

default: proxy

.PHONY: default clean clobber handin

proxy: $(objects)
		$(CC) -o $@ $^ $(LDFLAGS)

proxy.o: proxy.c pool.h mydns.h debug.h io.h media.h conn.h timer.h log.h parse_xml.h
pool.o: pool.c pool.h
io.o: io.c io.h
mydns.o: mydns.c mydns.h
media.o: media.c media.h
conn.o: conn.c conn.h
parse_xml.o: parse_xml.c parse_xml.h
timer.o: timer.c timer.h
log.o: log.c log.h

%.o: %.ce
		$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f *.o

clobber: clean
	rm -f proxy

handin:
	make clean; cd ..; tar cvf 15-441-project-3.tar 15-441-project-3 --exclude test
