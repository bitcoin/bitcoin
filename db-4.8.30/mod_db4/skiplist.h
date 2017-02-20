/* ======================================================================
 * Copyright (c) 2000,2006 Theo Schlossnagle
 * All rights reserved.
 * The following code was written by Theo Schlossnagle for use in the
 * Backhand project at The Center for Networking and Distributed Systems
 * at The Johns Hopkins University.
 *
 * This is a skiplist implementation to be used for abstract structures
 * and is release under the LGPL license version 2.1 or later.  A copy
 * of this license can be found file LGPL.
 *
 * Alternatively, this file may be licensed under the new BSD license.
 * A copy of this license can be found file BSD.
 *
 * ======================================================================
*/

#ifndef _SKIPLIST_P_H
#define _SKIPLIST_P_H

/* This is a skiplist implementation to be used for abstract structures
   within the Spread multicast and group communication toolkit

   This portion written by -- Theo Schlossnagle <jesus@cnds.jhu.eu>
*/

/* This is the function type that must be implemented per object type
   that is used in a skiplist for comparisons to maintain order */
typedef int (*SkiplistComparator)(void *, void *);
typedef void (*FreeFunc)(void *);

struct skiplistnode;

typedef struct _iskiplist {
  SkiplistComparator compare;
  SkiplistComparator comparek;
  int height;
  int preheight;
  int size;
  struct skiplistnode *top;
  struct skiplistnode *bottom;
  /* These two are needed for appending */
  struct skiplistnode *topend;
  struct skiplistnode *bottomend;
  struct _iskiplist *index;
} Skiplist;

struct skiplistnode {
  void *data;
  struct skiplistnode *next;
  struct skiplistnode *prev;
  struct skiplistnode *down;
  struct skiplistnode *up;
  struct skiplistnode *previndex;
  struct skiplistnode *nextindex;
  Skiplist *sl;
};


void skiplist_init(Skiplist *sl);
void skiplist_set_compare(Skiplist *sl, SkiplistComparator,
            SkiplistComparator);
void skiplist_add_index(Skiplist *sl, SkiplistComparator,
          SkiplistComparator);
struct skiplistnode *skiplist_getlist(Skiplist *sl);
void *skiplist_find_compare(Skiplist *sl, void *data, struct skiplistnode **iter,
              SkiplistComparator func);
void *skiplist_find(Skiplist *sl, void *data, struct skiplistnode **iter);
void *skiplist_next(Skiplist *sl, struct skiplistnode **);
void *skiplist_previous(Skiplist *sl, struct skiplistnode **);

struct skiplistnode *skiplist_insert_compare(Skiplist *sl,
                       void *data, SkiplistComparator comp);
struct skiplistnode *skiplist_insert(Skiplist *sl, void *data);
int skiplist_remove_compare(Skiplist *sl, void *data,
              FreeFunc myfree, SkiplistComparator comp);
int skiplist_remove(Skiplist *sl, void *data, FreeFunc myfree);
int skiplisti_remove(Skiplist *sl, struct skiplistnode *m, FreeFunc myfree);
void skiplist_remove_all(Skiplist *sl, FreeFunc myfree);

int skiplisti_find_compare(Skiplist *sl,
            void *data,
            struct skiplistnode **ret,
            SkiplistComparator comp);

void *skiplist_pop(Skiplist * a, FreeFunc myfree);

#endif
