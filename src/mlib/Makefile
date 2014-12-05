CFLAGS = -Wall -g -I../
LDFLAGS = -L../
CC = gcc
objects = mlist.o mqueue.o
AR = ar




libming.a: $(objects)
	$(AR) r $@ $^

mlist.o: mlist.c mlist.h mutil.h mtypes.h
mqueue.o: mqueue.c mqueue.h mutil.h mtypes.h


%.o: %.ce
	$(CC) -c $(CFLAGS) -o $@ $<