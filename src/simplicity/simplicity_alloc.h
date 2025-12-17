#ifndef SIMPLICITY_SIMPLICITY_ALLOC_H
#define SIMPLICITY_SIMPLICITY_ALLOC_H

#include <stdlib.h>

/* Allocate with malloc by default. */
#define simplicity_malloc malloc

/* Allocate+zero initialize with calloc by default. */
#define simplicity_calloc calloc

/* Deallocate with free by default. */
#define simplicity_free free

#endif /* SIMPLICITY_SIMPLICITY_ALLOC_H */
