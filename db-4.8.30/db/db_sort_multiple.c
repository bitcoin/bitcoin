/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/btree.h"

static int __db_quicksort __P((DB *, DBT *, DBT *, u_int32_t *, u_int32_t *,
		u_int32_t *, u_int32_t *, u_int32_t));

/*
 * __db_compare_both --
 *	Use the comparison functions from db to compare akey and bkey, and if
 *	DB_DUPSORT adata and bdata.
 *
 * PUBLIC: int __db_compare_both __P((DB *, const DBT *, const DBT *,
 * PUBLIC:   const DBT *, const DBT *));
 */
int
__db_compare_both(db, akey, adata, bkey, bdata)
	DB *db;
	const DBT *akey;
	const DBT *adata;
	const DBT *bkey;
	const DBT *bdata;
{
	BTREE *t;
	int cmp;

	t = (BTREE *)db->bt_internal;

	cmp = t->bt_compare(db, akey, bkey);
	if (cmp != 0) return cmp;
	if (!F_ISSET(db, DB_AM_DUPSORT)) return 0;

	if (adata == 0) return bdata == 0 ? 0 : -1;
	if (bdata == 0) return 1;

#ifdef HAVE_COMPRESSION
	if (DB_IS_COMPRESSED(db))
		return t->compress_dup_compare(db, adata, bdata);
#endif
	return db->dup_compare(db, adata, bdata);
}

#define	DB_SORT_SWAP(a, ad, b, bd)					\
do {									\
	tmp = (a)[0]; (a)[0] = (b)[0]; (b)[0] = tmp;			\
	tmp = (a)[-1]; (a)[-1] = (b)[-1]; (b)[-1] = tmp;		\
	if (data != NULL) {						\
		tmp = (ad)[0]; (ad)[0] = (bd)[0]; (bd)[0] = tmp;	\
		tmp = (ad)[-1]; (ad)[-1] = (bd)[-1]; (bd)[-1] = tmp;	\
	}								\
} while (0)

#define	DB_SORT_LOAD_DBT(a, ad, aptr, adptr)				\
do {									\
	(a).data = (u_int8_t*)key->data + (aptr)[0];			\
	(a).size = (aptr)[-1];						\
	if (data != NULL) {						\
		(ad).data = (u_int8_t*)data->data + (adptr)[0];		\
		(ad).size = (adptr)[-1];				\
	}								\
} while (0)

#define	DB_SORT_COMPARE(a, ad, b, bd) (data != NULL ?			\
	__db_compare_both(db, &(a), &(ad), &(b), &(bd)) :		\
	__db_compare_both(db, &(a), 0, &(b), 0))

#define	DB_SORT_STACKSIZE 32

/*
 * __db_quicksort --
 *	The quicksort implementation for __db_sort_multiple() and
 *	__db_sort_multiple_key().
 */
static int
__db_quicksort(db, key, data, kstart, kend, dstart, dend, size)
	DB *db;
	DBT *key, *data;
	u_int32_t *kstart, *kend, *dstart, *dend;
	u_int32_t size;
{
	int ret;
	u_int32_t tmp;
	u_int32_t *kmiddle, *dmiddle, *kptr, *dptr;
	DBT a, ad, b, bd, m, md;
	ENV *env;

	struct DB_SORT_quicksort_stack {
		u_int32_t *kstart;
		u_int32_t *kend;
		u_int32_t *dstart;
		u_int32_t *dend;
	} stackbuf[DB_SORT_STACKSIZE], *stack;
	u_int32_t soff, slen;

	ret = 0;
	env = db->env;

	memset(&a, 0, sizeof(DBT));
	memset(&ad, 0, sizeof(DBT));
	memset(&b, 0, sizeof(DBT));
	memset(&bd, 0, sizeof(DBT));
	memset(&m, 0, sizeof(DBT));
	memset(&md, 0, sizeof(DBT));

	/* NB end is smaller than start */

	stack = stackbuf;
	soff = 0;
	slen = DB_SORT_STACKSIZE;

 start:
	if (kend >= kstart) goto pop;

	/* If there's only one value, it's already sorted */
	tmp = (u_int32_t)(kstart - kend) / size;
	if (tmp == 1) goto pop;

	DB_SORT_LOAD_DBT(a, ad, kstart, dstart);
	DB_SORT_LOAD_DBT(b, bd, kend + size, dend + size);

	if (tmp == 2) {
		/* Special case the sorting of two value sequences */
		if (DB_SORT_COMPARE(a, ad, b, bd) > 0) {
			DB_SORT_SWAP(kstart, dstart, kend + size, dend + size);
		}
		goto pop;
	}

	kmiddle = kstart - (tmp / 2) * size;
	dmiddle = dstart - (tmp / 2) * size;
	DB_SORT_LOAD_DBT(m, md, kmiddle, dmiddle);

	/* Find the median of three */
	if (DB_SORT_COMPARE(a, ad, b, bd) < 0) {
		if (DB_SORT_COMPARE(m, md, a, ad) < 0) {
			/* m < a < b */
			DB_SORT_SWAP(kstart, dstart, kend + size, dend + size);
		} else if (DB_SORT_COMPARE(m, md, b, bd) < 0) {
			/* a < m < b */
			DB_SORT_SWAP(kmiddle,
			    dmiddle, kend + size, dend + size);
		} else {
			/* a < b < m */
			/* Do nothing */
		}
	} else {
		if (DB_SORT_COMPARE(a, ad, m, md) < 0) {
			/* b < a < m */
			DB_SORT_SWAP(kstart, dstart, kend + size, dend + size);
		} else if (DB_SORT_COMPARE(b, bd, m, md) < 0) {
			/* b < m < a */
			DB_SORT_SWAP(kmiddle,
			    dmiddle, kend + size, dend + size);
		} else {
			/* m < b < a */
			/* Do nothing */
		}
	}

	/* partition */
	DB_SORT_LOAD_DBT(b, bd, kend + size, dend + size);
	kmiddle = kstart;
	dmiddle = dstart;
	for (kptr = kstart, dptr = dstart; kptr > kend;
	    kptr -= size, dptr -= size) {
		DB_SORT_LOAD_DBT(a, ad, kptr, dptr);
		if (DB_SORT_COMPARE(a, ad, b, bd) < 0) {
			DB_SORT_SWAP(kmiddle, dmiddle, kptr, dptr);
			kmiddle -= size;
			dmiddle -= size;
		}
	}

	DB_SORT_SWAP(kmiddle, dmiddle, kend + size, dend + size);

	if (soff == slen) {
		/* Grow the stack */
		slen = slen * 2;
		if (stack == stackbuf) {
			ret = __os_malloc(env, slen *
				sizeof(struct DB_SORT_quicksort_stack), &stack);
			if (ret != 0) goto error;
			memcpy(stack, stackbuf, soff *
				sizeof(struct DB_SORT_quicksort_stack));
		} else {
			ret = __os_realloc(env, slen *
				sizeof(struct DB_SORT_quicksort_stack), &stack);
			if (ret != 0) goto error;
		}
	}

	/* divide and conquer */
	stack[soff].kstart = kmiddle - size;
	stack[soff].kend = kend;
	stack[soff].dstart = dmiddle - size;
	stack[soff].dend = dend;
	++soff;

	kend = kmiddle;
	dend = dmiddle;

	goto start;

 pop:
	if (soff != 0) {
		--soff;
		kstart = stack[soff].kstart;
		kend = stack[soff].kend;
		dstart = stack[soff].dstart;
		dend = stack[soff].dend;
		goto start;
	}

 error:
	if (stack != stackbuf)
		__os_free(env, stack);

	return ret;
}

#undef DB_SORT_SWAP
#undef DB_SORT_LOAD_DBT

/*
 * __db_sort_multiple --
 *	If flags == DB_MULTIPLE_KEY, sorts a DB_MULTIPLE_KEY format DBT using
 *	the BTree comparison function and duplicate comparison function.
 *
 *	If flags == DB_MULTIPLE, sorts one or two DB_MULTIPLE format DBTs using
 *	the BTree comparison function and duplicate comparison function. Will
 *	assume key and data specifies pairs of key/data to sort together. If
 *	data is NULL, will just sort key according to the btree comparison
 *	function.
 *
 *	Uses an in-place quicksort algorithm, with median of three for the pivot
 *	point.
 *
 * PUBLIC: int __db_sort_multiple __P((DB *, DBT *, DBT *, u_int32_t));
 */
int
__db_sort_multiple(db, key, data, flags)
	DB *db;
	DBT *key, *data;
	u_int32_t flags;
{
	u_int32_t *kstart, *kend, *dstart, *dend;

	/* TODO: sanity checks on the DBTs */
	/* DB_ILLEGAL_METHOD(db, DB_OK_BTREE); */

	kstart = (u_int32_t*)((u_int8_t *)key->data + key->ulen) - 1;

	switch (flags) {
	case DB_MULTIPLE:
		if (data != NULL)
			dstart = (u_int32_t*)((u_int8_t *)data->data +
				data->ulen) - 1;
		else
			dstart = kstart;

		/* Find the end */
		for (kend = kstart, dend = dstart;
		    *kend != (u_int32_t)-1 && *dend != (u_int32_t)-1;
		    kend -= 2, dend -= 2)
			;

		return (__db_quicksort(db, key, data, kstart, kend, dstart,
			dend, 2));
	case DB_MULTIPLE_KEY:
		/* Find the end */
		for (kend = kstart; *kend != (u_int32_t)-1; kend -= 4)
			;

		return (__db_quicksort(db, key, key, kstart, kend, kstart - 2,
			kend - 2, 4));
	default:
		return (__db_ferr(db->env, "DB->sort_multiple", 0));
	}
}
