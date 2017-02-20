/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <db.h>
#include "rep_base.h"
#ifndef _SYS_QUEUE_H
/*
 * Some *BSD Unix variants include the Queue macros in their libraries and
 * these might already have been included.  In that case, it would be bad
 * to include them again.
 */
#include <dbinc/queue.h>		/* !!!: for the LIST_XXX macros. */
#endif

int machtab_add __P((machtab_t *, socket_t, u_int32_t, int, int *));
#ifdef DIAGNOSTIC
void machtab_print __P((machtab_t *));
#endif
ssize_t readn __P((socket_t, void *, size_t));

/*
 * This file defines the communication infrastructure for the ex_repquote
 * sample application.
 *
 * This application uses TCP/IP for its communication.  In an N-site
 * replication group, this means that there are N * N communication
 * channels so that every site can communicate with every other site
 * (this allows elections to be held when the master fails).  We do
 * not require that anyone know about all sites when the application
 * starts up.  In order to communicate, the application should know
 * about someone, else it has no idea how to ever get in the game.
 *
 * Communication is handled via a number of different threads.  These
 * thread functions are implemented in rep_util.c  In this file, we
 * define the data structures that maintain the state that describes
 * the comm infrastructure, the functions that manipulates this state
 * and the routines used to actually send and receive data over the
 * sockets.
 */

/*
 * The communication infrastructure is represented by a machine table,
 * machtab_t, which is essentially a mutex-protected linked list of members
 * of the group.  The machtab also contains the parameters that are needed
 * to call for an election.  We hardwire values for these parameters in the
 * init function, but these could be set via some configuration setup in a
 * real application.  We reserve the machine-id 1 to refer to ourselves and
 * make the machine-id 0 be invalid.
 */

#define	MACHID_INVALID	0
#define	MACHID_SELF	1

struct __machtab {
	LIST_HEAD(__machlist, __member) machlist;
	int nextid;
	mutex_t mtmutex;
	u_int32_t timeout_time;
	int current;
	int max;
	int nsites;
};

/* Data structure that describes each entry in the machtab. */
struct __member {
	u_int32_t hostaddr;	/* Host IP address. */
	int port;		/* Port number. */
	int eid;		/* Application-specific machine id. */
	socket_t fd;		/* File descriptor for the socket. */
	LIST_ENTRY(__member) links;
				/* For linked list of all members we know of. */
};

static int quote_send_broadcast __P((machtab_t *,
    const DBT *, const DBT *, u_int32_t));
static int quote_send_one __P((const DBT *, const DBT *, socket_t, u_int32_t));

/*
 * machtab_init --
 *	Initialize the machine ID table.
 * XXX Right now we treat the number of sites as the maximum
 * number we've ever had on the list at one time.  We probably
 * want to make that smarter.
 */
int
machtab_init(machtabp, nsites)
	machtab_t **machtabp;
	int nsites;
{
	int ret;
	machtab_t *machtab;

	if ((machtab = malloc(sizeof(machtab_t))) == NULL) {
		fprintf(stderr, "can't allocate memory\n");
		return (ENOMEM);
	}

	LIST_INIT(&machtab->machlist);

	/* Reserve eid's 0 and 1. */
	machtab->nextid = 2;
	machtab->timeout_time = 2 * 1000000;		/* 2 seconds. */
	machtab->current = machtab->max = 0;
	machtab->nsites = nsites;

	ret = mutex_init(&machtab->mtmutex, NULL);
	*machtabp = machtab;

	return (ret);
}

/*
 * machtab_add --
 *	Add a file descriptor to the table of machines, returning
 *  a new machine ID.
 */
int
machtab_add(machtab, fd, hostaddr, port, idp)
	machtab_t *machtab;
	socket_t fd;
	u_int32_t hostaddr;
	int port, *idp;
{
	int ret;
	member_t *m, *member;

	ret = 0;
	if ((member = malloc(sizeof(member_t))) == NULL) {
		fprintf(stderr, "can't allocate memory\n");
		return (ENOMEM);
	}

	member->fd = fd;
	member->hostaddr = hostaddr;
	member->port = port;

	if ((ret = mutex_lock(&machtab->mtmutex)) != 0) {
		fprintf(stderr, "can't lock mutex");
		return (ret);
	}

	for (m = LIST_FIRST(&machtab->machlist);
	    m != NULL; m = LIST_NEXT(m, links))
		if (m->hostaddr == hostaddr && m->port == port)
			break;

	if (m == NULL) {
		member->eid = machtab->nextid++;
		LIST_INSERT_HEAD(&machtab->machlist, member, links);
	} else
		member->eid = m->eid;

	if ((ret = mutex_unlock(&machtab->mtmutex)) != 0) {
		fprintf(stderr, "can't unlock mutex\n");
		return (ret);
	}

	if (idp != NULL)
		*idp = member->eid;

	if (m == NULL) {
		if (++machtab->current > machtab->max)
			machtab->max = machtab->current;
	} else {
		free(member);
		ret = EEXIST;
	}
#ifdef DIAGNOSTIC
	printf("Exiting machtab_add\n");
	machtab_print(machtab);
#endif
	return (ret);
}

/*
 * machtab_getinfo --
 *	Return host and port information for a particular machine id.
 */
int
machtab_getinfo(machtab, eid, hostp, portp)
	machtab_t *machtab;
	int eid;
	u_int32_t *hostp;
	int *portp;
{
	int ret;
	member_t *member;

	if ((ret = mutex_lock(&machtab->mtmutex)) != 0) {
		fprintf(stderr, "can't lock mutex\n");
		return (ret);
	}

	for (member = LIST_FIRST(&machtab->machlist);
	    member != NULL;
	    member = LIST_NEXT(member, links))
		if (member->eid == eid) {
			*hostp = member->hostaddr;
			*portp = member->port;
			break;
		}

	if ((ret = mutex_unlock(&machtab->mtmutex)) != 0) {
		fprintf(stderr, "can't unlock mutex\n");
		return (ret);
	}

	return (member != NULL ? 0 : EINVAL);
}

/*
 * machtab_rem --
 *	Remove a mapping from the table of machines.  Lock indicates
 * whether we need to lock the machtab or not (0 indicates we do not
 * need to lock; non-zero indicates that we do need to lock).
 */
int
machtab_rem(machtab, eid, lock)
	machtab_t *machtab;
	int eid;
	int lock;
{
	int found, ret;
	member_t *member;

	ret = 0;
	if (lock && (ret = mutex_lock(&machtab->mtmutex)) != 0) {
		fprintf(stderr, "can't lock mutex\n");
		return (ret);
	}

	for (found = 0, member = LIST_FIRST(&machtab->machlist);
	    member != NULL;
	    member = LIST_NEXT(member, links))
		if (member->eid == eid) {
			found = 1;
			LIST_REMOVE(member, links);
			(void)closesocket(member->fd);
			free(member);
			machtab->current--;
			break;
		}

	if (LIST_FIRST(&machtab->machlist) == NULL)
		machtab->nextid = 2;

	if (lock && (ret = mutex_unlock(&machtab->mtmutex)) != 0)
		fprintf(stderr, "can't unlock mutex\n");

#ifdef DIAGNOSTIC
	printf("Exiting machtab_rem\n");
	machtab_print(machtab);
#endif
	return (ret);
}

void
machtab_parm(machtab, nump, timeoutp)
	machtab_t *machtab;
	int *nump;
	u_int32_t *timeoutp;
{
	if (machtab->nsites == 0)
		*nump = machtab->max;
	else
		*nump = machtab->nsites;
	*timeoutp = machtab->timeout_time;
}

#ifdef DIAGNOSTIC
void
machtab_print(machtab)
	machtab_t *machtab;
{
	member_t *m;

	if (mutex_lock(&machtab->mtmutex) != 0) {
		fprintf(stderr, "can't lock mutex\n");
		abort();
	}

	for (m = LIST_FIRST(&machtab->machlist);
	    m != NULL; m = LIST_NEXT(m, links)) {

	    printf("IP: %lx Port: %6d EID: %2d FD: %3d\n",
		(long)m->hostaddr, m->port, m->eid, m->fd);
	}

	if (mutex_unlock(&machtab->mtmutex) != 0) {
		fprintf(stderr, "can't unlock mutex\n");
		abort();
	}
}
#endif
/*
 * listen_socket_init --
 *	Initialize a socket for listening on the specified port.  Returns
 *	a file descriptor for the socket, ready for an accept() call
 *	in a thread that we're happy to let block.
 */
socket_t
listen_socket_init(progname, port)
	const char *progname;
	int port;
{
	socket_t s;
	int sockopt;
	struct sockaddr_in si;

	COMPQUIET(progname, NULL);

	if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("can't create listen socket");
		return (-1);
	}

	memset(&si, 0, sizeof(si));
	si.sin_family = AF_INET;
	si.sin_addr.s_addr = htonl(INADDR_ANY);
	si.sin_port = htons((unsigned short)port);

	/*
	 * When using this example for testing, it's common to kill and restart
	 * regularly.  On some systems, this causes bind to fail with "address
	 * in use" errors unless this option is set.
	 */
	sockopt = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
	    (const char *)&sockopt, sizeof(sockopt));

	if (bind(s, (struct sockaddr *)&si, sizeof(si)) != 0) {
		perror("can't bind listen socket");
		goto err;
	}

	if (listen(s, 5) != 0) {
		perror("can't establish listen queue");
		goto err;
	}

	return (s);

err:	closesocket(s);
	return (-1);
}

/*
 * listen_socket_accept --
 *	Accept a connection on a socket.  This is essentially just a wrapper
 *	for accept(3).
 */
socket_t
listen_socket_accept(machtab, progname, s, eidp)
	machtab_t *machtab;
	const char *progname;
	socket_t s;
	int *eidp;
{
	struct sockaddr_in si;
	socklen_t si_len;
	int host, ret;
	socket_t ns;
	u_int16_t port;

	COMPQUIET(progname, NULL);

accept_wait:
	memset(&si, 0, sizeof(si));
	si_len = sizeof(si);
	ns = accept(s, (struct sockaddr *)&si, &si_len);
	if (ns == SOCKET_CREATION_FAILURE) {
		fprintf(stderr, "can't accept incoming connection\n");
		return ns;
	}
	host = ntohl(si.sin_addr.s_addr);

	/*
	 * Sites send their listening port when connections are first
	 * established, as it will be different from the outgoing port
	 * for this connection.
	 */
	if (readn(ns, &port, 2) != 2)
		goto err;
	port = ntohs(port);

	ret = machtab_add(machtab, ns, host, port, eidp);
	if (ret == EEXIST) {
		closesocket(ns);
		goto accept_wait;
	} else if (ret != 0)
		goto err;
	printf("Connected to host %x port %d, eid = %d\n", host, port, *eidp);
	return (ns);

err:	closesocket(ns);
	return SOCKET_CREATION_FAILURE;
}

/*
 * get_connected_socket --
 *	Connect to the specified port of the specified remote machine,
 *	and return a file descriptor when we have accepted a connection on it.
 *	Add this connection to the machtab.  If we already have a connection
 *	open to this machine, then don't create another one, return the eid
 *	of the connection (in *eidp) and set is_open to 1.  Return 0.
 */
socket_t
get_connected_socket(machtab, progname, remotehost, port, is_open, eidp)
	machtab_t *machtab;
	const char *progname, *remotehost;
	int port, *is_open, *eidp;
{
	int ret;
	socket_t s;
	struct hostent *hp;
	struct sockaddr_in si;
	u_int32_t addr;
	u_int16_t nport;

	*is_open = 0;

	if ((hp = gethostbyname(remotehost)) == NULL) {
		fprintf(stderr, "%s: host not found: %s\n", progname,
		    strerror(net_errno));
		return (-1);
	}

	if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("can't create outgoing socket");
		return (-1);
	}
	memset(&si, 0, sizeof(si));
	memcpy((char *)&si.sin_addr, hp->h_addr, hp->h_length);
	addr = ntohl(si.sin_addr.s_addr);
	ret = machtab_add(machtab, s, addr, port, eidp);
	if (ret == EEXIST) {
		*is_open = 1;
		closesocket(s);
		return (0);
	} else if (ret != 0) {
		closesocket(s);
		return (-1);
	}

	si.sin_family = AF_INET;
	si.sin_port = htons((unsigned short)port);
	if (connect(s, (struct sockaddr *)&si, sizeof(si)) < 0) {
		fprintf(stderr, "%s: connection failed: %s\n",
		    progname, strerror(net_errno));
		(void)machtab_rem(machtab, *eidp, 1);
		return (-1);
	}

	/*
	 * The first thing we send on the socket is our (listening) port
	 * so the site we are connecting to can register us correctly in
	 * its machtab.
	 */
	nport = htons(myport);
	writesocket(s, &nport, 2);

	return (s);
}

/*
 * get_next_message --
 *	Read a single message from the specified file descriptor, and
 * return it in the format used by rep functions (two DBTs and a type).
 *
 * This function is called in a loop by both clients and masters, and
 * the resulting DBTs are manually dispatched to DB_ENV->rep_process_message().
 */
int
get_next_message(fd, rec, control)
	socket_t fd;
	DBT *rec, *control;
{
	size_t nr;
	u_int32_t rsize, csize;
	u_int8_t *recbuf, *controlbuf;

	/*
	 * The protocol we use on the wire is dead simple:
	 *
	 *	4 bytes		- rec->size
	 *	(# read above)	- rec->data
	 *	4 bytes		- control->size
	 *	(# read above)	- control->data
	 */

	/* Read rec->size. */
	nr = readn(fd, &rsize, 4);
	if (nr != 4)
		return (1);

	/* Read the record itself. */
	if (rsize > 0) {
		if (rec->size < rsize)
			rec->data = realloc(rec->data, rsize);
		if ((recbuf = rec->data) == NULL)
			return (1);
		nr = readn(fd, recbuf, rsize);
	} else {
		if (rec->data != NULL)
			free(rec->data);
		rec->data = NULL;
	}
	rec->size = rsize;

	/* Read control->size. */
	nr = readn(fd, &csize, 4);
	if (nr != 4)
		return (1);

	/* Read the control struct itself. */
	if (csize > 0) {
		controlbuf = control->data;
		if (control->size < csize)
			controlbuf = realloc(controlbuf, csize);
		if (controlbuf == NULL)
			return (1);
		nr = readn(fd, controlbuf, csize);
		if (nr != csize)
			return (1);
	} else {
		if (control->data != NULL)
			free(control->data);
		controlbuf = NULL;
	}
	control->data = controlbuf;
	control->size = csize;

	return (0);
}

/*
 * readn --
 *     Read a full n characters from a file descriptor, unless we get an error
 * or EOF.
 */
ssize_t
readn(fd, vptr, n)
	socket_t fd;
	void *vptr;
	size_t n;
{
	size_t nleft;
	ssize_t nread;
	char *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ((nread = readsocket(fd, ptr, nleft)) < 0) {
			/*
			 * Call read() again on interrupted system call;
			 * on other errors, bail.
			 */
			if (net_errno == EINTR)
				nread = 0;
			else {
				perror("can't read from socket");
				return (-1);
			}
		} else if (nread == 0)
			break;  /* EOF */

		nleft -= nread;
		ptr   += nread;
	}

	return (n - nleft);
}

/*
 * quote_send --
 * The f_send function for DB_ENV->set_rep_transport.
 */
int
quote_send(dbenv, control, rec, lsnp, eid, flags)
	DB_ENV *dbenv;
	const DBT *control, *rec;
	const DB_LSN *lsnp;
	int eid;
	u_int32_t flags;
{
	int n, ret, t_ret;
	socket_t fd;
	machtab_t *machtab;
	member_t *m;

	COMPQUIET(lsnp, NULL);
	machtab =
	    (machtab_t *)((APP_DATA*)dbenv->app_private)->comm_infrastructure;

	if (eid == DB_EID_BROADCAST) {
		/*
		 * Right now, we do not require successful transmission.
		 * I'd like to move this requiring at least one successful
		 * transmission on PERMANENT requests.
		 */
		n = quote_send_broadcast(machtab, rec, control, flags);
		if (n < 0 /*|| (n == 0 && LF_ISSET(DB_REP_PERMANENT))*/)
			return (DB_REP_UNAVAIL);
		return (0);
	}

	if ((ret = mutex_lock(&machtab->mtmutex)) != 0) {
		dbenv->errx(dbenv, "can't lock mutex");
		return (ret);
	}

	fd = 0;
	for (m = LIST_FIRST(&machtab->machlist); m != NULL;
	    m = LIST_NEXT(m, links)) {
		if (m->eid == eid) {
			fd = m->fd;
			break;
		}
	}

	if (fd == 0) {
		dbenv->err(dbenv, DB_REP_UNAVAIL,
		    "quote_send: cannot find machine ID %d", eid);
		return (DB_REP_UNAVAIL);
	}

	if ((ret = quote_send_one(rec, control, fd, flags)) != 0)
		fprintf(stderr, "socket write error in send() function\n");

	if ((t_ret = mutex_unlock(&machtab->mtmutex)) != 0) {
		dbenv->errx(dbenv, "can't unlock mutex");
		if (ret == 0)
			ret = t_ret;
	}

	return (ret);
}

/*
 * quote_send_broadcast --
 *	Send a message to everybody.
 * Returns the number of sites to which this message was successfully
 * communicated.  A -1 indicates a fatal error.
 */
static int
quote_send_broadcast(machtab, rec, control, flags)
	machtab_t *machtab;
	const DBT *rec, *control;
	u_int32_t flags;
{
	int ret, sent;
	member_t *m, *next;

	if ((ret = mutex_lock(&machtab->mtmutex)) != 0) {
		fprintf(stderr, "can't lock mutex\n");
		return (ret);
	}

	sent = 0;
	for (m = LIST_FIRST(&machtab->machlist); m != NULL; m = next) {
		next = LIST_NEXT(m, links);
		if ((ret = quote_send_one(rec, control, m->fd, flags)) != 0) {
			fprintf(stderr, "socket write error in broadcast\n");
			(void)machtab_rem(machtab, m->eid, 0);
		} else
			sent++;
	}

	if (mutex_unlock(&machtab->mtmutex) != 0) {
		fprintf(stderr, "can't unlock mutex\n");
		return (-1);
	}

	return (sent);
}

/*
 * quote_send_one --
 *	Send a message to a single machine, given that machine's file
 * descriptor.
 *
 * !!!
 * Note that the machtab mutex should be held through this call.
 * It doubles as a synchronizer to make sure that two threads don't
 * intersperse writes that are part of two single messages.
 */
static int
quote_send_one(rec, control, fd, flags)
	const DBT *rec, *control;
	socket_t fd;
	u_int32_t flags;

{
	int retry;
	ssize_t bytes_left, nw;
	u_int8_t *wp;

	COMPQUIET(flags, 0);

	/*
	 * The protocol is simply: write rec->size, write rec->data,
	 * write control->size, write control->data.
	 */
	nw = writesocket(fd, (const char *)&rec->size, 4);
	if (nw != 4)
		return (DB_REP_UNAVAIL);

	if (rec->size > 0) {
		nw = writesocket(fd, rec->data, rec->size);
		if (nw < 0)
			return (DB_REP_UNAVAIL);
		if (nw != (ssize_t)rec->size) {
			/* Try a couple of times to finish the write. */
			wp = (u_int8_t *)rec->data + nw;
			bytes_left = rec->size - nw;
			for (retry = 0; bytes_left > 0 && retry < 3; retry++) {
				nw = writesocket(fd, wp, bytes_left);
				if (nw < 0)
					return (DB_REP_UNAVAIL);
				bytes_left -= nw;
				wp += nw;
			}
			if (bytes_left > 0)
				return (DB_REP_UNAVAIL);
		}
	}

	nw = writesocket(fd, (const char *)&control->size, 4);
	if (nw != 4)
		return (DB_REP_UNAVAIL);
	if (control->size > 0) {
		nw = writesocket(fd, control->data, control->size);
		if (nw != (ssize_t)control->size)
			return (DB_REP_UNAVAIL);
	}
	return (0);
}
