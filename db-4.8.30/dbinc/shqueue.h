/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef _DB_SHQUEUE_H_
#define _DB_SHQUEUE_H_

/*
 * This file defines three types of data structures: chains, lists and
 * tail queues similarly to the include file <sys/queue.h>.
 *
 * The difference is that this set of macros can be used for structures that
 * reside in shared memory that may be mapped at different addresses in each
 * process.  In most cases, the macros for shared structures exactly mirror
 * the normal macros, although the macro calls require an additional type
 * parameter, only used by the HEAD and ENTRY macros of the standard macros.
 *
 * Since we use relative offsets of type ssize_t rather than pointers, 0
 * (aka NULL) is a valid offset and cannot be used to indicate the end
 * of a list.  Therefore, we use -1 to indicate end of list.
 *
 * The macros ending in "P" return pointers without checking for end or
 * beginning of lists, the others check for end of list and evaluate to
 * either a pointer or NULL.
 *
 * For details on the use of these macros, see the queue(3) manual page.
 */

#if defined(__cplusplus)
extern "C" {
#endif

#define SH_PTR_TO_OFF(src, dest)                    \
    ((ssize_t)(((u_int8_t *)(dest)) - ((u_int8_t *)(src))))

/*
 * Shared memory chain definitions.
 */
#define SH_CHAIN_ENTRY                          \
struct {                                \
    ssize_t sce_next;   /* relative offset to next element */   \
    ssize_t sce_prev;   /* relative offset of prev element */   \
}

#define SH_CHAIN_INIT(elm, field)                   \
    (elm)->field.sce_next = (elm)->field.sce_prev = -1

#define SH_CHAIN_HASNEXT(elm, field)    ((elm)->field.sce_next != -1)
#define SH_CHAIN_NEXTP(elm, field, type)                \
    ((struct type *)((u_int8_t *)(elm) + (elm)->field.sce_next))
#define SH_CHAIN_NEXT(elm, field, type) (SH_CHAIN_HASNEXT(elm, field) ? \
    SH_CHAIN_NEXTP(elm, field, type) : (struct type *)NULL)

#define SH_CHAIN_HASPREV(elm, field)    ((elm)->field.sce_prev != -1)
#define SH_CHAIN_PREVP(elm, field, type)                \
    ((struct type *)((u_int8_t *)(elm) + (elm)->field.sce_prev))
#define SH_CHAIN_PREV(elm, field, type) (SH_CHAIN_HASPREV(elm, field) ? \
     SH_CHAIN_PREVP(elm, field, type) : (struct type *)NULL)

#define SH_CHAIN_SINGLETON(elm, field)                  \
    (!(SH_CHAIN_HASNEXT(elm, field) || SH_CHAIN_HASPREV(elm, field)))

#define SH_CHAIN_INSERT_AFTER(listelm, elm, field, type) do {       \
    struct type *__next = SH_CHAIN_NEXT(listelm, field, type);  \
    if (__next != NULL) {                       \
        (elm)->field.sce_next = SH_PTR_TO_OFF(elm, __next); \
        __next->field.sce_prev = SH_PTR_TO_OFF(__next, elm);    \
    } else                              \
        (elm)->field.sce_next = -1;             \
    (elm)->field.sce_prev = SH_PTR_TO_OFF(elm, listelm);        \
    (listelm)->field.sce_next = SH_PTR_TO_OFF(listelm, elm);    \
} while (0)

#define SH_CHAIN_INSERT_BEFORE(listelm, elm, field, type) do {      \
    struct type *__prev = SH_CHAIN_PREV(listelm, field, type);  \
    if (__prev != NULL) {                       \
        (elm)->field.sce_prev = SH_PTR_TO_OFF(elm, __prev); \
        __prev->field.sce_next = SH_PTR_TO_OFF(__prev, elm);    \
    } else                              \
        (elm)->field.sce_prev = -1;             \
    (elm)->field.sce_next = SH_PTR_TO_OFF(elm, listelm);        \
    (listelm)->field.sce_prev = SH_PTR_TO_OFF(listelm, elm);    \
} while (0)

#define SH_CHAIN_REMOVE(elm, field, type) do {              \
    struct type *__prev = SH_CHAIN_PREV(elm, field, type);      \
    struct type *__next = SH_CHAIN_NEXT(elm, field, type);      \
    if (__next != NULL)                     \
        __next->field.sce_prev = (__prev == NULL) ? -1 :    \
            SH_PTR_TO_OFF(__next, __prev);          \
    if (__prev != NULL)                     \
        __prev->field.sce_next = (__next == NULL) ? -1 :    \
            SH_PTR_TO_OFF(__prev, __next);          \
    SH_CHAIN_INIT(elm, field);                  \
} while (0)

/*
 * Shared memory list definitions.
 */
#define SH_LIST_HEAD(name)                      \
struct name {                               \
    ssize_t slh_first;  /* first element */         \
}

#define SH_LIST_HEAD_INITIALIZER(head)                  \
    { -1 }

#define SH_LIST_ENTRY                           \
struct {                                \
    ssize_t sle_next;   /* relative offset to next element */   \
    ssize_t sle_prev;   /* relative offset of prev element */   \
}

/*
 * Shared memory list functions.
 */
#define SH_LIST_EMPTY(head)                     \
    ((head)->slh_first == -1)

#define SH_LIST_FIRSTP(head, type)                  \
    ((struct type *)(((u_int8_t *)(head)) + (head)->slh_first))

#define SH_LIST_FIRST(head, type)                   \
    (SH_LIST_EMPTY(head) ? NULL :                   \
    ((struct type *)(((u_int8_t *)(head)) + (head)->slh_first)))

#define SH_LIST_NEXTP(elm, field, type)                 \
    ((struct type *)(((u_int8_t *)(elm)) + (elm)->field.sle_next))

#define SH_LIST_NEXT(elm, field, type)                  \
    ((elm)->field.sle_next == -1 ? NULL :               \
    ((struct type *)(((u_int8_t *)(elm)) + (elm)->field.sle_next)))

  /*
   *__SH_LIST_PREV_OFF is private API.  It calculates the address of
   * the elm->field.sle_next member of a SH_LIST structure.  All offsets
   * between elements are relative to that point in SH_LIST structures.
   */
#define __SH_LIST_PREV_OFF(elm, field)                  \
    ((ssize_t *)(((u_int8_t *)(elm)) + (elm)->field.sle_prev))

#define SH_LIST_PREV(elm, field, type)                  \
    (struct type *)((ssize_t)(elm) - (*__SH_LIST_PREV_OFF(elm, field)))

#define SH_LIST_FOREACH(var, head, field, type)             \
    for ((var) = SH_LIST_FIRST((head), type);           \
        (var) != NULL;                      \
        (var) = SH_LIST_NEXT((var), field, type))

/*
 * Given correct A.next: B.prev = SH_LIST_NEXT_TO_PREV(A)
 * in a list [A, B]
 * The prev value is always the offset from an element to its preceding
 * element's next location, not the beginning of the structure.  To get
 * to the beginning of an element structure in memory given an element
 * do the following:
 * A = B - (B.prev + (&B.next - B))
 * Take the element's next pointer and calculate what the corresponding
 * Prev pointer should be -- basically it is the negation plus the offset
 * of the next field in the structure.
 */
#define SH_LIST_NEXT_TO_PREV(elm, field)                \
    (((elm)->field.sle_next == -1 ? 0 : -(elm)->field.sle_next) +   \
       SH_PTR_TO_OFF(elm, &(elm)->field.sle_next))

#define SH_LIST_INIT(head) (head)->slh_first = -1

#define SH_LIST_INSERT_BEFORE(head, listelm, elm, field, type) do { \
    if (listelm == SH_LIST_FIRST(head, type)) {         \
    SH_LIST_INSERT_HEAD(head, elm, field, type);            \
    } else {                            \
        (elm)->field.sle_next = SH_PTR_TO_OFF(elm, listelm);    \
        (elm)->field.sle_prev = SH_LIST_NEXT_TO_PREV(       \
            SH_LIST_PREV((listelm), field, type), field) +  \
        (elm)->field.sle_next;                  \
        (SH_LIST_PREV(listelm, field, type))->field.sle_next =  \
            (SH_PTR_TO_OFF((SH_LIST_PREV(listelm, field,    \
                             type)), elm)); \
    (listelm)->field.sle_prev = SH_LIST_NEXT_TO_PREV(elm, field);   \
    }                               \
} while (0)

#define SH_LIST_INSERT_AFTER(listelm, elm, field, type) do {        \
    if ((listelm)->field.sle_next != -1) {              \
        (elm)->field.sle_next = SH_PTR_TO_OFF(elm,      \
            SH_LIST_NEXTP(listelm, field, type));       \
        SH_LIST_NEXTP(listelm, field, type)->field.sle_prev =   \
            SH_LIST_NEXT_TO_PREV(elm, field);       \
    } else                              \
        (elm)->field.sle_next = -1;             \
    (listelm)->field.sle_next = SH_PTR_TO_OFF(listelm, elm);    \
    (elm)->field.sle_prev = SH_LIST_NEXT_TO_PREV(listelm, field);   \
} while (0)

#define SH_LIST_INSERT_HEAD(head, elm, field, type) do {        \
    if ((head)->slh_first != -1) {                  \
        (elm)->field.sle_next =                 \
            (head)->slh_first - SH_PTR_TO_OFF(head, elm);   \
        SH_LIST_FIRSTP(head, type)->field.sle_prev =        \
            SH_LIST_NEXT_TO_PREV(elm, field);       \
    } else                              \
        (elm)->field.sle_next = -1;             \
    (head)->slh_first = SH_PTR_TO_OFF(head, elm);           \
    (elm)->field.sle_prev = SH_PTR_TO_OFF(elm, &(head)->slh_first); \
} while (0)

#define SH_LIST_REMOVE(elm, field, type) do {               \
    if ((elm)->field.sle_next != -1) {              \
        SH_LIST_NEXTP(elm, field, type)->field.sle_prev =   \
            (elm)->field.sle_prev - (elm)->field.sle_next;  \
        *__SH_LIST_PREV_OFF(elm, field) += (elm)->field.sle_next;\
    } else                              \
        *__SH_LIST_PREV_OFF(elm, field) = -1;           \
} while (0)

#define SH_LIST_REMOVE_HEAD(head, field, type) do {         \
    if (!SH_LIST_EMPTY(head)) {                 \
        SH_LIST_REMOVE(SH_LIST_FIRSTP(head, type), field, type);\
    }                               \
} while (0)

/*
 * Shared memory tail queue definitions.
 */
#define SH_TAILQ_HEAD(name)                     \
struct name {                               \
    ssize_t stqh_first; /* relative offset of first element */  \
    ssize_t stqh_last;  /* relative offset of last's next */    \
}

#define SH_TAILQ_HEAD_INITIALIZER(head)                 \
    { -1, 0 }

#define SH_TAILQ_ENTRY                          \
struct {                                \
    ssize_t stqe_next;  /* relative offset of next element */   \
    ssize_t stqe_prev;  /* relative offset of prev's next */    \
}

/*
 * Shared memory tail queue functions.
 */

#define SH_TAILQ_EMPTY(head)                        \
    ((head)->stqh_first == -1)

#define SH_TAILQ_FIRSTP(head, type)                 \
    ((struct type *)((u_int8_t *)(head) + (head)->stqh_first))

#define SH_TAILQ_FIRST(head, type)                  \
    (SH_TAILQ_EMPTY(head) ? NULL : SH_TAILQ_FIRSTP(head, type))

#define SH_TAILQ_NEXTP(elm, field, type)                \
    ((struct type *)((u_int8_t *)(elm) + (elm)->field.stqe_next))

#define SH_TAILQ_NEXT(elm, field, type)                 \
    ((elm)->field.stqe_next == -1 ? NULL :              \
    ((struct type *)((u_int8_t *)(elm) + (elm)->field.stqe_next)))

  /*
   * __SH_TAILQ_PREV_OFF is private API.  It calculates the address of
   * the elm->field.stqe_next member of a SH_TAILQ structure.  All
   * offsets between elements are relative to that point in SH_TAILQ
   * structures.
   */
#define __SH_TAILQ_PREV_OFF(elm, field)                 \
    ((ssize_t *)(((u_int8_t *)(elm)) + (elm)->field.stqe_prev))

#define SH_TAILQ_PREVP(elm, field, type)                \
    (struct type *)((ssize_t)elm - (*__SH_TAILQ_PREV_OFF(elm, field)))

#define SH_TAILQ_PREV(head, elm, field, type)               \
    (((elm) == SH_TAILQ_FIRST(head, type)) ? NULL :     \
      (struct type *)((ssize_t)elm - (*__SH_TAILQ_PREV_OFF(elm, field))))

  /*
   * __SH_TAILQ_LAST_OFF is private API.  It calculates the address of
   * the stqe_next member of a SH_TAILQ structure in the last element
   * of this list.  All offsets between elements are relative to that
   * point in SH_TAILQ structures.
   */
#define __SH_TAILQ_LAST_OFF(head)                   \
    ((ssize_t *)(((u_int8_t *)(head)) + (head)->stqh_last))

#define SH_TAILQ_LASTP(head, field, type)               \
    ((struct type *)((ssize_t)(head) +              \
     ((ssize_t)((head)->stqh_last) -                \
     ((ssize_t)SH_PTR_TO_OFF(SH_TAILQ_FIRST(head, type),        \
        &(SH_TAILQ_FIRSTP(head, type)->field.stqe_next))))))

#define SH_TAILQ_LAST(head, field, type)                \
    (SH_TAILQ_EMPTY(head) ? NULL : SH_TAILQ_LASTP(head, field, type))

/*
 * Given correct A.next: B.prev = SH_TAILQ_NEXT_TO_PREV(A)
 * in a list [A, B]
 * The prev value is always the offset from an element to its preceding
 * element's next location, not the beginning of the structure.  To get
 * to the beginning of an element structure in memory given an element
 * do the following:
 * A = B - (B.prev + (&B.next - B))
 */
#define SH_TAILQ_NEXT_TO_PREV(elm, field)               \
    (((elm)->field.stqe_next == -1 ? 0 :                \
        (-(elm)->field.stqe_next) +             \
        SH_PTR_TO_OFF(elm, &(elm)->field.stqe_next)))

#define SH_TAILQ_FOREACH(var, head, field, type)            \
    for ((var) = SH_TAILQ_FIRST((head), type);          \
        (var) != NULL;                      \
        (var) = SH_TAILQ_NEXT((var), field, type))

#define SH_TAILQ_FOREACH_REVERSE(var, head, field, type)        \
    for ((var) = SH_TAILQ_LAST((head), field, type);        \
        (var) != NULL;                      \
        (var) = SH_TAILQ_PREV((head), (var), field, type))

#define SH_TAILQ_INIT(head) {                       \
    (head)->stqh_first = -1;                    \
    (head)->stqh_last = SH_PTR_TO_OFF(head, &(head)->stqh_first);   \
}

#define SH_TAILQ_INSERT_HEAD(head, elm, field, type) do {       \
    if ((head)->stqh_first != -1) {                 \
        (elm)->field.stqe_next =                \
            (head)->stqh_first - SH_PTR_TO_OFF(head, elm);  \
        SH_TAILQ_FIRSTP(head, type)->field.stqe_prev =      \
            SH_TAILQ_NEXT_TO_PREV(elm, field);      \
    } else {                            \
        (head)->stqh_last =                 \
            SH_PTR_TO_OFF(head, &(elm)->field.stqe_next);   \
        (elm)->field.stqe_next = -1;                \
    }                               \
    (head)->stqh_first = SH_PTR_TO_OFF(head, elm);          \
    (elm)->field.stqe_prev =                    \
        SH_PTR_TO_OFF(elm, &(head)->stqh_first);            \
} while (0)

#define SH_TAILQ_INSERT_TAIL(head, elm, field) do {         \
    (elm)->field.stqe_next = -1;                    \
    (elm)->field.stqe_prev =                    \
        -SH_PTR_TO_OFF(head, elm) + (head)->stqh_last;      \
    if ((head)->stqh_last ==                    \
        SH_PTR_TO_OFF((head), &(head)->stqh_first))         \
        (head)->stqh_first = SH_PTR_TO_OFF(head, elm);      \
    else                                \
        *__SH_TAILQ_LAST_OFF(head) = -(head)->stqh_last +   \
            SH_PTR_TO_OFF((elm), &(elm)->field.stqe_next) + \
            SH_PTR_TO_OFF(head, elm);               \
    (head)->stqh_last =                     \
        SH_PTR_TO_OFF(head, &((elm)->field.stqe_next));     \
} while (0)

#define SH_TAILQ_INSERT_BEFORE(head, listelm, elm, field, type) do {    \
    if (listelm == SH_TAILQ_FIRST(head, type)) {            \
        SH_TAILQ_INSERT_HEAD(head, elm, field, type);       \
    } else {                            \
        (elm)->field.stqe_next = SH_PTR_TO_OFF(elm, listelm);   \
        (elm)->field.stqe_prev = SH_TAILQ_NEXT_TO_PREV(     \
            SH_TAILQ_PREVP((listelm), field, type), field) + \
            (elm)->field.stqe_next;             \
        (SH_TAILQ_PREVP(listelm, field, type))->field.stqe_next =\
        (SH_PTR_TO_OFF((SH_TAILQ_PREVP(listelm, field, type)),  \
            elm));                      \
        (listelm)->field.stqe_prev =                \
            SH_TAILQ_NEXT_TO_PREV(elm, field);      \
    }                               \
} while (0)

#define SH_TAILQ_INSERT_AFTER(head, listelm, elm, field, type) do { \
    if ((listelm)->field.stqe_next != -1) {             \
        (elm)->field.stqe_next = (listelm)->field.stqe_next -   \
            SH_PTR_TO_OFF(listelm, elm);            \
        SH_TAILQ_NEXTP(listelm, field, type)->field.stqe_prev = \
            SH_TAILQ_NEXT_TO_PREV(elm, field);          \
    } else {                            \
        (elm)->field.stqe_next = -1;                \
        (head)->stqh_last =                 \
            SH_PTR_TO_OFF(head, &(elm)->field.stqe_next);   \
    }                               \
    (listelm)->field.stqe_next = SH_PTR_TO_OFF(listelm, elm);   \
    (elm)->field.stqe_prev = SH_TAILQ_NEXT_TO_PREV(listelm, field); \
} while (0)

#define SH_TAILQ_REMOVE(head, elm, field, type) do {            \
    if ((elm)->field.stqe_next != -1) {             \
        SH_TAILQ_NEXTP(elm, field, type)->field.stqe_prev = \
            (elm)->field.stqe_prev +                \
            SH_PTR_TO_OFF(SH_TAILQ_NEXTP(elm,           \
            field, type), elm);                 \
        *__SH_TAILQ_PREV_OFF(elm, field) += (elm)->field.stqe_next;\
    } else {                            \
        (head)->stqh_last = (elm)->field.stqe_prev +        \
            SH_PTR_TO_OFF(head, elm);           \
        *__SH_TAILQ_PREV_OFF(elm, field) = -1;          \
    }                               \
} while (0)

#if defined(__cplusplus)
}
#endif
#endif  /* !_DB_SHQUEUE_H_ */
