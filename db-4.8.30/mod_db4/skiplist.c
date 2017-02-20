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

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "skiplist.h"
}

#ifdef USE_DMALLOC
#    include <dmalloc.h>
#endif

#ifndef MIN
#define MIN(a,b) ((a<b)?(a):(b))
#endif

#define MALLOC(type, ptr) do {					\
  void *tmptr = malloc(sizeof(struct type));			\
  if (tmptr == NULL){						\
    fprintf(stderr, 						\
    "In file %s on line %d: malloc failed to allocate memory, exiting...", \
      __FILE__, __LINE__);					\
      assert(0);						\
  }								\
  ptr = (struct type*)tmptr;					\
} while (0)

#define MALLOC_N(type, num, ptr) do {				\
  void *tmptr = NULL;						\
  assert(num >= 0);						\
  tmptr = malloc(sizeof(struct type) * num);			\
  if (tmptr == NULL){						\
    fprintf(stderr, 						\
    "In file %s on line %d: malloc failed to allocate memory, exiting...", \
      __FILE__, __LINE__);					\
      assert(0);						\
  }								\
  ptr = (struct type*)tmptr;					\
} while (0)

static int get_b_rand(void) {
  static int ph=32; /* More bits than we will ever use */
  static unsigned long randseq;
  if(ph > 31) { /* Num bits in return of lrand48() */
    ph=0;
    randseq = lrand48();
  }
  ph++;
  return ((randseq & (1 << (ph-1))) >> (ph-1));
}

void skiplisti_init(Skiplist *sl) {
  sl->compare = (SkiplistComparator)NULL;
  sl->comparek = (SkiplistComparator)NULL;
  sl->height=0;
  sl->preheight=0;
  sl->size=0;
  sl->top = NULL;
  sl->bottom = NULL;
  sl->index = NULL;
}

static int indexing_comp(void *a, void *b) {
  assert(a);
  assert(b);
  return (void *)(((Skiplist *)a)->compare)>(void *)(((Skiplist *)b)->compare);
}
static int indexing_compk(void *a, void *b) {
  assert(b);
  return a>(void *)(((Skiplist *)b)->compare);
}

void skiplist_init(Skiplist *sl) {
  skiplisti_init(sl);
  MALLOC(_iskiplist, sl->index);
  skiplisti_init(sl->index);
  skiplist_set_compare(sl->index, indexing_comp, indexing_compk);
}

void skiplist_set_compare(Skiplist *sl,
		    SkiplistComparator comp,
		    SkiplistComparator compk) {
  if(sl->compare && sl->comparek) {
    skiplist_add_index(sl, comp, compk);
  } else {
    sl->compare = comp;
    sl->comparek = compk;
  }
}

void skiplist_add_index(Skiplist *sl,
		  SkiplistComparator comp,
		  SkiplistComparator compk) {
  struct skiplistnode *m;
  Skiplist *ni;
  int icount=0;
#ifdef SLDEBUG
  fprintf(stderr, "Adding index to %p\n", sl);
#endif
  skiplist_find(sl->index, (void *)comp, &m);
  if(m) return; /* Index already there! */
  MALLOC(_iskiplist, ni);
  skiplisti_init(ni);
  skiplist_set_compare(ni, comp, compk);
  /* Build the new index... This can be expensive! */
  m = skiplist_insert(sl->index, ni);
  while(m->prev) m=m->prev, icount++;
  for(m=skiplist_getlist(sl); m; skiplist_next(sl, &m)) {
    int j=icount-1;
    struct skiplistnode *nsln;
    nsln = skiplist_insert(ni, m->data);
    /* skip from main index down list */
    while(j>0) m=m->nextindex, j--;
    /* insert this node in the indexlist after m */
    nsln->nextindex = m->nextindex;
    if(m->nextindex) m->nextindex->previndex = nsln;
    nsln->previndex = m;
    m->nextindex = nsln;
  } 
}

struct skiplistnode *skiplist_getlist(Skiplist *sl) {
  if(!sl->bottom) return NULL;
  return sl->bottom->next;
}

void *skiplist_find(Skiplist *sl,
	      void *data,
	      struct skiplistnode **iter) {
  void *ret;
  struct skiplistnode *aiter;
  if(!sl->compare) return 0;
  if(iter)
    ret = skiplist_find_compare(sl, data, iter, sl->compare);
  else
    ret = skiplist_find_compare(sl, data, &aiter, sl->compare);
  return ret;
}  
void *skiplist_find_compare(Skiplist *sli,
		      void *data,
		      struct skiplistnode **iter,
		      SkiplistComparator comp) {
  struct skiplistnode *m = NULL;
  Skiplist *sl;
  if(comp==sli->compare || !sli->index) {
    sl = sli;
  } else {
    skiplist_find(sli->index, (void *)comp, &m);
    assert(m);
    sl= (Skiplist *) m->data;
  }
  skiplisti_find_compare(sl, data, iter, sl->comparek);
  return (*iter)?((*iter)->data):(*iter);
}
int skiplisti_find_compare(Skiplist *sl,
		     void *data,
		     struct skiplistnode **ret,
		     SkiplistComparator comp) {
  struct skiplistnode *m = NULL;
  int count=0;
  m = sl->top;
  while(m) {
    int compared;
    if(m->next) compared=comp(data, m->next->data);
    if(compared == 0) {
#ifdef SL_DEBUG
      printf("Looking -- found in %d steps\n", count);
#endif
      m=m->next;
      while(m->down) m=m->down;
      *ret = m;
      return count;
    }
    if((m->next == NULL) || (compared<0))
      m = m->down, count++;
    else
      m = m->next, count++;
  }
#ifdef SL_DEBUG
  printf("Looking -- not found in %d steps\n", count);
#endif
  *ret = NULL;
  return count;
}
void *skiplist_next(Skiplist *sl, struct skiplistnode **iter) {
  if(!*iter) return NULL;
  *iter = (*iter)->next;
  return (*iter)?((*iter)->data):NULL;
}
void *skiplist_previous(Skiplist *sl, struct skiplistnode **iter) {
  if(!*iter) return NULL;
  *iter = (*iter)->prev;
  return (*iter)?((*iter)->data):NULL;
}
struct skiplistnode *skiplist_insert(Skiplist *sl,
			       void *data) {
  if(!sl->compare) return 0;
  return skiplist_insert_compare(sl, data, sl->compare);
}

struct skiplistnode *skiplist_insert_compare(Skiplist *sl,
				       void *data,
				       SkiplistComparator comp) {
  struct skiplistnode *m, *p, *tmp, *ret, **stack;
  int nh=1, ch, stacki;
#ifdef SLDEBUG
  skiplist_print_struct(sl, "BI: ");
#endif
  if(!sl->top) {
    sl->height = 1;
    MALLOC(skiplistnode, sl->bottom);
    sl->topend = sl->bottomend = sl->top = sl->bottom;
    assert(sl->top);
    sl->top->next = (struct skiplistnode *) NULL;
    sl->top->data = (struct skiplistnode *) NULL;
    sl->top->prev =(struct skiplistnode *) NULL;
	sl->top->up = (struct skiplistnode *) NULL;
    sl->top->down = (struct skiplistnode *) NULL;
	sl->top->nextindex=  (struct skiplistnode *) NULL;
    sl->top->previndex = (struct skiplistnode *) NULL;
    sl->top->sl = sl;
  }
  if(sl->preheight) {
    while(nh < sl->preheight && get_b_rand()) nh++;
  } else {
    while(nh <= sl->height && get_b_rand()) nh++;
  }
  /* Now we have the new height at which we wish to insert our new node */
  /* Let us make sure that our tree is a least that tall (grow if necessary)*/
  for(;sl->height<nh;sl->height++) {
    MALLOC(skiplistnode, sl->top->up);
    sl->top->up->down = sl->top;
    sl->top = sl->topend = sl->top->up;
    sl->top->prev = sl->top->next = sl->top->nextindex =
      sl->top->previndex = sl->top->up = NULL;
    sl->top->data = NULL;
    sl->top->sl = sl;
  }
  ch = sl->height;
  /* Find the node (or node after which we would insert) */
  /* Keep a stack to pop back through for insertion */
  m = sl->top;
  MALLOC_N(skiplistnode*, nh, stack);
  stacki=0;
  while(m) {
    int compared=-1;
    if(m->next) compared=comp(data, m->next->data);
    if(compared == 0) {
      free(stack);
      return 0;
    }
    if((m->next == NULL) || (compared<0)) {
      if(ch<=nh) {
	/* push on stack */
	stack[stacki++] = m;
      }
      m = m->down;
      ch--;
    } else {
      m = m->next;
    }
  }
  /* Pop the stack and insert nodes */
  p = NULL;
  for(;stacki>0;stacki--) {
    m = stack[stacki-1];
    MALLOC(skiplistnode, tmp);
    tmp->next = m->next;
    if(m->next) m->next->prev=tmp;
    tmp->prev = m;
    tmp->up = NULL;
    tmp->nextindex = tmp->previndex = NULL;
    tmp->down = p;
    if(p) p->up=tmp;
    tmp->data = data;
    tmp->sl = sl;
    m->next = tmp;
    /* This sets ret to the bottom-most node we are inserting */
    if(!p) ret=tmp;
    p = tmp;
  }
  free(stack);
  if(sl->index != NULL) {
    /* this is a external insertion, we must insert into each index as well */
    struct skiplistnode *p, *ni, *li;
    li=ret;
    for(p = skiplist_getlist(sl->index); p; skiplist_next(sl->index, &p)) {
      ni = skiplist_insert((Skiplist *)p->data, ret->data);
      assert(ni);
#ifdef SLDEBUG
      fprintf(stderr, "Adding %p to index %p\n", ret->data, p->data);
#endif
      li->nextindex = ni;
      ni->previndex = li;
      li = ni;
    }
  } else {
    sl->size++;
  }
#ifdef SLDEBUG
  skiplist_print_struct(sl, "AI: ");
#endif
  return ret;
}
struct skiplistnode *skiplist_append(Skiplist *sl, void *data) {
  int nh=1, ch, compared;
  struct skiplistnode *lastnode, *nodeago;
  if(sl->bottomend != sl->bottom) {
    compared=sl->compare(data, sl->bottomend->prev->data);
    /* If it doesn't belong at the end, then fail */
    if(compared<=0) return NULL;
  }
  if(sl->preheight) {
    while(nh < sl->preheight && get_b_rand()) nh++;
  } else {
    while(nh <= sl->height && get_b_rand()) nh++;
  }
  /* Now we have the new height at which we wish to insert our new node */
  /* Let us make sure that our tree is a least that tall (grow if necessary)*/
  lastnode = sl->bottomend;
  nodeago = NULL;

  if(!lastnode) return skiplist_insert(sl, data);

  for(;sl->height<nh;sl->height++) {
    MALLOC(skiplistnode, sl->top->up); 
    assert(sl->top);
    sl->top->up->down = sl->top;
    sl->top = sl->top->up;
    sl->top->prev = sl->top->next = sl->top->nextindex =
      sl->top->previndex = NULL;
    sl->top->data = NULL;
    sl->top->sl = sl;
  }
  ch = sl->height;
  while(nh) {
    struct skiplistnode *anode;
    MALLOC(skiplistnode, anode); 
    anode->next = lastnode;
    anode->prev = lastnode->prev;
    anode->up = NULL;
    anode->down = nodeago;
    if(lastnode->prev) {
      if(lastnode == sl->bottom)
	sl->bottom = anode;
      else if (lastnode == sl->top)
	sl->top = anode;
    }
    nodeago = anode;
    lastnode = lastnode->up;
    nh--;
  }
  sl->size++;
  return sl->bottomend;
}
Skiplist *skiplist_concat(Skiplist *sl1, Skiplist *sl2) {
  /* Check integrity! */
  int compared, eheight;
  Skiplist temp;
  struct skiplistnode *lbottom, *lbottomend, *b1, *e1, *b2, *e2;
  if(sl1->bottomend == NULL || sl1->bottomend->prev == NULL) {
    skiplist_remove_all(sl1, free);
    temp = *sl1;
    *sl1 = *sl2;
    *sl2 = temp;
    /* swap them so that sl2 can be freed normally upon return. */
    return sl1;
  }
  if(sl2->bottom == NULL || sl2->bottom->next == NULL) {
    skiplist_remove_all(sl2, free);
    return sl1;
  }
  compared = sl1->compare(sl1->bottomend->prev->data, sl2->bottom->data);
  /* If it doesn't belong at the end, then fail */
  if(compared<=0) return NULL;
  
  /* OK now append sl2 onto sl1 */
  lbottom = lbottomend = NULL;
  eheight = MIN(sl1->height, sl2->height);
  b1 = sl1->bottom; e1 = sl1->bottomend;
  b2 = sl2->bottom; e2 = sl2->bottomend;
  while(eheight) {    
    e1->prev->next = b2;
    b2->prev = e1->prev->next;
    e2->prev->next = e1;
    e1->prev = e2->prev;
    e2->prev = NULL;
    b2 = e2;
    b1->down = lbottom;
    e1->down = lbottomend;
    if(lbottom) lbottom->up = b1;
    if(lbottomend) lbottomend->up = e1;
    
    lbottom = b1;
    lbottomend = e1;
  }
  /* Take the top of the longer one (if it is sl2) and make it sl1's */
  if(sl2->height > sl1->height) {
    b1->up = b2->up;
    e1->up = e2->up;
    b1->up->down = b1;
    e1->up->down = e1;
    sl1->height = sl2->height;
    sl1->top = sl2->top;
    sl1->topend = sl2->topend;
  }

  /* move the top pointer to here if it isn't there already */
  sl2->top = sl2->topend = b2;
  sl2->top->up = NULL; /* If it isn't already */
  sl1->size += sl2->size;
  skiplist_remove_all(sl2, free);
  return sl1;
}
int skiplist_remove(Skiplist *sl,
	      void *data, FreeFunc myfree) {
  if(!sl->compare) return 0;
  return skiplist_remove_compare(sl, data, myfree, sl->comparek);
}
void skiplist_print_struct(Skiplist *sl, char *prefix) {
  struct skiplistnode *p, *q;
  fprintf(stderr, "Skiplist Structure (height: %d)\n", sl->height);
  p = sl->bottom;
  while(p) {
    q = p;
    fprintf(stderr, "%s", prefix);
    while(q) {
      fprintf(stderr, "%p ", q->data);
      q=q->up;
    }
    fprintf(stderr, "\n");
    p=p->next;
  }
}
int skiplisti_remove(Skiplist *sl, struct skiplistnode *m, FreeFunc myfree) {
  struct skiplistnode *p;
  if(!m) return 0;
  if(m->nextindex) skiplisti_remove(m->nextindex->sl, m->nextindex, NULL);
  else sl->size--;
#ifdef SLDEBUG
  skiplist_print_struct(sl, "BR:");
#endif
  while(m->up) m=m->up;
  while(m) {
    p=m;
    p->prev->next = p->next; /* take me out of the list */
    if(p->next) p->next->prev = p->prev; /* take me out of the list */
    m=m->down;
    /* This only frees the actual data in the bottom one */
    if(!m && myfree && p->data) myfree(p->data);
    free(p);
  }
  while(sl->top && sl->top->next == NULL) {
    /* While the row is empty and we are not on the bottom row */
    p = sl->top;
    sl->top = sl->top->down; /* Move top down one */
    if(sl->top) sl->top->up = NULL;      /* Make it think its the top */
    free(p);
    sl->height--;
  }
  if(!sl->top) sl->bottom = NULL;
  assert(sl->height>=0);
#ifdef SLDEBUG
  skiplist_print_struct(sl, "AR: ");
#endif
  return sl->height;
}
int skiplist_remove_compare(Skiplist *sli,
		      void *data,
		      FreeFunc myfree, SkiplistComparator comp) {
  struct skiplistnode *m;
  Skiplist *sl;
  if(comp==sli->comparek || !sli->index) {
    sl = sli;
  } else {
    skiplist_find(sli->index, (void *)comp, &m);
    assert(m);
    sl= (Skiplist *) m->data;
  }
  skiplisti_find_compare(sl, data, &m, comp);
  if(!m) return 0;
  while(m->previndex) m=m->previndex;
  return skiplisti_remove(sl, m, myfree);
}
void skiplist_remove_all(Skiplist *sl, FreeFunc myfree) {
  /* This must remove even the place holder nodes (bottom though top)
     because we specify in the API that one can free the Skiplist after
     making this call without memory leaks */
  struct skiplistnode *m, *p, *u;
  m=sl->bottom;
  while(m) {
    p = m->next;
    if(myfree && p->data) myfree(p->data);
    while(m) {
      u = m->up;
      free(m);
      m=u;
    }
    m = p;
  }
  sl->top = sl->bottom = NULL;
  sl->height = 0;
  sl->size = 0;
}

void *skiplist_pop(Skiplist * a, FreeFunc myfree)
{
  struct skiplistnode *sln;
  void *data = NULL;
  sln = skiplist_getlist(a);
  if (sln) {
    data = sln->data;
    skiplisti_remove(a, sln, myfree);
  }
  return data;
}
