/* Some type define
 *
 * Author: Ming Fang <mingf@cs.cmu.edu>
 * Time:   11/26/2014
 */

#ifndef MTYPES_H
#define MTYPES_H

#include <stdbool.h>


typedef int (*MCompareFunc) (const void* a, const void* b);
typedef int (*MCompareDataFunc) (const void* a, const void* b, void* func_data);
typedef bool (*MEqualFunc) (const void* a, const void* b);
typedef void (*MFunc) (void* data, void* func_data);

#endif
