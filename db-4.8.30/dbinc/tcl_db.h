/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef _DB_TCL_DB_H_
#define _DB_TCL_DB_H_

#if defined(__cplusplus)
extern "C" {
#endif

#define MSG_SIZE 100        /* Message size */

enum INFOTYPE {
    I_DB, I_DBC, I_ENV, I_LOCK, I_LOGC, I_MP, I_NDBM, I_PG, I_SEQ, I_TXN};

#define MAX_ID      8   /* Maximum number of sub-id's we need */
#define DBTCL_PREP  64  /* Size of txn_recover preplist */

#define DBTCL_DBM   1
#define DBTCL_NDBM  2

#define DBTCL_GETCLOCK      0
#define DBTCL_GETLIMIT      1
#define DBTCL_GETREQ        2

#define DBTCL_MUT_ALIGN 0
#define DBTCL_MUT_INCR  1
#define DBTCL_MUT_MAX   2
#define DBTCL_MUT_TAS   3

/*
 * Why use a home grown package over the Tcl_Hash functions?
 *
 * We could have implemented the stuff below without maintaining our
 * own list manipulation, efficiently hashing it with the available
 * Tcl functions (Tcl_CreateHashEntry, Tcl_GetHashValue, etc).  I chose
 * not to do so for these reasons:
 *
 * We still need the information below.  Using the hashing only removes
 * us from needing the next/prev pointers.  We still need the structure
 * itself because we need more than one value associated with a widget.
 * We need to keep track of parent pointers for sub-widgets (like cursors)
 * so we can correctly close.  We need to keep track of individual widget's
 * id counters for any sub-widgets they may have.  We need to be able to
 * associate the name/client data outside the scope of the widget.
 *
 * So, is it better to use the hashing rather than
 * the linear list we have now?  I decided against it for the simple reason
 * that to access the structure would require two calls.  The first is
 * Tcl_FindHashEntry(table, key) and then, once we have the entry, we'd
 * have to do Tcl_GetHashValue(entry) to get the pointer of the structure.
 *
 * I believe the number of simultaneous DB widgets in existence at one time
 * is not going to be that large (more than several dozen) such that
 * linearly searching the list is not going to impact performance in a
 * noticeable way.  Should performance be impacted due to the size of the
 * info list, then perhaps it is time to revisit this decision.
 */
typedef struct dbtcl_info {
    LIST_ENTRY(dbtcl_info) entries;
    Tcl_Interp *i_interp;
    char *i_name;
    enum INFOTYPE i_type;
    union infop {
        DB *dbp;
        DBC *dbcp;
        DB_ENV *envp;
        DB_LOCK *lock;
        DB_LOGC *logc;
        DB_MPOOLFILE *mp;
        DB_TXN *txnp;
        void *anyp;
    } un;
    union data {
        int anydata;
        db_pgno_t pgno;
        u_int32_t lockid;
    } und;
    union data2 {
        int anydata;
        int pagesz;
        DB_COMPACT *c_data;
    } und2;
    DBT i_lockobj;
    FILE *i_err;
    char *i_errpfx;

    /* Callbacks--Tcl_Objs containing proc names */
    Tcl_Obj *i_compare;
    Tcl_Obj *i_dupcompare;
    Tcl_Obj *i_event;
    Tcl_Obj *i_hashproc;
    Tcl_Obj *i_isalive;
    Tcl_Obj *i_part_callback;
    Tcl_Obj *i_rep_send;
    Tcl_Obj *i_second_call;

    /* Environment ID for the i_rep_send callback. */
    Tcl_Obj *i_rep_eid;

    struct dbtcl_info *i_parent;
    int i_otherid[MAX_ID];
} DBTCL_INFO;

#define i_anyp un.anyp
#define i_dbp un.dbp
#define i_dbcp un.dbcp
#define i_envp un.envp
#define i_lock un.lock
#define i_logc un.logc
#define i_mp un.mp
#define i_pagep un.anyp
#define i_txnp un.txnp

#define i_data und.anydata
#define i_pgno und.pgno
#define i_locker und.lockid
#define i_data2 und2.anydata
#define i_pgsz und2.pagesz
#define i_cdata und2.c_data

#define i_envtxnid i_otherid[0]
#define i_envmpid i_otherid[1]
#define i_envlockid i_otherid[2]
#define i_envlogcid i_otherid[3]

#define i_mppgid  i_otherid[0]

#define i_dbdbcid i_otherid[0]

extern int __debug_on, __debug_print, __debug_stop, __debug_test;

typedef struct dbtcl_global {
    LIST_HEAD(infohead, dbtcl_info) g_infohead;
} DBTCL_GLOBAL;
#define __db_infohead __dbtcl_global.g_infohead

extern DBTCL_GLOBAL __dbtcl_global;

/*
 * Tcl_NewStringObj takes an "int" length argument, when the typical use is to
 * call it with a size_t length (for example, returned by strlen).  Tcl is in
 * the wrong, but that doesn't help us much -- cast the argument.
 */
#define NewStringObj(a, b)                      \
    Tcl_NewStringObj(a, (int)b)

#define NAME_TO_DB(name)    (DB *)_NameToPtr((name))
#define NAME_TO_DBC(name)   (DBC *)_NameToPtr((name))
#define NAME_TO_ENV(name)   (DB_ENV *)_NameToPtr((name))
#define NAME_TO_LOCK(name)  (DB_LOCK *)_NameToPtr((name))
#define NAME_TO_MP(name)    (DB_MPOOLFILE *)_NameToPtr((name))
#define NAME_TO_TXN(name)   (DB_TXN *)_NameToPtr((name))
#define NAME_TO_SEQUENCE(name)  (DB_SEQUENCE *)_NameToPtr((name))

/*
 * MAKE_STAT_LIST appends a {name value} pair to a result list that MUST be
 * called 'res' that is a Tcl_Obj * in the local function.  This macro also
 * assumes a label "error" to go to in the event of a Tcl error.  For stat
 * functions this will typically go before the "free" function to free the
 * stat structure returned by DB.
 */
#define MAKE_STAT_LIST(s, v) do {                   \
    result = _SetListElemInt(interp, res, (s), (long)(v));      \
    if (result != TCL_OK)                       \
        goto error;                     \
} while (0)

#define MAKE_WSTAT_LIST(s, v) do {                  \
    result = _SetListElemWideInt(interp, res, (s), (int64_t)(v));   \
    if (result != TCL_OK)                       \
        goto error;                     \
} while (0)

/*
 * MAKE_STAT_LSN appends a {name {LSNfile LSNoffset}} pair to a result list
 * that MUST be called 'res' that is a Tcl_Obj * in the local
 * function.  This macro also assumes a label "error" to go to
 * in the even of a Tcl error.  For stat functions this will
 * typically go before the "free" function to free the stat structure
 * returned by DB.
 */
#define MAKE_STAT_LSN(s, lsn) do {                  \
    myobjc = 2;                         \
    myobjv[0] = Tcl_NewLongObj((long)(lsn)->file);          \
    myobjv[1] = Tcl_NewLongObj((long)(lsn)->offset);        \
    lsnlist = Tcl_NewListObj(myobjc, myobjv);           \
    myobjc = 2;                         \
    myobjv[0] = Tcl_NewStringObj((s), (int)strlen(s));      \
    myobjv[1] = lsnlist;                        \
    thislist = Tcl_NewListObj(myobjc, myobjv);          \
    result = Tcl_ListObjAppendElement(interp, res, thislist);   \
    if (result != TCL_OK)                       \
        goto error;                     \
} while (0)

/*
 * MAKE_STAT_STRLIST appends a {name string} pair to a result list
 * that MUST be called 'res' that is a Tcl_Obj * in the local
 * function.  This macro also assumes a label "error" to go to
 * in the even of a Tcl error.  For stat functions this will
 * typically go before the "free" function to free the stat structure
 * returned by DB.
 */
#define MAKE_STAT_STRLIST(s,s1) do {                    \
    result = _SetListElem(interp, res, (s), (u_int32_t)strlen(s),   \
        (s1), (u_int32_t)strlen(s1));               \
    if (result != TCL_OK)                       \
        goto error;                     \
} while (0)

/*
 * MAKE_SITE_LIST appends a {eid host port status} tuple to a result list
 * that MUST be called 'res' that is a Tcl_Obj * in the local function.
 * This macro also assumes a label "error" to go to in the event of a Tcl
 * error.
 */
#define MAKE_SITE_LIST(e, h, p, s) do {                 \
    myobjc = 4;                         \
    myobjv[0] = Tcl_NewIntObj(e);                   \
    myobjv[1] = Tcl_NewStringObj((h), (int)strlen(h));      \
    myobjv[2] = Tcl_NewIntObj((int)p);              \
    myobjv[3] = Tcl_NewStringObj((s), (int)strlen(s));      \
    thislist = Tcl_NewListObj(myobjc, myobjv);          \
    result = Tcl_ListObjAppendElement(interp, res, thislist);   \
    if (result != TCL_OK)                       \
        goto error;                     \
} while (0)

/*
 * FLAG_CHECK checks that the given flag is not set yet.
 * If it is, it sets up an error message.
 */
#define FLAG_CHECK(flag) do {                       \
    if ((flag) != 0) {                      \
        Tcl_SetResult(interp,                   \
            " Only 1 policy can be specified.\n",       \
            TCL_STATIC);                    \
        result = TCL_ERROR;                 \
        break;                          \
    }                               \
} while (0)

/*
 * FLAG_CHECK2 checks that the given flag is not set yet or is
 * only set to the given allowed value.
 * If it is, it sets up an error message.
 */
#define FLAG_CHECK2(flag, val) do {                 \
    if (((flag) & ~(val)) != 0) {                   \
        Tcl_SetResult(interp,                   \
            " Only 1 policy can be specified.\n",       \
            TCL_STATIC);                    \
        result = TCL_ERROR;                 \
        break;                          \
    }                               \
} while (0)

/*
 * IS_HELP checks whether the arg we bombed on is -?, which is a help option.
 * If it is, we return TCL_OK (but leave the result set to whatever
 * Tcl_GetIndexFromObj says, which lists all the valid options.  Otherwise
 * return TCL_ERROR.
 */
#define IS_HELP(s)                      \
    (strcmp(Tcl_GetStringFromObj(s,NULL), "-?") == 0) ? TCL_OK : TCL_ERROR

#if defined(__cplusplus)
}
#endif

#include "dbinc_auto/tcl_ext.h"
#endif /* !_DB_TCL_DB_H_ */
