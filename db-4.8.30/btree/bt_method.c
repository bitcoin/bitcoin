/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/btree.h"
#include "dbinc/qam.h"

static int __bam_set_bt_minkey __P((DB *, u_int32_t));
static int __bam_get_bt_compare
	       __P((DB *, int (**)(DB *, const DBT *, const DBT *)));
static int __bam_get_bt_prefix
	       __P((DB *, size_t(**)(DB *, const DBT *, const DBT *)));
static int __bam_set_bt_prefix
	       __P((DB *, size_t(*)(DB *, const DBT *, const DBT *)));
static int __bam_get_bt_compress __P((DB *,
    int (**)(DB *, const DBT *, const DBT *, const DBT *, const DBT *, DBT *),
    int (**)(DB *, const DBT *, const DBT *, DBT *, DBT *, DBT *)));
static int __ram_get_re_delim __P((DB *, int *));
static int __ram_set_re_delim __P((DB *, int));
static int __ram_set_re_len __P((DB *, u_int32_t));
static int __ram_set_re_pad __P((DB *, int));
static int __ram_get_re_source __P((DB *, const char **));
static int __ram_set_re_source __P((DB *, const char *));

/*
 * __bam_db_create --
 *	Btree specific initialization of the DB structure.
 *
 * PUBLIC: int __bam_db_create __P((DB *));
 */
int
__bam_db_create(dbp)
	DB *dbp;
{
	BTREE *t;
	int ret;

	/* Allocate and initialize the private btree structure. */
	if ((ret = __os_calloc(dbp->env, 1, sizeof(BTREE), &t)) != 0)
		return (ret);
	dbp->bt_internal = t;

	t->bt_minkey = DEFMINKEYPAGE;		/* Btree */
	t->bt_compare = __bam_defcmp;
	t->bt_prefix = __bam_defpfx;
#ifdef HAVE_COMPRESSION
	t->bt_compress = NULL;
	t->bt_decompress = NULL;
	t->compress_dup_compare = NULL;

	/*
	 * DB_AM_COMPRESS may have been set in __bam_metachk before the
	 * bt_internal structure existed.
	 */
	if (F_ISSET(dbp, DB_AM_COMPRESS) &&
	    (ret = __bam_set_bt_compress(dbp, NULL, NULL)) != 0)
		return (ret);
#endif

	dbp->get_bt_compare = __bam_get_bt_compare;
	dbp->set_bt_compare = __bam_set_bt_compare;
	dbp->get_bt_minkey = __bam_get_bt_minkey;
	dbp->set_bt_minkey = __bam_set_bt_minkey;
	dbp->get_bt_prefix = __bam_get_bt_prefix;
	dbp->set_bt_prefix = __bam_set_bt_prefix;
	dbp->get_bt_compress = __bam_get_bt_compress;
	dbp->set_bt_compress = __bam_set_bt_compress;

	t->re_pad = ' ';			/* Recno */
	t->re_delim = '\n';
	t->re_eof = 1;

	dbp->get_re_delim = __ram_get_re_delim;
	dbp->set_re_delim = __ram_set_re_delim;
	dbp->get_re_len = __ram_get_re_len;
	dbp->set_re_len = __ram_set_re_len;
	dbp->get_re_pad = __ram_get_re_pad;
	dbp->set_re_pad = __ram_set_re_pad;
	dbp->get_re_source = __ram_get_re_source;
	dbp->set_re_source = __ram_set_re_source;

	return (0);
}

/*
 * __bam_db_close --
 *	Btree specific discard of the DB structure.
 *
 * PUBLIC: int __bam_db_close __P((DB *));
 */
int
__bam_db_close(dbp)
	DB *dbp;
{
	BTREE *t;

	if ((t = dbp->bt_internal) == NULL)
		return (0);
						/* Recno */
	/* Close any backing source file descriptor. */
	if (t->re_fp != NULL)
		(void)fclose(t->re_fp);

	/* Free any backing source file name. */
	if (t->re_source != NULL)
		__os_free(dbp->env, t->re_source);

	__os_free(dbp->env, t);
	dbp->bt_internal = NULL;

	return (0);
}

/*
 * __bam_map_flags --
 *	Map Btree specific flags from public to the internal values.
 *
 * PUBLIC: void __bam_map_flags __P((DB *, u_int32_t *, u_int32_t *));
 */
void
__bam_map_flags(dbp, inflagsp, outflagsp)
	DB *dbp;
	u_int32_t *inflagsp, *outflagsp;
{
	COMPQUIET(dbp, NULL);

	if (FLD_ISSET(*inflagsp, DB_DUP)) {
		FLD_SET(*outflagsp, DB_AM_DUP);
		FLD_CLR(*inflagsp, DB_DUP);
	}
	if (FLD_ISSET(*inflagsp, DB_DUPSORT)) {
		FLD_SET(*outflagsp, DB_AM_DUP | DB_AM_DUPSORT);
		FLD_CLR(*inflagsp, DB_DUPSORT);
	}
	if (FLD_ISSET(*inflagsp, DB_RECNUM)) {
		FLD_SET(*outflagsp, DB_AM_RECNUM);
		FLD_CLR(*inflagsp, DB_RECNUM);
	}
	if (FLD_ISSET(*inflagsp, DB_REVSPLITOFF)) {
		FLD_SET(*outflagsp, DB_AM_REVSPLITOFF);
		FLD_CLR(*inflagsp, DB_REVSPLITOFF);
	}
}

/*
 * __bam_set_flags --
 *	Set Btree specific flags.
 *
 * PUBLIC: int __bam_set_flags __P((DB *, u_int32_t *flagsp));
 */
int
__bam_set_flags(dbp, flagsp)
	DB *dbp;
	u_int32_t *flagsp;
{
	BTREE *t;
	u_int32_t flags;

	t = dbp->bt_internal;

	flags = *flagsp;
	if (LF_ISSET(DB_DUP | DB_DUPSORT | DB_RECNUM | DB_REVSPLITOFF))
		DB_ILLEGAL_AFTER_OPEN(dbp, "DB->set_flags");

	/*
	 * The DB_DUP and DB_DUPSORT flags are shared by the Hash
	 * and Btree access methods.
	 */
	if (LF_ISSET(DB_DUP | DB_DUPSORT))
		DB_ILLEGAL_METHOD(dbp, DB_OK_BTREE | DB_OK_HASH);

	if (LF_ISSET(DB_RECNUM | DB_REVSPLITOFF))
		DB_ILLEGAL_METHOD(dbp, DB_OK_BTREE);

	/* DB_DUP/DB_DUPSORT is incompatible with DB_RECNUM. */
	if (LF_ISSET(DB_DUP | DB_DUPSORT) && F_ISSET(dbp, DB_AM_RECNUM))
		goto incompat;

	/* DB_RECNUM is incompatible with DB_DUP/DB_DUPSORT. */
	if (LF_ISSET(DB_RECNUM) && F_ISSET(dbp, DB_AM_DUP))
		goto incompat;

	/* DB_RECNUM is incompatible with DB_DUP/DB_DUPSORT. */
	if (LF_ISSET(DB_RECNUM) && LF_ISSET(DB_DUP | DB_DUPSORT))
		goto incompat;

#ifdef HAVE_COMPRESSION
	/* DB_RECNUM is incompatible with compression */
	if (LF_ISSET(DB_RECNUM) && DB_IS_COMPRESSED(dbp)) {
		__db_errx(dbp->env,
		    "DB_RECNUM cannot be used with compression");
		return (EINVAL);
	}

	/* DB_DUP without DB_DUPSORT is incompatible with compression */
	if (LF_ISSET(DB_DUP) && !LF_ISSET(DB_DUPSORT) &&
		!F_ISSET(dbp, DB_AM_DUPSORT) && DB_IS_COMPRESSED(dbp)) {
		__db_errx(dbp->env,
		   "DB_DUP cannot be used with compression without DB_DUPSORT");
		return (EINVAL);
	}
#endif

	if (LF_ISSET(DB_DUPSORT) && dbp->dup_compare == NULL) {
#ifdef HAVE_COMPRESSION
		if (DB_IS_COMPRESSED(dbp)) {
			dbp->dup_compare = __bam_compress_dupcmp;
			t->compress_dup_compare = __bam_defcmp;
		} else
#endif
			dbp->dup_compare = __bam_defcmp;
	}

	__bam_map_flags(dbp, flagsp, &dbp->flags);
	return (0);

incompat:
	return (__db_ferr(dbp->env, "DB->set_flags", 1));
}

/*
 * __bam_get_bt_compare --
 *	Get the comparison function.
 */
static int
__bam_get_bt_compare(dbp, funcp)
	DB *dbp;
	int (**funcp) __P((DB *, const DBT *, const DBT *));
{
	BTREE *t;

	DB_ILLEGAL_METHOD(dbp, DB_OK_BTREE);

	t = dbp->bt_internal;

	if (funcp != NULL)
		*funcp = t->bt_compare;

	return (0);
}

/*
 * __bam_set_bt_compare --
 *	Set the comparison function.
 *
 * PUBLIC: int __bam_set_bt_compare
 * PUBLIC:         __P((DB *, int (*)(DB *, const DBT *, const DBT *)));
 */
int
__bam_set_bt_compare(dbp, func)
	DB *dbp;
	int (*func) __P((DB *, const DBT *, const DBT *));
{
	BTREE *t;

	DB_ILLEGAL_AFTER_OPEN(dbp, "DB->set_bt_compare");
	DB_ILLEGAL_METHOD(dbp, DB_OK_BTREE);

	t = dbp->bt_internal;

	/*
	 * Can't default the prefix routine if the user supplies a comparison
	 * routine; shortening the keys can break their comparison algorithm.
	 */
	t->bt_compare = func;
	if (t->bt_prefix == __bam_defpfx)
		t->bt_prefix = NULL;

	return (0);
}

/*
 * __bam_get_bt_compress --
 *	Get the compression functions.
 */
static int
__bam_get_bt_compress(dbp, compressp, decompressp)
	DB *dbp;
	int (**compressp) __P((DB *, const DBT *, const DBT *, const DBT *,
				      const DBT *, DBT *));
	int (**decompressp) __P((DB *, const DBT *, const DBT *, DBT *, DBT *,
					DBT *));
{
#ifdef HAVE_COMPRESSION
	BTREE *t;

	DB_ILLEGAL_METHOD(dbp, DB_OK_BTREE);

	t = dbp->bt_internal;

	if (compressp != NULL)
		*compressp = t->bt_compress;
	if (decompressp != NULL)
		*decompressp = t->bt_decompress;

	return (0);
#else
	COMPQUIET(compressp, NULL);
	COMPQUIET(decompressp, NULL);

	__db_errx(dbp->env, "compression support has not been compiled in");
	return (EINVAL);
#endif
}

/*
 * __bam_set_bt_compress --
 *	Set the compression functions.
 *
 * PUBLIC: int __bam_set_bt_compress __P((DB *,
 * PUBLIC:  int (*)(DB *, const DBT *, const DBT *,
 * PUBLIC:	    const DBT *, const DBT *, DBT *),
 * PUBLIC:  int (*)(DB *, const DBT *, const DBT *, DBT *, DBT *, DBT *)));
 */
int
__bam_set_bt_compress(dbp, compress, decompress)
	DB *dbp;
	int (*compress) __P((DB *, const DBT *, const DBT *, const DBT *,
				    const DBT *, DBT *));
	int (*decompress) __P((DB *, const DBT *, const DBT *, DBT *, DBT *,
				      DBT *));
{
#ifdef HAVE_COMPRESSION
	BTREE *t;

	DB_ILLEGAL_AFTER_OPEN(dbp, "DB->set_bt_compress");
	DB_ILLEGAL_METHOD(dbp, DB_OK_BTREE);

	t = dbp->bt_internal;

	/* compression is incompatible with DB_RECNUM */
	if (F_ISSET(dbp, DB_AM_RECNUM)) {
		__db_errx(dbp->env,
		    "compression cannot be used with DB_RECNUM");
		return (EINVAL);
	}

	/* compression is incompatible with DB_DUP without DB_DUPSORT */
	if (F_ISSET(dbp, DB_AM_DUP) && !F_ISSET(dbp, DB_AM_DUPSORT)) {
		__db_errx(dbp->env,
		   "compression cannot be used with DB_DUP without DB_DUPSORT");
		return (EINVAL);
	}

	if (compress != 0 && decompress != 0) {
		t->bt_compress = compress;
		t->bt_decompress = decompress;
	} else if (compress == 0 && decompress == 0) {
		t->bt_compress = __bam_defcompress;
		t->bt_decompress = __bam_defdecompress;
	} else {
		__db_errx(dbp->env,
	    "to enable compression you need to supply both function arguments");
		return (EINVAL);
	}
	F_SET(dbp, DB_AM_COMPRESS);

	/* Copy dup_compare to compress_dup_compare, and use the compression
	   duplicate compare */
	if (F_ISSET(dbp, DB_AM_DUPSORT)) {
		t->compress_dup_compare = dbp->dup_compare;
		dbp->dup_compare = __bam_compress_dupcmp;
	}

	return (0);
#else
	COMPQUIET(compress, NULL);
	COMPQUIET(decompress, NULL);

	__db_errx(dbp->env, "compression support has not been compiled in");
	return (EINVAL);
#endif
}

/*
 * __db_get_bt_minkey --
 *	Get the minimum keys per page.
 *
 * PUBLIC: int __bam_get_bt_minkey __P((DB *, u_int32_t *));
 */
int
__bam_get_bt_minkey(dbp, bt_minkeyp)
	DB *dbp;
	u_int32_t *bt_minkeyp;
{
	BTREE *t;

	DB_ILLEGAL_METHOD(dbp, DB_OK_BTREE);

	t = dbp->bt_internal;
	*bt_minkeyp = t->bt_minkey;
	return (0);
}

/*
 * __bam_set_bt_minkey --
 *	Set the minimum keys per page.
 */
static int
__bam_set_bt_minkey(dbp, bt_minkey)
	DB *dbp;
	u_int32_t bt_minkey;
{
	BTREE *t;

	DB_ILLEGAL_AFTER_OPEN(dbp, "DB->set_bt_minkey");
	DB_ILLEGAL_METHOD(dbp, DB_OK_BTREE);

	t = dbp->bt_internal;

	if (bt_minkey < 2) {
		__db_errx(dbp->env, "minimum bt_minkey value is 2");
		return (EINVAL);
	}

	t->bt_minkey = bt_minkey;
	return (0);
}

/*
 * __bam_get_bt_prefix --
 *	Get the prefix function.
 */
static int
__bam_get_bt_prefix(dbp, funcp)
	DB *dbp;
	size_t (**funcp) __P((DB *, const DBT *, const DBT *));
{
	BTREE *t;

	DB_ILLEGAL_METHOD(dbp, DB_OK_BTREE);

	t = dbp->bt_internal;
	if (funcp != NULL)
		*funcp = t->bt_prefix;
	return (0);
}

/*
 * __bam_set_bt_prefix --
 *	Set the prefix function.
 */
static int
__bam_set_bt_prefix(dbp, func)
	DB *dbp;
	size_t (*func) __P((DB *, const DBT *, const DBT *));
{
	BTREE *t;

	DB_ILLEGAL_AFTER_OPEN(dbp, "DB->set_bt_prefix");
	DB_ILLEGAL_METHOD(dbp, DB_OK_BTREE);

	t = dbp->bt_internal;

	t->bt_prefix = func;
	return (0);
}

/*
 * __bam_copy_config
 *	Copy the configuration of one DB handle to another.
 * PUBLIC: void __bam_copy_config __P((DB *, DB*, u_int32_t));
 */
void
__bam_copy_config(src, dst, nparts)
	DB *src, *dst;
	u_int32_t nparts;
{
	BTREE *s, *d;

	COMPQUIET(nparts, 0);

	s = src->bt_internal;
	d = dst->bt_internal;
	d->bt_compare = s->bt_compare;
	d->bt_minkey = s->bt_minkey;
	d->bt_minkey = s->bt_minkey;
	d->bt_prefix = s->bt_prefix;
#ifdef HAVE_COMPRESSION
	d->bt_compress = s->bt_compress;
	d->bt_decompress = s->bt_decompress;
	d->compress_dup_compare = s->compress_dup_compare;
#endif
}

/*
 * __ram_map_flags --
 *	Map Recno specific flags from public to the internal values.
 *
 * PUBLIC: void __ram_map_flags __P((DB *, u_int32_t *, u_int32_t *));
 */
void
__ram_map_flags(dbp, inflagsp, outflagsp)
	DB *dbp;
	u_int32_t *inflagsp, *outflagsp;
{
	COMPQUIET(dbp, NULL);

	if (FLD_ISSET(*inflagsp, DB_RENUMBER)) {
		FLD_SET(*outflagsp, DB_AM_RENUMBER);
		FLD_CLR(*inflagsp, DB_RENUMBER);
	}
	if (FLD_ISSET(*inflagsp, DB_SNAPSHOT)) {
		FLD_SET(*outflagsp, DB_AM_SNAPSHOT);
		FLD_CLR(*inflagsp, DB_SNAPSHOT);
	}
}

/*
 * __ram_set_flags --
 *	Set Recno specific flags.
 *
 * PUBLIC: int __ram_set_flags __P((DB *, u_int32_t *flagsp));
 */
int
__ram_set_flags(dbp, flagsp)
	DB *dbp;
	u_int32_t *flagsp;
{
	u_int32_t flags;

	flags = *flagsp;
	if (LF_ISSET(DB_RENUMBER | DB_SNAPSHOT)) {
		DB_ILLEGAL_AFTER_OPEN(dbp, "DB->set_flags");
		DB_ILLEGAL_METHOD(dbp, DB_OK_RECNO);
	}

	__ram_map_flags(dbp, flagsp, &dbp->flags);
	return (0);
}

/*
 * __db_get_re_delim --
 *	Get the variable-length input record delimiter.
 */
static int
__ram_get_re_delim(dbp, re_delimp)
	DB *dbp;
	int *re_delimp;
{
	BTREE *t;

	DB_ILLEGAL_METHOD(dbp, DB_OK_RECNO);
	t = dbp->bt_internal;
	*re_delimp = t->re_delim;
	return (0);
}

/*
 * __ram_set_re_delim --
 *	Set the variable-length input record delimiter.
 */
static int
__ram_set_re_delim(dbp, re_delim)
	DB *dbp;
	int re_delim;
{
	BTREE *t;

	DB_ILLEGAL_AFTER_OPEN(dbp, "DB->set_re_delim");
	DB_ILLEGAL_METHOD(dbp, DB_OK_RECNO);

	t = dbp->bt_internal;

	t->re_delim = re_delim;
	F_SET(dbp, DB_AM_DELIMITER);

	return (0);
}

/*
 * __db_get_re_len --
 *	Get the variable-length input record length.
 *
 * PUBLIC: int __ram_get_re_len __P((DB *, u_int32_t *));
 */
int
__ram_get_re_len(dbp, re_lenp)
	DB *dbp;
	u_int32_t *re_lenp;
{
	BTREE *t;
	QUEUE *q;

	DB_ILLEGAL_METHOD(dbp, DB_OK_QUEUE | DB_OK_RECNO);

	/*
	 * This has to work for all access methods, before or after opening the
	 * database.  When the record length is set with __ram_set_re_len, the
	 * value in both the BTREE and QUEUE structs will be correct.
	 * Otherwise, this only makes sense after the database in opened, in
	 * which case we know the type.
	 */
	if (dbp->type == DB_QUEUE) {
		q = dbp->q_internal;
		*re_lenp = q->re_len;
	} else {
		t = dbp->bt_internal;
		*re_lenp = t->re_len;
	}

	return (0);
}

/*
 * __ram_set_re_len --
 *	Set the variable-length input record length.
 */
static int
__ram_set_re_len(dbp, re_len)
	DB *dbp;
	u_int32_t re_len;
{
	BTREE *t;
	QUEUE *q;

	DB_ILLEGAL_AFTER_OPEN(dbp, "DB->set_re_len");
	DB_ILLEGAL_METHOD(dbp, DB_OK_QUEUE | DB_OK_RECNO);

	t = dbp->bt_internal;
	t->re_len = re_len;

	q = dbp->q_internal;
	q->re_len = re_len;

	F_SET(dbp, DB_AM_FIXEDLEN);

	return (0);
}

/*
 * __db_get_re_pad --
 *	Get the fixed-length record pad character.
 *
 * PUBLIC: int __ram_get_re_pad __P((DB *, int *));
 */
int
__ram_get_re_pad(dbp, re_padp)
	DB *dbp;
	int *re_padp;
{
	BTREE *t;
	QUEUE *q;

	DB_ILLEGAL_METHOD(dbp, DB_OK_QUEUE | DB_OK_RECNO);

	/*
	 * This has to work for all access methods, before or after opening the
	 * database.  When the record length is set with __ram_set_re_pad, the
	 * value in both the BTREE and QUEUE structs will be correct.
	 * Otherwise, this only makes sense after the database in opened, in
	 * which case we know the type.
	 */
	if (dbp->type == DB_QUEUE) {
		q = dbp->q_internal;
		*re_padp = q->re_pad;
	} else {
		t = dbp->bt_internal;
		*re_padp = t->re_pad;
	}

	return (0);
}

/*
 * __ram_set_re_pad --
 *	Set the fixed-length record pad character.
 */
static int
__ram_set_re_pad(dbp, re_pad)
	DB *dbp;
	int re_pad;
{
	BTREE *t;
	QUEUE *q;

	DB_ILLEGAL_AFTER_OPEN(dbp, "DB->set_re_pad");
	DB_ILLEGAL_METHOD(dbp, DB_OK_QUEUE | DB_OK_RECNO);

	t = dbp->bt_internal;
	t->re_pad = re_pad;

	q = dbp->q_internal;
	q->re_pad = re_pad;

	F_SET(dbp, DB_AM_PAD);

	return (0);
}

/*
 * __db_get_re_source --
 *	Get the backing source file name.
 */
static int
__ram_get_re_source(dbp, re_sourcep)
	DB *dbp;
	const char **re_sourcep;
{
	BTREE *t;

	DB_ILLEGAL_METHOD(dbp, DB_OK_RECNO);

	t = dbp->bt_internal;
	*re_sourcep = t->re_source;
	return (0);
}

/*
 * __ram_set_re_source --
 *	Set the backing source file name.
 */
static int
__ram_set_re_source(dbp, re_source)
	DB *dbp;
	const char *re_source;
{
	BTREE *t;

	DB_ILLEGAL_AFTER_OPEN(dbp, "DB->set_re_source");
	DB_ILLEGAL_METHOD(dbp, DB_OK_RECNO);

	t = dbp->bt_internal;

	return (__os_strdup(dbp->env, re_source, &t->re_source));
}
