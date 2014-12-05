/* Debug routines
 *
 * Author: Ming Fang <mingf@cs.cmu.edu>
 * Date: 11/26/2014
 */

#ifndef MDEBUG_H
#define MDEBUG_H


//#define DEBUG

#ifdef DEBUG
#define DPRINTF(fmt, args...) \
        do { fprintf(stderr, fmt, ##args); } while(0)
#else
#define DPRINTF(fmt, args...)
#endif

#endif