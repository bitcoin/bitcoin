/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2004-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"
#ifdef HAVE_64BIT_TYPES

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/db_am.h"
#include "dbinc_auto/sequence_ext.h"

#ifdef HAVE_STATISTICS
static int __seq_print_all __P((DB_SEQUENCE *, u_int32_t));
static int __seq_print_stats __P((DB_SEQUENCE *, u_int32_t));

/*
 * __seq_stat --
 *	Get statistics from the sequence.
 *
 * PUBLIC: int __seq_stat __P((DB_SEQUENCE *, DB_SEQUENCE_STAT **, u_int32_t));
 */
int
__seq_stat(seq, spp, flags)
	DB_SEQUENCE *seq;
	DB_SEQUENCE_STAT **spp;
	u_int32_t flags;
{
	DB *dbp;
	DBT data;
	DB_SEQUENCE_STAT *sp;
	DB_SEQ_RECORD record;
	DB_THREAD_INFO *ip;
	ENV *env;
	int handle_check, ret, t_ret;

	dbp = seq->seq_dbp;
	env = dbp->env;

	SEQ_ILLEGAL_BEFORE_OPEN(seq, "DB_SEQUENCE->stat");

	switch (flags) {
	case DB_STAT_CLEAR:
	case DB_STAT_ALL:
	case 0:
		break;
	default:
		return (__db_ferr(env, "DB_SEQUENCE->stat", 0));
	}

	ENV_ENTER(env, ip);

	/* Check for replication block. */
	handle_check = IS_ENV_REPLICATED(env);
	if (handle_check && (ret = __db_rep_enter(dbp, 1, 0, 0)) != 0) {
		handle_check = 0;
		goto err;
	}

	/* Allocate and clear the structure. */
	if ((ret = __os_umalloc(env, sizeof(*sp), &sp)) != 0)
		goto err;
	memset(sp, 0, sizeof(*sp));

	if (seq->mtx_seq != MUTEX_INVALID) {
		__mutex_set_wait_info(
		    env, seq->mtx_seq, &sp->st_wait, &sp->st_nowait);

		if (LF_ISSET(DB_STAT_CLEAR))
			__mutex_clear(env, seq->mtx_seq);
	}
	memset(&data, 0, sizeof(data));
	data.data = &record;
	data.ulen = sizeof(record);
	data.flags = DB_DBT_USERMEM;
retry:	if ((ret = __db_get(dbp, ip, NULL, &seq->seq_key, &data, 0)) != 0) {
		if (ret == DB_BUFFER_SMALL &&
		    data.size > sizeof(seq->seq_record)) {
			if ((ret = __os_malloc(env,
			    data.size, &data.data)) != 0)
				goto err;
			data.ulen = data.size;
			goto retry;
		}
		goto err;
	}

	if (data.data != &record)
		memcpy(&record, data.data, sizeof(record));
	sp->st_current = record.seq_value;
	sp->st_value = seq->seq_record.seq_value;
	sp->st_last_value = seq->seq_last_value;
	sp->st_min = seq->seq_record.seq_min;
	sp->st_max = seq->seq_record.seq_max;
	sp->st_cache_size = seq->seq_cache_size;
	sp->st_flags = seq->seq_record.flags;

	*spp = sp;
	if (data.data != &record)
		__os_free(env, data.data);

	/* Release replication block. */
err:	if (handle_check && (t_ret = __env_db_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __seq_stat_print --
 *	Print statistics from the sequence.
 *
 * PUBLIC: int __seq_stat_print __P((DB_SEQUENCE *, u_int32_t));
 */
int
__seq_stat_print(seq, flags)
	DB_SEQUENCE *seq;
	u_int32_t flags;
{
	DB *dbp;
	DB_THREAD_INFO *ip;
	ENV *env;
	int handle_check, ret, t_ret;

	dbp = seq->seq_dbp;
	env = dbp->env;

	SEQ_ILLEGAL_BEFORE_OPEN(seq, "DB_SEQUENCE->stat_print");

	ENV_ENTER(env, ip);

	/* Check for replication block. */
	handle_check = IS_ENV_REPLICATED(env);
	if (handle_check && (ret = __db_rep_enter(dbp, 1, 0, 0)) != 0) {
		handle_check = 0;
		goto err;
	}

	if ((ret = __seq_print_stats(seq, flags)) != 0)
		goto err;

	if (LF_ISSET(DB_STAT_ALL) &&
	    (ret = __seq_print_all(seq, flags)) != 0)
		goto err;

	/* Release replication block. */
err:	if (handle_check && (t_ret = __env_db_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;

	ENV_LEAVE(env, ip);
	return (ret);

}

static const FN __db_seq_flags_fn[] = {
	{ DB_SEQ_DEC,		"decrement" },
	{ DB_SEQ_INC,		"increment" },
	{ DB_SEQ_RANGE_SET,	"range set (internal)" },
	{ DB_SEQ_WRAP,		"wraparound at end" },
	{ 0,			NULL }
};

/*
 * __db_get_seq_flags_fn --
 *	Return the __db_seq_flags_fn array.
 *
 * PUBLIC: const FN * __db_get_seq_flags_fn __P((void));
 */
const FN *
__db_get_seq_flags_fn()
{
	return (__db_seq_flags_fn);
}

/*
 * __seq_print_stats --
 *	Display sequence stat structure.
 */
static int
__seq_print_stats(seq, flags)
	DB_SEQUENCE *seq;
	u_int32_t flags;
{
	DB_SEQUENCE_STAT *sp;
	ENV *env;
	int ret;

	env = seq->seq_dbp->env;

	if ((ret = __seq_stat(seq, &sp, flags)) != 0)
		return (ret);
	__db_dl_pct(env,
	    "The number of sequence locks that required waiting",
	    (u_long)sp->st_wait,
	     DB_PCT(sp->st_wait, sp->st_wait + sp->st_nowait), NULL);
	STAT_FMT("The current sequence value",
	    INT64_FMT, int64_t, sp->st_current);
	STAT_FMT("The cached sequence value",
	    INT64_FMT, int64_t, sp->st_value);
	STAT_FMT("The last cached sequence value",
	    INT64_FMT, int64_t, sp->st_last_value);
	STAT_FMT("The minimum sequence value",
	    INT64_FMT, int64_t, sp->st_value);
	STAT_FMT("The maximum sequence value",
	    INT64_FMT, int64_t, sp->st_value);
	STAT_ULONG("The cache size", sp->st_cache_size);
	__db_prflags(env, NULL,
	    sp->st_flags, __db_seq_flags_fn, NULL, "\tSequence flags");
	__os_ufree(seq->seq_dbp->env, sp);
	return (0);
}

/*
 * __seq_print_all --
 *	Display sequence debugging information - none for now.
 *	(The name seems a bit strange, no?)
 */
static int
__seq_print_all(seq, flags)
	DB_SEQUENCE *seq;
	u_int32_t flags;
{
	COMPQUIET(seq, NULL);
	COMPQUIET(flags, 0);
	return (0);
}

#else /* !HAVE_STATISTICS */

int
__seq_stat(seq, statp, flags)
	DB_SEQUENCE *seq;
	DB_SEQUENCE_STAT **statp;
	u_int32_t flags;
{
	COMPQUIET(statp, NULL);
	COMPQUIET(flags, 0);

	return (__db_stat_not_built(seq->seq_dbp->env));
}

int
__seq_stat_print(seq, flags)
	DB_SEQUENCE *seq;
	u_int32_t flags;
{
	COMPQUIET(flags, 0);

	return (__db_stat_not_built(seq->seq_dbp->env));
}

/*
 * __db_get_seq_flags_fn --
 *	Return the __db_seq_flags_fn array.
 *
 * PUBLIC: const FN * __db_get_seq_flags_fn __P((void));
 */
const FN *
__db_get_seq_flags_fn()
{
	static const FN __db_seq_flags_fn[] = {
		{ 0,	NULL }
	};

	/*
	 * !!!
	 * The Tcl API uses this interface, stub it off.
	 */
	return (__db_seq_flags_fn);
}
#endif /* !HAVE_STATISTICS */
#endif /* HAVE_64BIT_TYPES */
