/*-
 * Copyright (c) 2004-2009 Oracle.  All rights reserved.
 *
 * http://www.apache.org/licenses/LICENSE-2.0.txt
 * 
 * authors: Thies C. Arntzen <thies@php.net>
 *	    Sterling Hughes <sterling@php.net>
 *	    George Schlossnagle <george@omniti.com>
 */
extern "C"
{
#include <stdlib.h>
#include <string.h>
#include "mm_hash.h"
}

MM_Hash *mm_hash_new(MM *mm, MM_HashDtor dtor)
{
	MM_Hash *table;

	if ((table = (MM_Hash *) mm_calloc(mm, 1, sizeof(MM_Hash))) == NULL)
		return NULL;
	table->mm = mm;
	table->dtor = dtor;

	return table;
}

void mm_hash_free(MM_Hash *table) 
{
	MM_Bucket *cur;
	MM_Bucket *next;
	int     i;

	for (i = 0; i < MM_HASH_SIZE; i++) {
		cur = table->buckets[i];
		while (cur) {
			next = cur->next;

			if (table->dtor) table->dtor(cur->data);
			mm_free(table->mm, cur->key);
			mm_free(table->mm, cur);

			cur = next;
		}
	}
	mm_free(table->mm, table);
}

static unsigned int hash_hash(const char *key, int length)
{
    unsigned int hash = 0;

    while (--length)
        hash = hash * 65599 + *key++;

    return hash;
}

void *mm_hash_find(MM_Hash *table, const void *key, int length)
{
	MM_Bucket       *b;
	unsigned int  hash = hash_hash((const char *)key, length) % MM_HASH_SIZE;

    for (b = table->buckets[ hash ]; b; b = b->next) {
		if (hash != b->hash) continue;
		if (length != b->length) continue;
		if (memcmp(key, b->key, length)) continue;

		return b->data;
    }

    return NULL;
}

void mm_hash_update(MM_Hash *table, char *key, int length, void *data)
{
	MM_Bucket *b, p;
	unsigned int  hash;
	
	hash = hash_hash(key, length) % MM_HASH_SIZE;

	for(b = table->buckets[ hash ]; b; b = b->next) {
		if (hash != b->hash) continue;
		if (length != b->length) continue;
		if (memcmp(key, b->key, length)) continue;
		if (table->dtor) table->dtor(b->data);
		b->data = data;
	}
	if(!b) {
    		if ((b = (MM_Bucket *) mm_malloc(
		    table->mm, sizeof(MM_Bucket))) == NULL)
			return;
		if ((b->key = (char *) mm_malloc(
		    table->mm, length + 1)) == NULL) {
			free(b);
			return;
		}
		memcpy(b->key, key, length);
		b->key[length] = 0;
		b->length = length;
		b->hash = hash;
		b->data = data;
		b->next = table->buckets[ hash ];
		table->buckets[ hash ] = b;
	}
	table->nElements++;
}

void mm_hash_delete(MM_Hash *table, char *key, int length)
{
	MM_Bucket       *b; 
	MM_Bucket       *prev = NULL;
	unsigned int  hash;

	hash = hash_hash(key, length) % MM_HASH_SIZE;
    for (b = table->buckets[ hash ]; b; b = b->next) {
		if (hash != b->hash || length != b->length || memcmp(key, b->key, length)) {
			prev = b;
			continue;
		}

		/* unlink */
		if (prev) {
			prev->next = b->next;
		} else {
			table->buckets[hash] = b->next;
		}
		
		if (table->dtor) table->dtor(b->data);
		mm_free(table->mm, b->key);
		mm_free(table->mm, b);

		break;
    }
}

/*
Local variables:
tab-width: 4
c-basic-offset: 4
End:
vim600: noet sw=4 ts=4 fdm=marker
vim<600: noet sw=4 ts=4
*/
