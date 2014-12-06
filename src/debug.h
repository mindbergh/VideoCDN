#ifndef _DEBUG_H
#define _DEBUG_H


#include <sys/types.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

//#define DEBUG
#ifdef DEBUG
#define DPRINTF(fmt, args...) \
        do { fprintf(stderr, fmt, ##args); } while(0)
#else
#define DPRINTF(fmt, args...)
#endif


#endif