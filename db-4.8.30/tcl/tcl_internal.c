/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#ifdef HAVE_SYSTEM_INCLUDE_FILES
#include <tcl.h>
#endif
#include "dbinc/tcl_db.h"
#include "dbinc/db_page.h"
#include "dbinc/db_am.h"

/*
 *
 * internal.c --
 *
 *	This file contains internal functions we need to maintain
 *	state for our Tcl interface.
 *
 *	NOTE: This all uses a linear linked list.  If we end up with
 *	too many info structs such that this is a performance hit, it
 *	should be redone using hashes or a list per type.  The assumption
 *	is that the user won't have more than a few dozen info structs
 *	in operation at any given point in time.  Even a complicated
 *	application with a few environments, nested transactions, locking,
 *	and several databases open, using cursors should not have a
 *	negative performance impact, in terms of searching the list to
 *	get/manipulate the info structure.
 */

#define	GLOB_CHAR(c)	((c) == '*' || (c) == '?')

/*
 * PUBLIC: DBTCL_INFO *_NewInfo __P((Tcl_Interp *,
 * PUBLIC:    void *, char *, enum INFOTYPE));
 *
 * _NewInfo --
 *
 * This function will create a new info structure and fill it in
 * with the name and pointer, id and type.
 */
DBTCL_INFO *
_NewInfo(interp, anyp, name, type)
	Tcl_Interp *interp;
	void *anyp;
	char *name;
	enum INFOTYPE type;
{
	DBTCL_INFO *p;
	int ret;

	if ((ret = __os_calloc(NULL, sizeof(DBTCL_INFO), 1, &p)) != 0) {
		Tcl_SetResult(interp, db_strerror(ret), TCL_STATIC);
		return (NULL);
	}

	if ((ret = __os_strdup(NULL, name, &p->i_name)) != 0) {
		Tcl_SetResult(interp, db_strerror(ret), TCL_STATIC);
		__os_free(NULL, p);
		return (NULL);
	}
	p->i_interp = interp;
	p->i_anyp = anyp;
	p->i_type = type;

	LIST_INSERT_HEAD(&__db_infohead, p, entries);
	return (p);
}

/*
 * PUBLIC: void *_NameToPtr __P((CONST char *));
 */
void	*
_NameToPtr(name)
	CONST char *name;
{
	DBTCL_INFO *p;

	LIST_FOREACH(p, &__db_infohead, entries)
		if (strcmp(name, p->i_name) == 0)
			return (p->i_anyp);
	return (NULL);
}

/*
 * PUBLIC: DBTCL_INFO *_PtrToInfo __P((CONST void *));
 */
DBTCL_INFO *
_PtrToInfo(ptr)
	CONST void *ptr;
{
	DBTCL_INFO *p;

	LIST_FOREACH(p, &__db_infohead, entries)
		if (p->i_anyp == ptr)
			return (p);
	return (NULL);
}

/*
 * PUBLIC: DBTCL_INFO *_NameToInfo __P((CONST char *));
 */
DBTCL_INFO *
_NameToInfo(name)
	CONST char *name;
{
	DBTCL_INFO *p;

	LIST_FOREACH(p, &__db_infohead, entries)
		if (strcmp(name, p->i_name) == 0)
			return (p);
	return (NULL);
}

/*
 * PUBLIC: void  _SetInfoData __P((DBTCL_INFO *, void *));
 */
void
_SetInfoData(p, data)
	DBTCL_INFO *p;
	void *data;
{
	if (p == NULL)
		return;
	p->i_anyp = data;
	return;
}

/*
 * PUBLIC: void  _DeleteInfo __P((DBTCL_INFO *));
 */
void
_DeleteInfo(p)
	DBTCL_INFO *p;
{
	if (p == NULL)
		return;
	LIST_REMOVE(p, entries);
	if (p->i_lockobj.data != NULL)
		__os_free(NULL, p->i_lockobj.data);
	if (p->i_err != NULL && p->i_err != stderr && p->i_err != stdout) {
		(void)fclose(p->i_err);
		p->i_err = NULL;
	}
	if (p->i_errpfx != NULL)
		__os_free(NULL, p->i_errpfx);
	if (p->i_compare != NULL) {
		Tcl_DecrRefCount(p->i_compare);
	}
	if (p->i_dupcompare != NULL) {
		Tcl_DecrRefCount(p->i_dupcompare);
	}
	if (p->i_hashproc != NULL) {
		Tcl_DecrRefCount(p->i_hashproc);
	}
	if (p->i_part_callback != NULL) {
		Tcl_DecrRefCount(p->i_part_callback);
	}
	if (p->i_second_call != NULL) {
		Tcl_DecrRefCount(p->i_second_call);
	}
	if (p->i_rep_eid != NULL) {
		Tcl_DecrRefCount(p->i_rep_eid);
	}
	if (p->i_rep_send != NULL) {
		Tcl_DecrRefCount(p->i_rep_send);
	}
	if (p->i_event != NULL) {
		Tcl_DecrRefCount(p->i_event);
	}
	__os_free(NULL, p->i_name);
	__os_free(NULL, p);

	return;
}

/*
 * PUBLIC: int _SetListElem __P((Tcl_Interp *,
 * PUBLIC:    Tcl_Obj *, void *, u_int32_t, void *, u_int32_t));
 */
int
_SetListElem(interp, list, elem1, e1cnt, elem2, e2cnt)
	Tcl_Interp *interp;
	Tcl_Obj *list;
	void *elem1, *elem2;
	u_int32_t e1cnt, e2cnt;
{
	Tcl_Obj *myobjv[2], *thislist;
	int myobjc;

	myobjc = 2;
	myobjv[0] = Tcl_NewByteArrayObj((u_char *)elem1, (int)e1cnt);
	myobjv[1] = Tcl_NewByteArrayObj((u_char *)elem2, (int)e2cnt);
	thislist = Tcl_NewListObj(myobjc, myobjv);
	if (thislist == NULL)
		return (TCL_ERROR);
	return (Tcl_ListObjAppendElement(interp, list, thislist));

}

/*
 * PUBLIC: int _SetListElemInt __P((Tcl_Interp *, Tcl_Obj *, void *, long));
 */
int
_SetListElemInt(interp, list, elem1, elem2)
	Tcl_Interp *interp;
	Tcl_Obj *list;
	void *elem1;
	long elem2;
{
	Tcl_Obj *myobjv[2], *thislist;
	int myobjc;

	myobjc = 2;
	myobjv[0] =
	    Tcl_NewByteArrayObj((u_char *)elem1, (int)strlen((char *)elem1));
	myobjv[1] = Tcl_NewLongObj(elem2);
	thislist = Tcl_NewListObj(myobjc, myobjv);
	if (thislist == NULL)
		return (TCL_ERROR);
	return (Tcl_ListObjAppendElement(interp, list, thislist));
}

/*
 * Don't compile this code if we don't have sequences compiled into the DB
 * library, it's likely because we don't have a 64-bit type, and trying to
 * use int64_t is going to result in syntax errors.
 */
#ifdef HAVE_64BIT_TYPES
/*
 * PUBLIC: int _SetListElemWideInt __P((Tcl_Interp *,
 * PUBLIC:     Tcl_Obj *, void *, int64_t));
 */
int
_SetListElemWideInt(interp, list, elem1, elem2)
	Tcl_Interp *interp;
	Tcl_Obj *list;
	void *elem1;
	int64_t elem2;
{
	Tcl_Obj *myobjv[2], *thislist;
	int myobjc;

	myobjc = 2;
	myobjv[0] =
	    Tcl_NewByteArrayObj((u_char *)elem1, (int)strlen((char *)elem1));
	myobjv[1] = Tcl_NewWideIntObj(elem2);
	thislist = Tcl_NewListObj(myobjc, myobjv);
	if (thislist == NULL)
		return (TCL_ERROR);
	return (Tcl_ListObjAppendElement(interp, list, thislist));
}
#endif /* HAVE_64BIT_TYPES */

/*
 * PUBLIC: int _SetListRecnoElem __P((Tcl_Interp *, Tcl_Obj *,
 * PUBLIC:     db_recno_t, u_char *, u_int32_t));
 */
int
_SetListRecnoElem(interp, list, elem1, elem2, e2size)
	Tcl_Interp *interp;
	Tcl_Obj *list;
	db_recno_t elem1;
	u_char *elem2;
	u_int32_t e2size;
{
	Tcl_Obj *myobjv[2], *thislist;
	int myobjc;

	myobjc = 2;
	myobjv[0] = Tcl_NewWideIntObj((Tcl_WideInt)elem1);
	myobjv[1] = Tcl_NewByteArrayObj(elem2, (int)e2size);
	thislist = Tcl_NewListObj(myobjc, myobjv);
	if (thislist == NULL)
		return (TCL_ERROR);
	return (Tcl_ListObjAppendElement(interp, list, thislist));

}

/*
 * _Set3DBTList --
 *	This is really analogous to both _SetListElem and
 *	_SetListRecnoElem--it's used for three-DBT lists returned by
 *	DB->pget and DBC->pget().  We'd need a family of four functions
 *	to handle all the recno/non-recno cases, however, so we make
 *	this a little more aware of the internals and do the logic inside.
 *
 *	XXX
 *	One of these days all these functions should probably be cleaned up
 *	to eliminate redundancy and bring them into the standard DB
 *	function namespace.
 *
 * PUBLIC: int _Set3DBTList __P((Tcl_Interp *, Tcl_Obj *, DBT *, int,
 * PUBLIC:     DBT *, int, DBT *));
 */
int
_Set3DBTList(interp, list, elem1, is1recno, elem2, is2recno, elem3)
	Tcl_Interp *interp;
	Tcl_Obj *list;
	DBT *elem1, *elem2, *elem3;
	int is1recno, is2recno;
{

	Tcl_Obj *myobjv[3], *thislist;

	if (is1recno)
		myobjv[0] = Tcl_NewWideIntObj(
		    (Tcl_WideInt)*(db_recno_t *)elem1->data);
	else
		myobjv[0] = Tcl_NewByteArrayObj(
		    (u_char *)elem1->data, (int)elem1->size);

	if (is2recno)
		myobjv[1] = Tcl_NewWideIntObj(
		    (Tcl_WideInt)*(db_recno_t *)elem2->data);
	else
		myobjv[1] = Tcl_NewByteArrayObj(
		    (u_char *)elem2->data, (int)elem2->size);

	myobjv[2] = Tcl_NewByteArrayObj(
	    (u_char *)elem3->data, (int)elem3->size);

	thislist = Tcl_NewListObj(3, myobjv);

	if (thislist == NULL)
		return (TCL_ERROR);
	return (Tcl_ListObjAppendElement(interp, list, thislist));
}

/*
 * _SetMultiList -- build a list for return from multiple get.
 *
 * PUBLIC: int _SetMultiList __P((Tcl_Interp *,
 * PUBLIC:	    Tcl_Obj *, DBT *, DBT*, DBTYPE, u_int32_t));
 */
int
_SetMultiList(interp, list, key, data, type, flag)
	Tcl_Interp *interp;
	Tcl_Obj *list;
	DBT *key, *data;
	DBTYPE type;
	u_int32_t flag;
{
	db_recno_t recno;
	u_int32_t dlen, klen;
	int result;
	void *pointer, *dp, *kp;

	recno = 0;
	dlen = 0;
	kp = NULL;

	DB_MULTIPLE_INIT(pointer, data);
	result = TCL_OK;

	if (type == DB_RECNO || type == DB_QUEUE)
		recno = *(db_recno_t *) key->data;
	else
		kp = key->data;
	klen = key->size;
	do {
		if (flag & DB_MULTIPLE_KEY) {
			if (type == DB_RECNO || type == DB_QUEUE)
				DB_MULTIPLE_RECNO_NEXT(pointer,
				    data, recno, dp, dlen);
			else
				DB_MULTIPLE_KEY_NEXT(pointer,
				    data, kp, klen, dp, dlen);
		} else
			DB_MULTIPLE_NEXT(pointer, data, dp, dlen);

		if (pointer == NULL)
			break;

		if (type == DB_RECNO || type == DB_QUEUE) {
			result =
			    _SetListRecnoElem(interp, list, recno, dp, dlen);
			recno++;
			/* Wrap around and skip zero. */
			if (recno == 0)
				recno++;
		} else
			result = _SetListElem(interp, list, kp, klen, dp, dlen);
	} while (result == TCL_OK);

	return (result);
}
/*
 * PUBLIC: int _GetGlobPrefix __P((char *, char **));
 */
int
_GetGlobPrefix(pattern, prefix)
	char *pattern;
	char **prefix;
{
	int i, j;
	char *p;

	/*
	 * Duplicate it, we get enough space and most of the work is done.
	 */
	if (__os_strdup(NULL, pattern, prefix) != 0)
		return (1);

	p = *prefix;
	for (i = 0, j = 0; p[i] && !GLOB_CHAR(p[i]); i++, j++)
		/*
		 * Check for an escaped character and adjust
		 */
		if (p[i] == '\\' && p[i+1]) {
			p[j] = p[i+1];
			i++;
		} else
			p[j] = p[i];
	p[j] = 0;
	return (0);
}

/*
 * PUBLIC: int _ReturnSetup __P((Tcl_Interp *, int, int, char *));
 */
int
_ReturnSetup(interp, ret, ok, errmsg)
	Tcl_Interp *interp;
	int ret, ok;
	char *errmsg;
{
	char *msg;

	if (ret > 0)
		return (_ErrorSetup(interp, ret, errmsg));

	/*
	 * We either have success or a DB error.  If a DB error, set up the
	 * string.  We return an error if not one of the errors we catch.
	 * If anyone wants to reset the result to return anything different,
	 * then the calling function is responsible for doing so via
	 * Tcl_ResetResult or another Tcl_SetObjResult.
	 */
	if (ret == 0) {
		Tcl_SetResult(interp, "0", TCL_STATIC);
		return (TCL_OK);
	}

	msg = db_strerror(ret);
	Tcl_AppendResult(interp, msg, NULL);

	if (ok)
		return (TCL_OK);
	else {
		Tcl_SetErrorCode(interp, "BerkeleyDB", msg, NULL);
		return (TCL_ERROR);
	}
}

/*
 * PUBLIC: int _ErrorSetup __P((Tcl_Interp *, int, char *));
 */
int
_ErrorSetup(interp, ret, errmsg)
	Tcl_Interp *interp;
	int ret;
	char *errmsg;
{
	Tcl_SetErrno(ret);
	Tcl_AppendResult(interp, errmsg, ":", Tcl_PosixError(interp), NULL);
	return (TCL_ERROR);
}

/*
 * PUBLIC: void _ErrorFunc __P((const DB_ENV *, CONST char *, const char *));
 */
void
_ErrorFunc(dbenv, pfx, msg)
	const DB_ENV *dbenv;
	CONST char *pfx;
	const char *msg;
{
	DBTCL_INFO *p;
	Tcl_Interp *interp;
	size_t size;
	char *err;

	COMPQUIET(dbenv, NULL);

	p = _NameToInfo(pfx);
	if (p == NULL)
		return;
	interp = p->i_interp;

	size = strlen(pfx) + strlen(msg) + 4;
	/*
	 * If we cannot allocate enough to put together the prefix
	 * and message then give them just the message.
	 */
	if (__os_malloc(NULL, size, &err) != 0) {
		Tcl_AddErrorInfo(interp, msg);
		Tcl_AppendResult(interp, msg, "\n", NULL);
		return;
	}
	snprintf(err, size, "%s: %s", pfx, msg);
	Tcl_AddErrorInfo(interp, err);
	Tcl_AppendResult(interp, err, "\n", NULL);
	__os_free(NULL, err);
	return;
}

/*
 * PUBLIC: void _EventFunc __P((DB_ENV *, u_int32_t, void *));
 */
void
_EventFunc(dbenv, event, info)
	DB_ENV *dbenv;
	u_int32_t event;
	void *info;
{
#define	TCLDB_EVENTITEMS 2	/* Event name and any info */
#define	TCLDB_SENDEVENT 3	/* Event Tcl proc, env name, event objects. */
	DBTCL_INFO *ip;
	Tcl_Interp *interp;
	Tcl_Obj *event_o, *origobj;
	Tcl_Obj *myobjv[TCLDB_EVENTITEMS], *objv[TCLDB_SENDEVENT];
	int i, myobjc, result;

	ip = (DBTCL_INFO *)dbenv->app_private;
	interp = ip->i_interp;
	if (ip->i_event == NULL)
		return;
	objv[0] = ip->i_event;
	objv[1] = NewStringObj(ip->i_name, strlen(ip->i_name));

	/*
	 * Most events don't have additional info.  Assume none
	 * and handle individually those that do.
	 */
	myobjv[1] = NULL;
	myobjc = 1;
	switch (event) {
	case DB_EVENT_PANIC:
		/*
		 * Info is the original error code.
		 */
		myobjv[0] = NewStringObj("panic", strlen("panic"));
		myobjv[myobjc++] = Tcl_NewIntObj(*(int *)info);
		break;
	case DB_EVENT_REP_CLIENT:
		myobjv[0] = NewStringObj("rep_client", strlen("rep_client"));
		break;
	case DB_EVENT_REP_ELECTED:
		myobjv[0] = NewStringObj("elected", strlen("elected"));
		break;
	case DB_EVENT_REP_MASTER:
		myobjv[0] = NewStringObj("rep_master", strlen("rep_master"));
		break;
	case DB_EVENT_REP_NEWMASTER:
		/*
		 * Info is the EID of the new master.
		 */
		myobjv[0] = NewStringObj("newmaster", strlen("newmaster"));
		myobjv[myobjc++] = Tcl_NewIntObj(*(int *)info);
		break;
	case DB_EVENT_REP_PERM_FAILED:
		myobjv[0] = NewStringObj("perm_failed", strlen("perm_failed"));
		break;
	case DB_EVENT_REP_STARTUPDONE:
		myobjv[0] = NewStringObj("startupdone", strlen("startupdone"));
		break;
	case DB_EVENT_WRITE_FAILED:
		myobjv[0] =
		    NewStringObj("write_failed", strlen("write_failed"));
		break;
	default:
		__db_errx(dbenv->env, "Tcl unknown event %lu", (u_long)event);
		return;
	}

	for (i = 0; i < myobjc; i++)
		Tcl_IncrRefCount(myobjv[i]);

	event_o = Tcl_NewListObj(myobjc, myobjv);
	Tcl_IncrRefCount(event_o);
	objv[2] = event_o;

	/*
	 * We really want to return the original result to the
	 * user.  So, save the result obj here, and then after
	 * we've taken care of the Tcl_EvalObjv, set the result
	 * back to this original result.
	 */
	origobj = Tcl_GetObjResult(interp);
	Tcl_IncrRefCount(origobj);
	result = Tcl_EvalObjv(interp, TCLDB_SENDEVENT, objv, 0);
	if (result != TCL_OK) {
		/*
		 * XXX
		 * This probably isn't the right error behavior, but
		 * this error should only happen if the Tcl callback is
		 * somehow invalid, which is a fatal scripting bug.
		 * The event handler is a void function so we either
		 * just return or abort.
		 * For now, abort.
		 */
		__db_errx(dbenv->env, "Tcl event failure");
		__os_abort(dbenv->env);
	}

	Tcl_SetObjResult(interp, origobj);
	Tcl_DecrRefCount(origobj);
	for (i = 0; i < myobjc; i++)
		Tcl_DecrRefCount(myobjv[i]);
	Tcl_DecrRefCount(event_o);

	return;
}

#define	INVALID_LSNMSG "Invalid LSN with %d parts. Should have 2.\n"

/*
 * PUBLIC: int _GetLsn __P((Tcl_Interp *, Tcl_Obj *, DB_LSN *));
 */
int
_GetLsn(interp, obj, lsn)
	Tcl_Interp *interp;
	Tcl_Obj *obj;
	DB_LSN *lsn;
{
	Tcl_Obj **myobjv;
	char msg[MSG_SIZE];
	int myobjc, result;
	u_int32_t tmp;

	result = Tcl_ListObjGetElements(interp, obj, &myobjc, &myobjv);
	if (result == TCL_ERROR)
		return (result);
	if (myobjc != 2) {
		result = TCL_ERROR;
		snprintf(msg, MSG_SIZE, INVALID_LSNMSG, myobjc);
		Tcl_SetResult(interp, msg, TCL_VOLATILE);
		return (result);
	}
	result = _GetUInt32(interp, myobjv[0], &tmp);
	if (result == TCL_ERROR)
		return (result);
	lsn->file = tmp;
	result = _GetUInt32(interp, myobjv[1], &tmp);
	lsn->offset = tmp;
	return (result);
}

/*
 * _GetUInt32 --
 *	Get a u_int32_t from a Tcl object.  Tcl_GetIntFromObj does the
 * right thing most of the time, but on machines where a long is 8 bytes
 * and an int is 4 bytes, it errors on integers between the maximum
 * int32_t and the maximum u_int32_t.  This is correct, but we generally
 * want a u_int32_t in the end anyway, so we use Tcl_GetLongFromObj and do
 * the bounds checking ourselves.
 *
 * This code looks much like Tcl_GetIntFromObj, only with a different
 * bounds check.  It's essentially Tcl_GetUnsignedIntFromObj, which
 * unfortunately doesn't exist.
 *
 * PUBLIC: int _GetUInt32 __P((Tcl_Interp *, Tcl_Obj *, u_int32_t *));
 */
int
_GetUInt32(interp, obj, resp)
	Tcl_Interp *interp;
	Tcl_Obj *obj;
	u_int32_t *resp;
{
	int result;
	long ltmp;

	result = Tcl_GetLongFromObj(interp, obj, &ltmp);
	if (result != TCL_OK)
		return (result);

	if ((unsigned long)ltmp != (u_int32_t)ltmp) {
		if (interp != NULL) {
			Tcl_ResetResult(interp);
			Tcl_AppendToObj(Tcl_GetObjResult(interp),
			    "integer value too large for u_int32_t", -1);
		}
		return (TCL_ERROR);
	}

	*resp = (u_int32_t)ltmp;
	return (TCL_OK);
}

/*
 * _GetFlagsList --
 *	Get a new Tcl object, containing a list of the string values
 * associated with a particular set of flag values.
 *
 * PUBLIC: Tcl_Obj *_GetFlagsList __P((Tcl_Interp *, u_int32_t, const FN *));
 */
Tcl_Obj *
_GetFlagsList(interp, flags, fnp)
	Tcl_Interp *interp;
	u_int32_t flags;
	const FN *fnp;
{
	Tcl_Obj *newlist, *newobj;
	int result;

	newlist = Tcl_NewObj();

	/*
	 * If the Berkeley DB library wasn't compiled with statistics, then
	 * we may get a NULL reference.
	 */
	if (fnp == NULL)
		return (newlist);

	/*
	 * Append a Tcl_Obj containing each pertinent flag string to the
	 * specified Tcl list.
	 */
	for (; fnp->mask != 0; ++fnp)
		if (LF_ISSET(fnp->mask)) {
			newobj = NewStringObj(fnp->name, strlen(fnp->name));
			result =
			    Tcl_ListObjAppendElement(interp, newlist, newobj);

			/*
			 * Tcl_ListObjAppendElement is defined to return TCL_OK
			 * unless newlist isn't actually a list (or convertible
			 * into one).  If this is the case, we screwed up badly
			 * somehow.
			 */
			DB_ASSERT(NULL, result == TCL_OK);
		}

	return (newlist);
}

int __debug_stop, __debug_on, __debug_print, __debug_test;

/*
 * PUBLIC: void _debug_check  __P((void));
 */
void
_debug_check()
{
	if (__debug_on == 0)
		return;

	if (__debug_print != 0) {
		printf("\r%7d:", __debug_on);
		(void)fflush(stdout);
	}
	if (__debug_on++ == __debug_test || __debug_stop)
		__db_loadme();
}

/*
 * XXX
 * Tcl 8.1+ Tcl_GetByteArrayFromObj/Tcl_GetIntFromObj bug.
 *
 * There is a bug in Tcl 8.1+ and byte arrays in that if it happens
 * to use an object as both a byte array and something else like
 * an int, and you've done a Tcl_GetByteArrayFromObj, then you
 * do a Tcl_GetIntFromObj, your memory is deleted.
 *
 * Workaround is for all byte arrays we want to use, if it can be
 * represented as an integer, we copy it so that we don't lose the
 * memory.
 */
/*
 * PUBLIC: int _CopyObjBytes  __P((Tcl_Interp *, Tcl_Obj *obj, void *,
 * PUBLIC:     u_int32_t *, int *));
 */
int
_CopyObjBytes(interp, obj, newp, sizep, freep)
	Tcl_Interp *interp;
	Tcl_Obj *obj;
	void *newp;
	u_int32_t *sizep;
	int *freep;
{
	void *tmp, *new;
	int i, len, ret;

	/*
	 * If the object is not an int, then just return the byte
	 * array because it won't be transformed out from under us.
	 * If it is a number, we need to copy it.
	 */
	*freep = 0;
	ret = Tcl_GetIntFromObj(interp, obj, &i);
	tmp = Tcl_GetByteArrayFromObj(obj, &len);
	*sizep = (u_int32_t)len;
	if (ret == TCL_ERROR) {
		Tcl_ResetResult(interp);
		*(void **)newp = tmp;
		return (0);
	}

	/*
	 * If we get here, we have an integer that might be reused
	 * at some other point so we cannot count on GetByteArray
	 * keeping our pointer valid.
	 */
	if ((ret = __os_malloc(NULL, (size_t)len, &new)) != 0)
		return (ret);
	memcpy(new, tmp, (size_t)len);
	*(void **)newp = new;
	*freep = 1;
	return (0);
}
