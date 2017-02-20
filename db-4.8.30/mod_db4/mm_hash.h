/*-
 * Copyright (c) 2004-2009 Oracle.  All rights reserved.
 *
 * http://www.apache.org/licenses/LICENSE-2.0.txt
 * 
 * authors: Thies C. Arntzen <thies@php.net>
 *      Sterling Hughes <sterling@php.net>
 *      George Schlossnagle <george@omniti.com>
 */

#ifndef _MM_HASH_H
#define _MM_HASH_H
#include "mm.h"

typedef void (*MM_HashDtor)(void *);

typedef struct _MM_Bucket {
    struct _MM_Bucket *next;
    char *key;
    int length;
    unsigned int hash;
    void *data;
} MM_Bucket;

#define MM_HASH_SIZE 1009
typedef struct _Hash {
     MM_Bucket *buckets[ MM_HASH_SIZE ];
     MM *mm;
     MM_HashDtor dtor;
     int nElements;
} MM_Hash;

MM_Hash *mm_hash_new(MM *, MM_HashDtor);
void mm_hash_free(MM_Hash *table);
void *mm_hash_find(MM_Hash *table, const void *key, int length);
void mm_hash_add(MM_Hash *table, char *key, int length, void *data);
void mm_hash_delete(MM_Hash *table, char *key, int length);
void mm_hash_update(MM_Hash *table, char *key, int length, void *data);
#endif

/*
Local variables:
tab-width: 4
c-basic-offset: 4
End:
vim600: noet sw=4 ts=4 fdm=marker
vim<600: noet sw=4 ts=4
*/
