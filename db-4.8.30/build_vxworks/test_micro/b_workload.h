/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

/*
 * Macros to help with initializing/assigning key dbts
 */

#define KBUF_LEN 12
#define INIT_KEY(key, config) do {              \
    memset(&key, 0, sizeof(key));               \
    if (config->orderedkeys) {              \
        key.size = sizeof (u_int32_t);          \
    } else if (config->ksize != 0) {            \
        DB_BENCH_ASSERT(                \
            (key.data = malloc(key.size = config->ksize)) != NULL); \
    } else {                        \
        key.data = kbuf;                \
        key.size = 10;                  \
    }                           \
    } while (0)

#define GET_KEY_NEXT(key, config, kbuf, i) do {         \
    size_t tmp_int;                     \
    if (config->orderedkeys) {              \
        /* Will be sorted on little-endian system. */   \
        tmp_int = i;                    \
        M_32_SWAP(tmp_int);             \
        key.data = &tmp_int;                \
    } else if (config->ksize == 0) {            \
        /*                      \
         * This will produce duplicate keys.        \
         * That is not such a big deal, since we are    \
         * using the same seed to srand each time,  \
         * the scenario is reproducible.        \
         */                     \
        (void)snprintf(kbuf, sizeof(kbuf), "%10d", rand()); \
    } else {                        \
        /* TODO: Not sure of the best approach here. */ \
        (void)snprintf(key.data, config->ksize, "%10lu", (u_long)i); \
    }                           \
    } while (0)

/* Taken from dbinc/db_swap.h */
#undef  M_32_SWAP
#define M_32_SWAP(a) {                          \
    u_int32_t _tmp;                         \
    _tmp = (u_int32_t)a;                        \
    ((u_int8_t *)&a)[0] = ((u_int8_t *)&_tmp)[3];           \
    ((u_int8_t *)&a)[1] = ((u_int8_t *)&_tmp)[2];           \
    ((u_int8_t *)&a)[2] = ((u_int8_t *)&_tmp)[1];           \
    ((u_int8_t *)&a)[3] = ((u_int8_t *)&_tmp)[0];           \
}

/*
 * A singly linked list, that maintains a pointer
 * to the start and the end of the queue.
 * Should be possible to use a STAILQ, but this seemed easier
 */
typedef struct bench_qentry {
    char data[KBUF_LEN];
    struct bench_qentry *next;
}bench_qentry;
typedef struct bench_q {
    struct bench_qentry *head;
    struct bench_qentry *tail;
} bench_q;
#define BENCH_Q_TAIL_INSERT(queue, buf) do {        \
    struct bench_qentry *entry;             \
    DB_BENCH_ASSERT(                \
        (entry = malloc(sizeof(struct bench_qentry))) != NULL); \
    memcpy(entry->data, buf, sizeof(entry->data));  \
    if (queue.head == NULL)             \
        queue.head = queue.tail = entry;    \
    else {                      \
        queue.tail->next = entry;       \
        queue.tail = entry;         \
    }                       \
} while (0)

#define BENCH_Q_POP(queue, buf) do {            \
    struct bench_qentry *popped = queue.head;   \
    if (popped == NULL)             \
        break;                  \
    if (queue.head->next == NULL)           \
        queue.head = queue.tail = NULL;     \
    else                        \
        queue.head = queue.head->next;      \
    memcpy(buf, popped->data, sizeof(buf)); \
    free(popped);                   \
} while (0)

/*
 * Retrieve the head of the queue, save the data into user
 * buffer, and push the item back onto the end of the list.
 * Same functionality as pop/insert, but saves a malloc/free
 */
#define BENCH_Q_POP_PUSH(queue, buf) do {       \
    struct bench_qentry *popped = queue.head;   \
    if (popped == NULL)             \
        break;                  \
    if (queue.head->next == NULL)           \
        queue.head = queue.tail = NULL;     \
    else                        \
        queue.head = queue.head->next;      \
    memcpy(buf, popped->data, sizeof(buf)); \
    if (queue.head == NULL)             \
        queue.head = queue.tail = popped;   \
    else {                      \
        queue.tail->next = popped;      \
        queue.tail = popped;            \
    }                       \
} while (0)

typedef enum {
    T_PUT,
    T_GET,
    T_DELETE,
    T_PUT_GET,
    T_PUT_DELETE,
    T_PUT_GET_DELETE,
    T_GET_DELETE,
    T_MIXED
} test_type;

typedef struct
{
    u_int32_t ksize;
    u_int32_t dsize;
    size_t orderedkeys;
    size_t num_dups;
    u_int32_t pagesz;
    u_int32_t cachesz;
    u_int32_t pcount;
    size_t gcount;
    size_t cursor_del;
    size_t verbose;
    test_type workload;
    u_int32_t seed;
    size_t presize;
    DBTYPE type;
    char   *ts;
    char   *message;
    /* Fields used to store timing information */
    db_timespec put_time;
    db_timespec get_time;
    db_timespec del_time;
    db_timespec tot_time;
} CONFIG;
