/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2006-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef _DB_REPMGR_H_
#define _DB_REPMGR_H_

#include "dbinc_auto/repmgr_auto.h"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Replication Framework message types.  These values are transmitted to
 * identify messages sent between sites, even sites running differing versions
 * of software.  Therefore, once assigned, the values are permanently "frozen".
 * New message types added in later versions always get new (higher) values.
 *
 * For example, in repmgr wire protocol version 1 the highest assigned message
 * type value was 3, for REPMGR_REP_MESSAGE.  Wire protocol version 2 added the
 * HEARTBEAT message type (4).
 *
 * We still list them in alphabetical order, for ease of reference.  But this
 * generally does not correspond to numerical order.
 */
#define REPMGR_ACK      1   /* Acknowledgement. */
#define REPMGR_HANDSHAKE    2   /* Connection establishment sequence. */
#define REPMGR_HEARTBEAT    4   /* Monitor connection health. */
#define REPMGR_REP_MESSAGE  3   /* Normal replication message. */

/* Heartbeats were introduced in version 2. */
#define REPMGR_MAX_V1_MSG_TYPE  3
#define REPMGR_MAX_V2_MSG_TYPE  4
#define REPMGR_MAX_V3_MSG_TYPE  4
#define HEARTBEAT_MIN_VERSION   2

/* The range of protocol versions we're willing to support. */
#define DB_REPMGR_VERSION   3
#define DB_REPMGR_MIN_VERSION   1

#ifdef DB_WIN32
typedef SOCKET socket_t;
typedef HANDLE thread_id_t;
typedef HANDLE mgr_mutex_t;
typedef HANDLE cond_var_t;
typedef WSABUF db_iovec_t;
#else
typedef int socket_t;
typedef pthread_t thread_id_t;
typedef pthread_mutex_t mgr_mutex_t;
typedef pthread_cond_t cond_var_t;
typedef struct iovec db_iovec_t;
#endif

/*
 * The (arbitrary) maximum number of outgoing messages we're willing to hold, on
 * a queue per connection, waiting for TCP buffer space to become available in
 * the kernel.  Rather than exceeding this limit, we simply discard additional
 * messages (since this is always allowed by the replication protocol).
 *    As a special dispensation, if a message is destined for a specific remote
 * site (i.e., it's not a broadcast), then we first try blocking the sending
 * thread, waiting for space to become available (though we only wait a limited
 * time).  This is so as to be able to handle the immediate flood of (a
 * potentially large number of) outgoing messages that replication generates, in
 * a tight loop, when handling PAGE_REQ, LOG_REQ and ALL_REQ requests.
 */
#define OUT_QUEUE_LIMIT 10

/*
 * The system value is available from sysconf(_SC_HOST_NAME_MAX).
 * Historically, the maximum host name was 256.
 */
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN  256
#endif

/* A buffer big enough for the string "site host.domain.com:65535". */
#define MAX_SITE_LOC_STRING (MAXHOSTNAMELEN+20)
typedef char SITE_STRING_BUFFER[MAX_SITE_LOC_STRING+1];

/* Default timeout values, in seconds. */
#define DB_REPMGR_DEFAULT_ACK_TIMEOUT       (1 * US_PER_SEC)
#define DB_REPMGR_DEFAULT_CONNECTION_RETRY  (30 * US_PER_SEC)
#define DB_REPMGR_DEFAULT_ELECTION_RETRY    (10 * US_PER_SEC)

struct __repmgr_connection;
    typedef struct __repmgr_connection REPMGR_CONNECTION;
struct __repmgr_queue; typedef struct __repmgr_queue REPMGR_QUEUE;
struct __queued_output; typedef struct __queued_output QUEUED_OUTPUT;
struct __repmgr_retry; typedef struct __repmgr_retry REPMGR_RETRY;
struct __repmgr_runnable; typedef struct __repmgr_runnable REPMGR_RUNNABLE;
struct __repmgr_site; typedef struct __repmgr_site REPMGR_SITE;
struct __ack_waiters_table;
    typedef struct __ack_waiters_table ACK_WAITERS_TABLE;

typedef TAILQ_HEAD(__repmgr_conn_list, __repmgr_connection) CONNECTION_LIST;
typedef STAILQ_HEAD(__repmgr_out_q_head, __queued_output) OUT_Q_HEADER;
typedef TAILQ_HEAD(__repmgr_retry_q, __repmgr_retry) RETRY_Q_HEADER;

/* Information about threads managed by Replication Framework. */
struct __repmgr_runnable {
    ENV *env;
    thread_id_t thread_id;
    void *(*run) __P((void *));
    int finished;
};

/*
 * Information about pending connection establishment retry operations.
 *
 * We keep these in order by time.  This works, under the assumption that the
 * DB_REP_CONNECTION_RETRY never changes once we get going (though that
 * assumption is of course wrong, so this needs to be fixed).
 *
 * Usually, we put things onto the tail end of the list.  But when we add a new
 * site while threads are running, we trigger its first connection attempt by
 * scheduling a retry for "0" microseconds from now, putting its retry element
 * at the head of the list instead.
 *
 * TODO: I think this can be fixed by defining "time" to be the time the element
 * was added (with some convention like "0" meaning immediate), rather than the
 * deadline time.
 */
struct __repmgr_retry {
    TAILQ_ENTRY(__repmgr_retry) entries;
    u_int eid;
    db_timespec time;
};

/*
 * We use scatter/gather I/O for both reading and writing.  The largest number
 * of buffers we ever try to use at once is 5, corresponding to the 5 segments
 * of a message described in the "wire protocol" (repmgr_net.c).
 */
typedef struct {
    db_iovec_t vectors[5];

    /*
     * Index of the first iovec to be used.  Initially of course this is
     * zero.  But as we progress through partial I/O transfers, it ends up
     * pointing to the first iovec to be used on the next operation.
     */
    int offset;

    /*
     * Total number of pieces defined for this message; equal to the number
     * of times add_buffer and/or add_dbt were called to populate it.  We do
     * *NOT* revise this as we go along.  So subsequent I/O operations must
     * use count-offset to get the number of active vector pieces still
     * remaining.
     */
    int count;

    /*
     * Total number of bytes accounted for in all the pieces of this
     * message.  We do *NOT* revise this as we go along (though it's not
     * clear we shouldn't).
     */
    size_t total_bytes;
} REPMGR_IOVECS;

typedef struct {
    size_t length;      /* number of bytes in data */
    int ref_count;      /* # of sites' send queues pointing to us */
    u_int8_t data[1];   /* variable size data area */
} REPMGR_FLAT;

struct __queued_output {
    STAILQ_ENTRY(__queued_output) entries;
    REPMGR_FLAT *msg;
    size_t offset;
};

/*
 * The following is for input.  Once we know the sizes of the pieces of an
 * incoming message, we can create this struct (and also the data areas for the
 * pieces themselves, in the same memory allocation).  This is also the struct
 * in which the message lives while it's waiting to be processed by message
 * threads.
 */
typedef struct __repmgr_message {
    STAILQ_ENTRY(__repmgr_message) entries;
    int originating_eid;
    DBT control, rec;
} REPMGR_MESSAGE;

typedef enum {
    SIZES_PHASE,
    DATA_PHASE
} phase_t;

/*
 * If another site initiates a connection to us, when we receive it the
 * connection state is immediately "connected".  But when we initiate the
 * connection to another site, it first has to go through a "connecting" state,
 * until the non-blocking connect() I/O operation completes successfully.
 *     With an outgoing connection, we always know the associated site (and so
 * we have a valid eid).  But with an incoming connection, we don't know the
 * site until we get a handshake message, so until that time the eid is
 * invalid.
 */
struct __repmgr_connection {
    TAILQ_ENTRY(__repmgr_connection) entries;

    int eid;        /* index into sites array in machtab */
    socket_t fd;
#ifdef DB_WIN32
    WSAEVENT event_object;
#endif

    u_int32_t version;  /* Wire protocol version on this connection. */
                /* (0 means not yet determined.) */

#define CONN_INCOMING   0x01    /* We received this via accept(). */
    u_int32_t flags;

/*
 * When we initiate an outgoing connection, it starts off in CONNECTING state
 * (or possibly CONNECTED).  When the (non-blocking) connection operation later
 * completes, we move to CONNECTED state.  When we get the response to our
 * version negotiation, we move to READY.
 *     For incoming connections that we accept, we start in NEGOTIATE, then to
 * PARAMETERS, and then to READY.
 *     CONGESTED is a hierarchical substate of READY: it's just like READY, with
 * the additional wrinkle that we don't bother waiting for the outgoing queue to
 * drain in certain circumstances.
 */
#define CONN_CONGESTED  1   /* Long-lived full outgoing queue. */
#define CONN_CONNECTED  2   /* Awaiting reply to our version negotiation. */
#define CONN_CONNECTING 3   /* Awaiting completion of non-block connect. */
#define CONN_DEFUNCT    4   /* Basically dead, awaiting clean-up. */
#define CONN_NEGOTIATE  5   /* Awaiting version proposal. */
#define CONN_PARAMETERS 6   /* Awaiting parameters handshake. */
#define CONN_READY  7   /* Everything's fine. */
    int state;

    /*
     * Output: usually we just simply write messages right in line, in the
     * send() function's thread.  But if TCP doesn't have enough network
     * buffer space for us when we first try it, we instead allocate some
     * memory, and copy the message, and then send it as space becomes
     * available in our main select() thread.  In some cases, if the queue
     * gets too long we wait until it's drained, and then append to it.
     * This condition variable's associated mutex is the normal per-repmgr
     * db_rep->mutex, because that mutex is always held anyway whenever the
     * output queue is consulted.
     */
    OUT_Q_HEADER outbound_queue;
    int out_queue_length;
    cond_var_t drained;
    int blockers;       /* ref count of msg threads waiting on us */

    /*
     * Input: while we're reading a message, we keep track of what phase
     * we're in.  In both phases, we use a REPMGR_IOVECS to keep track of
     * our progress within the phase.  Depending upon the message type, we
     * end up with either a rep_message (which is a wrapper for the control
     * and rec DBTs), or a single generic DBT.
     *     Any time we're in DATA_PHASE, it means we have already received
     * the message header (consisting of msg_type and 2 sizes), and
     * therefore we have allocated buffer space to read the data.  (This is
     * important for resource clean-up.)
     */
    phase_t     reading_phase;
    REPMGR_IOVECS iovecs;

    u_int8_t    msg_type;
    u_int32_t   control_size_buf, rec_size_buf;

    union {
        REPMGR_MESSAGE *rep_message;
        struct {
            DBT cntrl, rec;
        } repmgr_msg;
    } input;
};

#define IS_READY_STATE(s)   ((s) == CONN_READY || (s) == CONN_CONGESTED)

#ifdef HAVE_GETADDRINFO
typedef struct addrinfo ADDRINFO;
#else
/*
 * Some windows platforms have getaddrinfo (Windows XP), some don't.  We don't
 * support conditional compilation in our Windows build, so we always use our
 * own getaddrinfo implementation.  Rename everything so that we don't collide
 * with the system libraries.
 */
#undef  AI_PASSIVE
#define AI_PASSIVE  0x01
#undef  AI_CANONNAME
#define AI_CANONNAME    0x02
#undef  AI_NUMERICHOST
#define AI_NUMERICHOST  0x04

typedef struct __addrinfo {
    int ai_flags;       /* AI_PASSIVE, AI_CANONNAME, AI_NUMERICHOST */
    int ai_family;      /* PF_xxx */
    int ai_socktype;    /* SOCK_xxx */
    int ai_protocol;    /* 0 or IPPROTO_xxx for IPv4 and IPv6 */
    size_t ai_addrlen;  /* length of ai_addr */
    char *ai_canonname; /* canonical name for nodename */
    struct sockaddr *ai_addr;   /* binary address */
    struct __addrinfo *ai_next; /* next structure in linked list */
} ADDRINFO;
#endif /* HAVE_GETADDRINFO */

/*
 * Unprocessed network address configuration, as stored in shared region.
 */
typedef struct {
    roff_t host;        /* Separately allocated copy of string. */
    u_int16_t port;     /* Stored in plain old host-byte-order. */
} SITEADDR;

/*
 * Local copy of local and remote addresses, with resolved addrinfo.
 */
typedef struct {
    char *host;     /* Separately allocated copy of string. */
    u_int16_t port;     /* Stored in plain old host-byte-order. */
    ADDRINFO *address_list;
    ADDRINFO *current;
} repmgr_netaddr_t;

/*
 * Each site that we know about is either idle or connected.  If it's connected,
 * we have a reference to a connection object; if it's idle, we have a reference
 * to a retry object.  (But see note about sub_conns, below.)
 *     We store site objects in a simple array in the machtab, indexed by EID.
 * (We allocate EID numbers for other sites simply according to their index
 * within this array; we use the special value INT_MAX to represent our own
 * EID.)
 */
struct __repmgr_site {
    repmgr_netaddr_t net_addr;
    DB_LSN max_ack;     /* Best ack we've heard from this site. */
    u_int32_t priority;
    db_timespec last_rcvd_timestamp;

    union {
        REPMGR_CONNECTION *conn; /* when CONNECTED */
        REPMGR_RETRY *retry; /* when IDLE */
    } ref;

    /*
     * Subordinate connections (connections from subordinate processes at a
     * multi-process site).  Note that the SITE_CONNECTED state, and all the
     * ref.retry stuff above is irrelevant to subordinate connections.  If a
     * connection is on this list, it exists; and we never bother trying to
     * reconnect lost connections (indeed we can't, for these are always
     * incoming-only).
     */
    CONNECTION_LIST sub_conns;

#define SITE_IDLE 1     /* Waiting til time to retry connecting. */
#define SITE_CONNECTED 2
    int state;

#define SITE_HAS_PRIO   0x01    /* Set if priority field has valid value. */
    u_int32_t flags;
};

/*
 * Repmgr keeps track of references to connection information (instances
 * of struct __repmgr_connection).  There are three kinds of places
 * connections may be found: (1) SITE->ref.conn, (2) SITE->sub_conns, and
 * (3) db_rep->connections.
 *
 * 1. SITE->ref.conn points to our connection with the main process running
 * at the given site, if such a connection exists.  We may have initiated
 * the connection to the site ourselves, or we may have received it as an
 * incoming connection.  Once it is established there is very little
 * difference between those two cases.
 *
 * 2. SITE->sub_conns is a list of connections we have with subordinate
 * processes running at the given site.  There can be any number of these
 * connections, one per subordinate process.  Note that these connections
 * are always incoming: there's no way for us to initiate this kind of
 * connection because subordinate processes do not "listen".
 *
 * 3. The db_rep->connections list contains the references to any
 * connections that are not actively associated with any site (we
 * sometimes call these "orphans").  There are two times when this can
 * be:
 *
 *   a) When we accept an incoming connection, we don't know what site it
 *      comes from until we read the initial handshake message.
 *
 *   b) When an error occurs on a connection, we first mark it as DEFUNCT
 *      and stop using it.  Then, at a later, well-defined time, we close
 *      the connection's file descriptor and get rid of the connection
 *      struct.
 *
 * In light of the above, we can see that the following describes the
 * rules for how connections may be moved among these three kinds of
 * "places":
 *
 * - when we initiate an outgoing connection, we of course know what site
 *   it's going to be going to, and so we immediately put the pointer to
 *   the connection struct into SITE->ref.conn
 *
 * - when we accept an incoming connection, we don't immediately know
 *   whom it's from, so we have to put it on the orphans list
 *   (db_rep->connections).
 *
 * - (incoming, cont.) But as soon as we complete the initial "handshake"
 *   message exchange, we will know which site it's from and whether it's
 *   a subordinate or main connection.  At that point we remove it from
 *   db_rep->connections and either point to it by SITE->ref.conn, or add
 *   it to the SITE->sub_conns list.
 *
 * - (for any active connection) when an error occurs, we move the
 *   connection to the orphans list until we have a chance to close it.
 */

/*
 * Repmgr message formats.
 *
 * Declarative definitions of current message formats appear in repmgr.src.
 * (The s_message/gen_msg.awk utility generates C code.)  In general, we send
 * the buffers marshaled from those structure formats in the "control" portion
 * of a message.
 */

/*
 * Flags for the handshake message (new in 4.8).
 */
#define REPMGR_SUBORDINATE  0x01    /* This is a subordinate connection. */

/*
 * Legacy V1 handshake message format.  For compatibility, we send this as part
 * of version negotiation upon connection establishment.
 */
typedef struct {
    u_int32_t version;
    u_int16_t port;
    u_int32_t priority;
} DB_REPMGR_V1_HANDSHAKE;

/*
 * We store site structs in a dynamically allocated, growable array, indexed by
 * EID.  We allocate EID numbers for remote sites simply according to their
 * index within this array.  We don't need (the same kind of) info for ourself
 * (the local site), so we use an EID value that won't conflict with any valid
 * array index.
 */
#define SITE_FROM_EID(eid)  (&db_rep->sites[eid])
#define EID_FROM_SITE(s)    ((int)((s) - (&db_rep->sites[0])))
#define IS_VALID_EID(e)     ((e) >= 0)
#define IS_KNOWN_REMOTE_SITE(e) ((e) >= 0 && ((u_int)(e)) < db_rep->site_cnt)
#define SELF_EID        INT_MAX

#define IS_SUBORDINATE(db_rep)  (db_rep->listen_fd == INVALID_SOCKET)

#define IS_PEER_POLICY(p) ((p) == DB_REPMGR_ACKS_ALL_PEERS ||       \
    (p) == DB_REPMGR_ACKS_QUORUM ||     \
    (p) == DB_REPMGR_ACKS_ONE_PEER)

/*
 * Most of the code in repmgr runs while holding repmgr's main mutex, which
 * resides in db_rep->mutex.  This mutex is owned by a single repmgr process,
 * and serializes access to the (large) critical sections among threads in the
 * process.  Unlike many other mutexes in DB, it is specifically coded as either
 * a POSIX threads mutex or a Win32 mutex.  Note that although it's a large
 * fraction of the code, it's a tiny fraction of the time: repmgr spends most of
 * its time in a call to select(), and as well a bit in calls into the Base
 * replication API.  All of those release the mutex.
 *     Access to repmgr's shared list of site addresses is protected by
 * another mutex: mtx_repmgr.  And, when changing space allocation for that site
 * list we conform to the convention of acquiring renv->mtx_regenv.  These are
 * less frequent of course.
 *     When it's necessary to acquire more than one of these mutexes, the
 * ordering priority is:
 *        db_rep->mutex (first)
 *        mtx_repmgr    (briefly)
 *        mtx_regenv    (last, and most briefly)
 */
#define LOCK_MUTEX(m) do {                      \
    int __ret;                          \
    if ((__ret = __repmgr_lock_mutex(m)) != 0)          \
        return (__ret);                     \
} while (0)

#define UNLOCK_MUTEX(m) do {                        \
    int __ret;                          \
    if ((__ret = __repmgr_unlock_mutex(m)) != 0)            \
        return (__ret);                     \
} while (0)

/* POSIX/Win32 socket (and other) portability. */
#ifdef DB_WIN32
#define WOULDBLOCK      WSAEWOULDBLOCK
#define INPROGRESS      WSAEWOULDBLOCK

#define net_errno       WSAGetLastError()
typedef int socklen_t;
typedef char * sockopt_t;

#define iov_len len
#define iov_base buf

typedef DWORD threadsync_timeout_t;

#define REPMGR_INITED(db_rep) (db_rep->waiters != NULL)
#else

#define INVALID_SOCKET      -1
#define SOCKET_ERROR        -1
#define WOULDBLOCK      EWOULDBLOCK
#define INPROGRESS      EINPROGRESS

#define net_errno       errno
typedef void * sockopt_t;

#define closesocket(fd)     close(fd)

typedef struct timespec threadsync_timeout_t;

#define REPMGR_INITED(db_rep) (db_rep->read_pipe >= 0)
#endif

/* Macros to proceed, as with a cursor, through the address_list: */
#define ADDR_LIST_CURRENT(na)   ((na)->current)
#define ADDR_LIST_FIRST(na) ((na)->current = (na)->address_list)
#define ADDR_LIST_NEXT(na)  ((na)->current = (na)->current->ai_next)
#define ADDR_LIST_INIT(na, al)  do {    \
    (na)->address_list = (al);  \
    ADDR_LIST_FIRST(na);        \
} while (0)

/*
 * Generic definition of some action to be performed on each connection, in the
 * form of a call-back function.
 */
typedef int (*CONNECTION_ACTION) __P((ENV *, REPMGR_CONNECTION *, void *));

#include "dbinc_auto/repmgr_ext.h"

#if defined(__cplusplus)
}
#endif
#endif /* !_DB_REPMGR_H_ */
