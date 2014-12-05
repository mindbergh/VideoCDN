#ifndef _DEBUG_H
#define _DEBUG_H


#include <sys/types.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#define VERBOSE 1
#define DPRINTF(fmt, args...) \
        do { if (VERBOSE) fprintf(stderr, fmt, ##args); } while(0)

#endif