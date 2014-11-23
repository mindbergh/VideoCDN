#ifndef _DEBUG_H
#define _DEBUG_H


#include <sys/types.h>
#include <stdlib.h>
#include <assert.h>

#define VERBOSE 0
#define DPRINTF(fmt, args...) \
        do { if (VERBOSE) fprintf(stderr, fmt, ##args); } while(0)

#endif