################################################################################
# Makefile  																   #
#                                 											   #
# Description: This file contains the make rules for proxy                     #
#                                                                              #		
# Authors: Ming Fang <mingf@cs.cmu.edu>                                        #
#          Yao Zhou <yaozhou@cs.cmu.edu>                                       #
# 																			   #
################################################################################
CFLAGS = -Wall -g -Ilibxml2 -I./
LDFLAGS = -Llibs -lxml2 -lm
CC = gcc
objects = pool.o io.o mydns.o proxy.o media.o conn.o parse_xml.o timer.o log.o
FMLIB = ./mlib/

default: proxy

.PHONY: default clean clobber handin

proxy: $(objects)
		$(CC) -o $@ $^ $(LDFLAGS)
mydns: mydns.o
		$(CC) -o $@ $^ $(LDFLAGS)

nameserver: nameserver.o mydns.o ospf.o
	    $(CC) -o $@ $^ $(FMLIB)libming.a $(LDFLAGS) 

dnstest: dnstest.o mydns.o
		$(CC) -o $@ $^ $(LDFLAGS)

ospf: ospf.o
		$(CC) -o $@ $^ $(FMLIB)libming.a $(LDFLAGS) 

proxy.o: proxy.c pool.h mydns.h debug.h io.h media.h conn.h timer.h log.h parse_xml.h
pool.o: pool.c pool.h
io.o: io.c io.h
mydns.o: mydns.c mydns.h debug.h
nameserver.o: nameserver.c nameserver.h mydns.h debug.h ospf.h
ospf.o: ospf.c ospf.h
dnstest.o: dnstest.c mydns.h
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
	rm -f proxy mydns nameserver dnstest ospf
	
handin:
	make clean; cd ..; tar cvf handin.tar handin --exclude test
