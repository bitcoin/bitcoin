/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/btree.h"

#ifdef HAVE_COMPRESSION

static int __bam_compress_marshal_data __P((DB *, const DBT *, DBT *));
static int __bam_compress_set_dbt __P((DB *, DBT *, const void *, u_int32_t));
static int __bamc_compress_del_and_get_next __P((DBC *, DBT *, DBT *));
static int __bamc_compress_get_bothc __P((DBC *, DBT *, u_int32_t));
static int __bamc_compress_get_multiple_key __P((DBC *, DBT *, u_int32_t));
static int __bamc_compress_get_multiple __P((DBC *, DBT *, DBT *,u_int32_t));
static int __bamc_compress_get_next __P((DBC *, u_int32_t));
static int __bamc_compress_get_next_dup __P((DBC *, DBT *, u_int32_t));
static int __bamc_compress_get_next_nodup __P((DBC *, u_int32_t));
static int __bamc_compress_get_prev __P((DBC *, u_int32_t));
static int __bamc_compress_get_prev_dup __P((DBC *, u_int32_t));
static int __bamc_compress_get_prev_nodup __P((DBC *, u_int32_t));
static int __bamc_compress_get_set __P((DBC *,
	DBT *, DBT *, u_int32_t, u_int32_t));
static int __bamc_compress_ibulk_del __P((DBC *, DBT *, u_int32_t));
static int __bamc_compress_idel __P((DBC *, u_int32_t));
static int __bamc_compress_iget __P((DBC *, DBT *, DBT *, u_int32_t));
static int __bamc_compress_iput __P((DBC *, DBT *, DBT *, u_int32_t));
static int __bamc_compress_relocate __P((DBC *));
static void __bamc_compress_reset __P((DBC *));
static int __bamc_compress_seek __P((DBC *,
	const DBT *, const DBT *, u_int32_t));
static int __bamc_compress_store __P((DBC *,
	DBT *, DBT*, DBT **, DBT **, DBT *, DBT *));
static int __bamc_next_decompress __P((DBC *));
static int __bamc_start_decompress __P((DBC *));

/*
 * Call __dbc_iget(), resizing DBTs if DB_BUFFER_SMALL is returned.
 * We're always using a transient cursor when this macro is used, so
 * we have to replace the OP with DB_CURRENT when we retry.
 */
#define	CMP_IGET_RETRY(ret, dbc, dbt1, dbt2, flags) do {		\
	DB_ASSERT((dbc)->env, F_ISSET((dbt1), DB_DBT_USERMEM));		\
	DB_ASSERT((dbc)->env, F_ISSET((dbt2), DB_DBT_USERMEM));		\
	if (((ret) =__dbc_iget((dbc),                                   \
	    (dbt1), (dbt2), (flags))) == DB_BUFFER_SMALL) {		\
		if ((CMP_RESIZE_DBT((ret), (dbc)->env, (dbt1))) != 0)	\
			break;						\
		if ((CMP_RESIZE_DBT((ret), (dbc)->env, (dbt2))) != 0)	\
			break;						\
		(ret) = __dbc_iget((dbc), (dbt1), (dbt2),		\
			((flags) & ~DB_OPFLAGS_MASK) | DB_CURRENT);	\
	}								\
} while (0)

#define	CMP_INIT_DBT(dbt) do {						\
	(dbt)->data = NULL;						\
	(dbt)->size = 0;						\
	(dbt)->ulen = 0;						\
	(dbt)->doff = 0;						\
	(dbt)->dlen = 0;						\
	(dbt)->flags = DB_DBT_USERMEM;					\
	(dbt)->app_data = NULL;						\
} while (0)

#define	CMP_FREE_DBT(env, dbt) do {					\
	DB_ASSERT((env), F_ISSET((dbt), DB_DBT_USERMEM));		\
	__os_free((env), (dbt)->data);					\
} while (0)

#define	CMP_RESIZE_DBT(ret, env, dbt)					\
	(((dbt)->size > (dbt)->ulen) ?					\
	((((ret) = __os_realloc((env), (dbt)->size, &(dbt)->data))	\
		!= 0) ? (ret) : (((dbt)->ulen = (dbt)->size), 0)) : 0)

static int
__bam_compress_set_dbt(dbp, dbt, data, size)
	DB *dbp;
	DBT *dbt;
	const void *data;
	u_int32_t size;
{
	int ret;

	ret = 0;
	DB_ASSERT(dbp->env, F_ISSET(dbt, DB_DBT_USERMEM));

	dbt->size = size;
	if (CMP_RESIZE_DBT(ret, dbp->env, dbt) != 0)
		return (ret);

	memcpy(dbt->data, data, size);
	return (0);
}

/******************************************************************************/

/*
 * Very simple key/data stream to give __bamc_compress_merge_insert()
 * a source of data to work on.
 */
struct __bam_compress_stream;
typedef struct __bam_compress_stream BTREE_COMPRESS_STREAM;
struct __bam_compress_stream
{
	int (*next)(BTREE_COMPRESS_STREAM *, DBT *, DBT *);

	void *kptr, *dptr;
	DBT *key, *data;
};

/*
 * These function prototypes can not go at the beginning because they rely on
 * on BTREE_COMPRESS_STREAM defined above.
 * The prototypes are required to avoid the Microsoft C++ compiler generating
 * warnings about mismatching parameter lists.
 */
static int __bam_cs_next_done __P((BTREE_COMPRESS_STREAM *, DBT *, DBT *));
static int __bam_cs_single_next __P((BTREE_COMPRESS_STREAM *, DBT *, DBT *));
static void __bam_cs_create_single
			__P((BTREE_COMPRESS_STREAM *, DBT *, DBT *));
static int __bam_cs_single_keyonly_next
			__P((BTREE_COMPRESS_STREAM *, DBT *, DBT *));
static void __bam_cs_create_single_keyonly
			__P((BTREE_COMPRESS_STREAM *, DBT *));
static int __bam_cs_multiple_key_next
			__P((BTREE_COMPRESS_STREAM *, DBT *, DBT *));
static void __bam_cs_create_multiple_key __P((BTREE_COMPRESS_STREAM *, DBT *));
static int __bam_cs_multiple_next __P((BTREE_COMPRESS_STREAM *, DBT *, DBT *));
static void __bam_cs_create_multiple
			__P((BTREE_COMPRESS_STREAM *, DBT *, DBT *));
static int __bam_cs_multiple_keyonly_next
			__P((BTREE_COMPRESS_STREAM *, DBT *, DBT *));
static void __bam_cs_create_multiple_keyonly
			__P((BTREE_COMPRESS_STREAM *, DBT *));
static int __bamc_compress_merge_insert
		__P((DBC *, BTREE_COMPRESS_STREAM *, u_int32_t *, u_int32_t));
static int __bamc_compress_merge_delete
		__P((DBC *, BTREE_COMPRESS_STREAM *, u_int32_t *));
static int __bamc_compress_merge_delete_dups
		__P((DBC *, BTREE_COMPRESS_STREAM *, u_int32_t *));

/* BTREE_COMPRESS_STREAM->next() for when the data has finished. */
static int
__bam_cs_next_done(stream, key, data)
	BTREE_COMPRESS_STREAM *stream;
	DBT *key, *data;
{
	COMPQUIET(stream, NULL);
	COMPQUIET(key, NULL);
	COMPQUIET(data, NULL);
	return (0);
}

/* BTREE_COMPRESS_STREAM->next() for a single key/data pair. */
static int
__bam_cs_single_next(stream, key, data)
	BTREE_COMPRESS_STREAM *stream;
	DBT *key, *data;
{
	key->data = stream->key->data;
	key->size = stream->key->size;
	data->data = stream->data->data;
	data->size = stream->data->size;
	stream->next = __bam_cs_next_done;
	return (1);
}

/* Create a BTREE_COMPRESS_STREAM for a single key/data pair */
static void
__bam_cs_create_single(stream, key, data)
	BTREE_COMPRESS_STREAM *stream;
	DBT *key, *data;
{
	stream->next = __bam_cs_single_next;
	stream->key = key;
	stream->data = data;
}

/* BTREE_COMPRESS_STREAM->next() for a single key. */
static int
__bam_cs_single_keyonly_next(stream, key, data)
	BTREE_COMPRESS_STREAM *stream;
	DBT *key, *data;
{
	key->data = stream->key->data;
	key->size = stream->key->size;
	if (data != NULL) {
		data->data = NULL;
		data->size = 0;
	}
	stream->next = __bam_cs_next_done;
	return (1);
}

/* Create a BTREE_COMPRESS_STREAM for a single key/data pair */
static void
__bam_cs_create_single_keyonly(stream, key)
	BTREE_COMPRESS_STREAM *stream;
	DBT *key;
{
	stream->next = __bam_cs_single_keyonly_next;
	stream->key = key;
}

/*
 * BTREE_COMPRESS_STREAM->next() for a single buffer in the DB_MULTIPLE_KEY
 * format.
 */
static int
__bam_cs_multiple_key_next(stream, key, data)
	BTREE_COMPRESS_STREAM *stream;
	DBT *key, *data;
{
	DB_MULTIPLE_KEY_NEXT(stream->kptr, stream->key, key->data, key->size,
		data->data, data->size);
	if (key->data == NULL) {
		stream->next = __bam_cs_next_done;
		return (0);
	}
	return (1);
}

/*
 * Create a BTREE_COMPRESS_STREAM for a single buffer in the DB_MULTIPLE_KEY
 * format.
 */
static void
__bam_cs_create_multiple_key(stream, multiple)
	BTREE_COMPRESS_STREAM *stream;
	DBT *multiple;
{
	stream->next = __bam_cs_multiple_key_next;
	stream->key = multiple;
	DB_MULTIPLE_INIT(stream->kptr, stream->key);
}

/* BTREE_COMPRESS_STREAM->next() for two buffers in the DB_MULTIPLE format. */
static int
__bam_cs_multiple_next(stream, key, data)
	BTREE_COMPRESS_STREAM *stream;
	DBT *key, *data;
{
	DB_MULTIPLE_NEXT(stream->kptr, stream->key, key->data, key->size);
	DB_MULTIPLE_NEXT(stream->dptr, stream->data, data->data, data->size);
	if (key->data == NULL || data->data == NULL) {
		stream->next = __bam_cs_next_done;
		return (0);
	}
	return (1);
}

/* Create a BTREE_COMPRESS_STREAM for two buffers in the DB_MULTIPLE format. */
static void
__bam_cs_create_multiple(stream, key, data)
	BTREE_COMPRESS_STREAM *stream;
	DBT *key, *data;
{
	stream->next = __bam_cs_multiple_next;
	stream->key = key;
	stream->data = data;
	DB_MULTIPLE_INIT(stream->kptr, stream->key);
	DB_MULTIPLE_INIT(stream->dptr, stream->data);
}

/*
 * BTREE_COMPRESS_STREAM->next() for a single buffer in the DB_MULTIPLE
 * format.
 */
static int
__bam_cs_multiple_keyonly_next(stream, key, data)
	BTREE_COMPRESS_STREAM *stream;
	DBT *key, *data;
{
	DB_MULTIPLE_NEXT(stream->kptr, stream->key, key->data, key->size);
	if (key->data == NULL) {
		stream->next = __bam_cs_next_done;
		return (0);
	}
	if (data != NULL) {
		data->data = NULL;
		data->size = 0;
	}
	return (1);
}

/*
 * Create a BTREE_COMPRESS_STREAM for a single buffer in the DB_MULTIPLE
 * format.
 */
static void
__bam_cs_create_multiple_keyonly(stream, key)
	BTREE_COMPRESS_STREAM *stream;
	DBT *key;
{
	stream->next = __bam_cs_multiple_keyonly_next;
	stream->key = key;
	DB_MULTIPLE_INIT(stream->kptr, stream->key);
}

/******************************************************************************/

/*
 * Marshal data in initial data format into destbuf, resizing destbuf if
 * necessary.
 */
static int
__bam_compress_marshal_data(dbp, data, destbuf)
	DB *dbp;
	const DBT *data;
	DBT *destbuf;
{
	int ret;
	u_int8_t *ptr;

	ret = 0;
	DB_ASSERT(dbp->env, F_ISSET(destbuf, DB_DBT_USERMEM));

	destbuf->size = __db_compress_count_int(data->size);
	destbuf->size += data->size;
	if (CMP_RESIZE_DBT(ret, dbp->env, destbuf) != 0)
		return (ret);

	ptr = (u_int8_t*)destbuf->data;
	ptr += __db_compress_int(ptr, data->size);
	memcpy(ptr, data->data, data->size);

	return (0);
}

/*
 * Unmarshal initial data from source into data - does not copy, points
 * into source.
 */
#define	CMP_UNMARSHAL_DATA(src, dest) do {				\
	(dest)->data = ((u_int8_t*)(src)->data) +			\
		__db_decompress_int32((u_int8_t*)(src)->data,		\
			&(dest)->size);					\
} while (0)

/******************************************************************************/

/*
 * __bam_compress_dupcmp --
 *	Duplicate comparison function for compressed BTrees.
 *
 * PUBLIC: int __bam_compress_dupcmp __P((DB *, const DBT *, const DBT *));
 */
int
__bam_compress_dupcmp(db, a, b)
	DB *db;
	const DBT *a;
	const DBT *b;
{
	DBT dcmp_a, dcmp_b;

	/* Decompress the initial data in a */
	CMP_UNMARSHAL_DATA(a, &dcmp_a);
	dcmp_a.ulen = 0;
	dcmp_a.doff = 0;
	dcmp_a.dlen = 0;
	dcmp_a.flags = 0;
	dcmp_a.app_data = 0;

	/* Decompress the initial data in b */
	CMP_UNMARSHAL_DATA(b, &dcmp_b);
	dcmp_b.ulen = 0;
	dcmp_b.doff = 0;
	dcmp_b.dlen = 0;
	dcmp_b.flags = 0;
	dcmp_b.app_data = 0;

	/* Call the user's duplicate compare function */
	return ((BTREE *)db->bt_internal)->
		compress_dup_compare(db, &dcmp_a, &dcmp_b);
}

/*
 * __bam_defcompress --
 *	Default compression routine.
 *
 * PUBLIC: int __bam_defcompress __P((DB *, const DBT *, const DBT *,
 * PUBLIC:     const DBT *, const DBT *, DBT *));
 */
int
__bam_defcompress(dbp, prevKey, prevData, key, data, dest)
	DB *dbp;
	const DBT *prevKey, *prevData, *key, *data;
	DBT *dest;
{
	u_int8_t *ptr;
	const u_int8_t *k, *p;
	size_t len, prefix, suffix;

	COMPQUIET(dbp, NULL);

	k = (const u_int8_t*)key->data;
	p = (const u_int8_t*)prevKey->data;
	len = key->size > prevKey->size ? prevKey->size : key->size;
	for (; len-- && *k == *p; ++k, ++p)
		continue;

	prefix = (size_t)(k - (u_int8_t*)key->data);
	suffix = key->size - prefix;

	if (prefix == prevKey->size && suffix == 0) {
		/* It's a duplicate - do prefix compression on the value */
		k = (const u_int8_t*)data->data;
		p = (const u_int8_t*)prevData->data;
		len = data->size > prevData->size ? prevData->size : data->size;
		for (; len-- && *k == *p; ++k, ++p)
			continue;

		prefix = (size_t)(k - (u_int8_t*)data->data);
		suffix = data->size - prefix;

		/* Check that we have enough space in dest */
		dest->size = (u_int32_t)(1 + __db_compress_count_int(prefix) +
			__db_compress_count_int(suffix) + suffix);
		if (dest->size > dest->ulen)
			return (DB_BUFFER_SMALL);

		/* Magic identifying byte */
		ptr = (u_int8_t*)dest->data;
		*ptr = CMP_INT_SPARE_VAL;
		++ptr;

		/* prefix length */
		ptr += __db_compress_int(ptr, prefix);

		/* suffix length */
		ptr += __db_compress_int(ptr, suffix);

		/* suffix */
		memcpy(ptr, k, suffix);

		return (0);
	}

	/* Check that we have enough space in dest */
	dest->size = (u_int32_t)(__db_compress_count_int(prefix) +
		__db_compress_count_int(suffix) +
		__db_compress_count_int(data->size) + suffix + data->size);
	if (dest->size > dest->ulen)
		return (DB_BUFFER_SMALL);

	/* prefix length */
	ptr = (u_int8_t*)dest->data;
	ptr += __db_compress_int(ptr, prefix);

	/* suffix length */
	ptr += __db_compress_int(ptr, suffix);

	/* data length */
	ptr += __db_compress_int(ptr, data->size);

	/* suffix */
	memcpy(ptr, k, suffix);
	ptr += suffix;

	/* data */
	memcpy(ptr, data->data, data->size);

	return (0);
}

/*
 * __bam_defdecompress --
 *	Default decompression routine.
 *
 * PUBLIC: int __bam_defdecompress __P((DB *, const DBT *, const DBT *, DBT *,
 * PUBLIC:     DBT *, DBT *));
 */
int
__bam_defdecompress(dbp, prevKey, prevData, compressed, destKey, destData)
	DB *dbp;
	const DBT *prevKey, *prevData;
	DBT *compressed, *destKey, *destData;
{
	u_int8_t *s, *d;
	u_int32_t prefix, suffix, size;

	COMPQUIET(dbp, NULL);

	/*
	 * Check for the magic identifying byte, that tells us that this is a
	 * compressed duplicate value.
	 */
	s = (u_int8_t*)compressed->data;
	if (*s == CMP_INT_SPARE_VAL) {
		++s;
		size = 1;

		/* Unmarshal prefix and suffix */
		size += __db_decompress_count_int(s);
		if (size > compressed->size)
			return (EINVAL);
		s += __db_decompress_int32(s, &prefix);

		size += __db_decompress_count_int(s);
		if (size > compressed->size)
			return (EINVAL);
		s += __db_decompress_int32(s, &suffix);

		/* Check destination lengths */
		destKey->size = prevKey->size;
		destData->size = prefix + suffix;
		if (destKey->size > destKey->ulen ||
			destData->size > destData->ulen)
			return (DB_BUFFER_SMALL);

		/* Write the key */
		memcpy(destKey->data, prevKey->data, destKey->size);

		/* Write the prefix */
		if (prefix > prevData->size)
			return (EINVAL);
		d = (u_int8_t*)destData->data;
		memcpy(d, prevData->data, prefix);
		d += prefix;

		/* Write the suffix */
		size += suffix;
		if (size > compressed->size)
			return (EINVAL);
		memcpy(d, s, suffix);
		s += suffix;

		/* Return bytes read */
		compressed->size = (u_int32_t)(s - (u_int8_t*)compressed->data);
		return (0);
	}

	/* Unmarshal prefix, suffix and data length */
	size = __db_decompress_count_int(s);
	if (size > compressed->size)
		return (EINVAL);
	s += __db_decompress_int32(s, &prefix);

	size += __db_decompress_count_int(s);
	if (size > compressed->size)
		return (EINVAL);
	s += __db_decompress_int32(s, &suffix);

	size += __db_decompress_count_int(s);
	if (size > compressed->size)
		return (EINVAL);
	s += __db_decompress_int32(s, &destData->size);

	/* Check destination lengths */
	destKey->size = prefix + suffix;
	if (destKey->size > destKey->ulen || destData->size > destData->ulen)
		return (DB_BUFFER_SMALL);

	/* Write the prefix */
	if (prefix > prevKey->size)
		return (EINVAL);
	d = (u_int8_t*)destKey->data;
	memcpy(d, prevKey->data, prefix);
	d += prefix;

	/* Write the suffix */
	size += suffix;
	if (size > compressed->size)
		return (EINVAL);
	memcpy(d, s, suffix);
	s += suffix;

	/* Write the data */
	size += destData->size;
	if (size > compressed->size)
		return (EINVAL);
	memcpy(destData->data, s, destData->size);
	s += destData->size;

	/* Return bytes read */
	compressed->size = (u_int32_t)(s - (u_int8_t*)compressed->data);
	return (0);
}

/******************************************************************************/

/*
 * Set dbc up to start decompressing the compressed key/data pair, dbc->key1
 * and dbc->compressed.
 */
static int
__bamc_start_decompress(dbc)
	DBC *dbc;
{
	BTREE_CURSOR *cp;
	int ret;
	u_int32_t datasize;

	cp = (BTREE_CURSOR *)dbc->internal;

	cp->prevKey = NULL;
	cp->prevData = NULL;
	cp->currentKey = &cp->key1;
	cp->currentData = &cp->data1;
	cp->compcursor = (u_int8_t*)cp->compressed.data;
	cp->compend = cp->compcursor + cp->compressed.size;
	cp->prevcursor = NULL;
	cp->prev2cursor = NULL;

	/* Unmarshal the first data */
	cp->compcursor += __db_decompress_int32(cp->compcursor, &datasize);
	ret = __bam_compress_set_dbt(dbc->dbp,
	    cp->currentData, cp->compcursor, datasize);

	if (ret == 0)
	     cp->compcursor += datasize;
	return (ret);
}

/* Decompress the next key/data pair from dbc->compressed. */
static int
__bamc_next_decompress(dbc)
	DBC *dbc;
{
	DBT compressed;
	int ret;
	BTREE_CURSOR *cp;
	DB *db;

	ret = 0;
	cp = (BTREE_CURSOR *)dbc->internal;
	db = dbc->dbp;

	if (cp->compcursor >= cp->compend)
		return (DB_NOTFOUND);

	cp->prevKey = cp->currentKey;
	cp->prevData = cp->currentData;
	cp->prev2cursor = cp->prevcursor;
	cp->prevcursor = cp->compcursor;

	if (cp->currentKey == &cp->key1) {
		cp->currentKey = &cp->key2;
		cp->currentData = &cp->data2;
	} else {
		cp->currentKey = &cp->key1;
		cp->currentData = &cp->data1;
	}

	compressed.flags = DB_DBT_USERMEM;
	compressed.data = (void*)cp->compcursor;
	compressed.ulen = compressed.size =
	    (u_int32_t)(cp->compend - cp->compcursor);
	compressed.app_data = NULL;

	while ((ret = ((BTREE *)db->bt_internal)->bt_decompress(db,
	     cp->prevKey, cp->prevData, &compressed,
	     cp->currentKey, cp->currentData)) == DB_BUFFER_SMALL) {
		if (CMP_RESIZE_DBT(ret, dbc->env, cp->currentKey) != 0)
			break;
		if (CMP_RESIZE_DBT(ret, dbc->env, cp->currentData) != 0)
			break;
	}

	if (ret == 0)
		cp->compcursor += compressed.size;
	return (ret);
}

/*
 * Store key and data into destkey and destbuf, using the compression
 * callback given.
 */
static int
__bamc_compress_store(dbc, key, data, prevKey, prevData, destkey, destbuf)
	DBC *dbc;
	DBT *key, *data;
	DBT **prevKey, **prevData;
	DBT *destkey, *destbuf;
{
	int ret;
	DBT dest;

	if (*prevKey == 0) {
		if ((ret = __bam_compress_set_dbt(dbc->dbp,
		    destkey, key->data, key->size)) != 0)
			return (ret);

		/* Marshal data - resize if it won't fit */
		ret = __bam_compress_marshal_data(dbc->dbp, data, destbuf);

	} else if (((BTREE_CURSOR *)dbc->internal)->ovflsize > destbuf->size) {
		/*
		 * Don't write more than cp->ovflsize bytes to the destination
		 * buffer - destbuf must be at least cp->ovflsize in size.
		 */
		dest.flags = DB_DBT_USERMEM;
		dest.data = (u_int8_t*)destbuf->data + destbuf->size;
		dest.ulen =
		    ((BTREE_CURSOR *)dbc->internal)->ovflsize - destbuf->size;
		dest.size = 0;
		dest.app_data = NULL;

		ret = ((BTREE *)dbc->dbp->bt_internal)->bt_compress(
		    dbc->dbp, *prevKey, *prevData, key, data, &dest);

		if (ret == 0)
			destbuf->size += dest.size;
	} else
		ret = DB_BUFFER_SMALL;

	if (ret == 0) {
		*prevKey = key;
		*prevData = data;
	}

	return (ret);
}

/*
 * Move dbc->dbc to the correct position to start linear searching for
 * seek_key/seek_data - the biggest key smaller than or equal to
 * seek_key/seek_data.
 */
static int
__bamc_compress_seek(dbc, seek_key, seek_data, flags)
	DBC *dbc;
	const DBT *seek_key;
	const DBT *seek_data;
	u_int32_t flags;
{
	int ret;
	u_int32_t method;
	DB *dbp;
	BTREE_CURSOR *cp;

	dbp = dbc->dbp;
	cp = (BTREE_CURSOR *)dbc->internal;

	if ((ret = __bam_compress_set_dbt(
	    dbp, &cp->key1, seek_key->data, seek_key->size)) != 0)
		return (ret);

	/*
	 * We allow seek_data to be 0 for __bamc_compress_get_set() with
	 * DB_SET
	 */
	if (F_ISSET(dbp, DB_AM_DUPSORT) && seek_data != NULL) {
		if ((ret = __bam_compress_marshal_data(
		    dbp, seek_data, &cp->compressed)) != 0)
			return (ret);

		method = DB_GET_BOTH_LTE;
	} else
		method = DB_SET_LTE;

	CMP_IGET_RETRY(ret, dbc, &cp->key1, &cp->compressed, method | flags);

	if (ret == 0 &&
	    F_ISSET(dbp, DB_AM_DUPSORT) && seek_data == NULL &&
	    __db_compare_both(dbp, seek_key, 0, &cp->key1, 0) == 0) {
		/*
		 * Some entries for seek_key might be in the previous chunk,
		 * so we need to start searching there.
		 */
		CMP_IGET_RETRY(ret,
		    dbc, &cp->key1, &cp->compressed, DB_PREV | flags);
		if (ret == DB_NOTFOUND) {
			/* No previous, we must need the first entry */
			CMP_IGET_RETRY(ret,
			    dbc, &cp->key1, &cp->compressed, DB_FIRST | flags);
		}
	}

	return (ret);
}

/* Reset the cursor to an uninitialized state */
static void
__bamc_compress_reset(dbc)
	DBC *dbc;
{
	BTREE_CURSOR *cp;

	cp = (BTREE_CURSOR *)dbc->internal;

	cp->prevKey = 0;
	cp->prevData = 0;
	cp->currentKey = 0;
	cp->currentData = 0;
	cp->compcursor = 0;
	cp->compend = 0;
	cp->prevcursor = 0;
	cp->prev2cursor = 0;

	F_CLR(cp, C_COMPRESS_DELETED|C_COMPRESS_MODIFIED);
}

/*
 * Duplicate the cursor and delete the current entry, move the original cursor
 * on and then close the cursor we used to delete. We do that to make sure that
 * the close method runs __bamc_physdel(), and actually gets rid of the deleted
 * entry!
 */
static int
__bamc_compress_del_and_get_next(dbc, nextk, nextc)
     DBC *dbc;
     DBT *nextk, *nextc;
{
	int ret, ret_n;
	DBC *dbc_n;

	if ((ret = __dbc_dup(dbc, &dbc_n, DB_POSITION | DB_SHALLOW_DUP)) != 0)
		return (ret);
	F_SET(dbc_n, DBC_TRANSIENT);

	if ((ret = __dbc_idel(dbc_n, 0)) != 0)
		goto err;

	/* Read the next position */
	CMP_IGET_RETRY(ret, dbc, nextk, nextc, DB_NEXT);

 err:
	if ((ret_n = __dbc_close(dbc_n)) != 0 && ret == 0)
		ret = ret_n;

	/* No need to relocate this cursor */
	F_CLR((BTREE_CURSOR *)dbc->internal, C_COMPRESS_MODIFIED);

	return (ret);
}

/*
 * Duplicate the cursor, re-locate the position that this cursor pointed to
 * using the duplicate (it may have been deleted), and then swap
 * the cursors. We do that to make sure that the close method runs
 * __bamc_physdel(), and gets rid of the entry that may have been deleted.
 */
static int
__bamc_compress_relocate(dbc)
	DBC *dbc;
{
	int ret, t_ret;
	BTREE_CURSOR *cp, *cp_n;
	DBC *dbc_n;

	cp = (BTREE_CURSOR *)dbc->internal;

	if ((ret = __dbc_dup(dbc, &dbc_n, 0)) != 0)
		return (ret);
	F_SET(dbc_n, DBC_TRANSIENT);

	cp_n = (BTREE_CURSOR *)dbc_n->internal;

	if (F_ISSET(cp, C_COMPRESS_DELETED)) {
		/* Find the position after the deleted entry again */
		ret = __bamc_compress_get_set(
		    dbc_n, &cp->del_key, &cp->del_data, 0, 0);
		if (ret == DB_NOTFOUND) {
			__bamc_compress_reset(dbc_n);
			ret = 0;
		} else if (ret != 0)
			goto err;

		F_SET(cp_n, C_COMPRESS_DELETED);

	} else if (cp->currentKey != NULL) {
		/* Find the current entry again */
		ret = __bamc_compress_get_set(
		    dbc_n, cp->currentKey, cp->currentData,
		    F_ISSET(dbc->dbp, DB_AM_DUPSORT) ? DB_GET_BOTH : DB_SET, 0);

		if (ret == DB_NOTFOUND) {
			/* The current entry has been deleted */
			if ((ret = __bam_compress_set_dbt(dbc_n->dbp,
			     &cp_n->del_key,
			     cp->currentKey->data, cp->currentKey->size)) != 0)
				return (ret);
			if ((ret = __bam_compress_set_dbt(dbc_n->dbp,
			     &cp_n->del_data, cp->currentData->data,
			     cp->currentData->size)) != 0)
				return (ret);
			F_SET(cp_n, C_COMPRESS_DELETED);
			ret = 0;
		} else if (ret != 0)
			goto err;
	}

 err:
	/* Cleanup and cursor resolution. This also clears the
	   C_COMPRESS_MODIFIED flag. */
	if ((t_ret = __dbc_cleanup(dbc, dbc_n, ret)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

/******************************************************************************/

#define	CMP_STORE(key, data) do {					    \
	while ((ret = __bamc_compress_store(dbc, (key), (data),             \
	    &prevDestKey, &prevDestData, &destkey, &destbuf))               \
	    == DB_BUFFER_SMALL) {					    \
		if ((ret = __dbc_iput(dbc,                                  \
		    &destkey, &destbuf, DB_KEYLAST)) != 0)                  \
			goto end;					    \
		prevDestKey = NULL;					    \
		prevDestData = NULL;					    \
		destbuf.size = 0;					    \
	}								    \
} while (0)

/* Merge the sorted key/data pairs from stream into the compressed database. */
static int
__bamc_compress_merge_insert(dbc, stream, countp, flags)
	DBC *dbc;
	BTREE_COMPRESS_STREAM *stream;
	u_int32_t *countp;
	u_int32_t flags;
{
	DBT ikey1, ikey2, idata1, idata2, nextk, nextc, nextd, destkey, destbuf;
	DBT *ikey, *idata, *prevIkey, *prevIdata, *prevDestKey, *prevDestData;
	int ret, bulk_ret, cmp, nextExists, moreCompressed, iSmallEnough;
	int moreStream;
	u_int32_t chunk_count;
	ENV *env;
	BTREE_CURSOR *cp;
	DB *dbp;

	env = dbc->env;
	cp = (BTREE_CURSOR *)dbc->internal;
	dbp = dbc->dbp;
	bulk_ret = 0;

	memset(&ikey1, 0, sizeof(DBT));
	memset(&ikey2, 0, sizeof(DBT));
	memset(&idata1, 0, sizeof(DBT));
	memset(&idata2, 0, sizeof(DBT));

	CMP_INIT_DBT(&nextk);
	CMP_INIT_DBT(&nextc);
	memset(&nextd, 0, sizeof(DBT));

	CMP_INIT_DBT(&destkey);
	CMP_INIT_DBT(&destbuf);
	if ((ret = __os_malloc(env, cp->ovflsize, &destbuf.data)) != 0)
		goto end;
	destbuf.ulen = cp->ovflsize;

	if (countp != NULL)
		*countp = 0;
	chunk_count = 0;

	/* Get the first input key and data */
	ret = 0;
	prevIkey = NULL;
	prevIdata = NULL;
	ikey = &ikey1;
	idata = &idata1;
	if (stream->next(stream, ikey, idata) == 0)
		goto end;

	prevDestKey = NULL;
	prevDestData = NULL;

	moreStream = 1;
	while (moreStream != 0) {
		nextExists = 1;
		moreCompressed = 1;

		/* Seek the ikey/idata position */
		ret = __bamc_compress_seek(dbc, ikey, idata, 0);
		if (ret == 0) {
			/*
			 * Delete the key - we might overwrite it below
			 * but it's safer to just always delete it, and it
			 * doesn't seem significantly slower to do so.
			 */
			ret = __bamc_compress_del_and_get_next(dbc, &nextk,
				&nextc);
			if (ret == DB_NOTFOUND) {
				ret = 0;
				nextExists = 0;
			} else if (ret == 0) {
				CMP_UNMARSHAL_DATA(&nextc, &nextd);
			} else
				goto end;
			ret = __bamc_start_decompress(dbc);
		} else if (ret == DB_NOTFOUND) {
			moreCompressed = 0;

			/* Read the next position */
			CMP_IGET_RETRY(ret, dbc, &nextk, &nextc, DB_FIRST);
			if (ret == DB_NOTFOUND) {
				ret = 0;
				nextExists = 0;
			} else if (ret == 0) {
				CMP_UNMARSHAL_DATA(&nextc, &nextd);
			}
		}

		if (ret != 0)
			goto end;

		/* !nextExists || ikey/idata < nextk/nextd */
		iSmallEnough = 1;

		while (moreCompressed != 0 || iSmallEnough != 0) {
			if (moreCompressed == 0)
				cmp = 1;
			else if (iSmallEnough == 0)
				cmp = -1;
			else
				cmp = __db_compare_both(dbp, cp->currentKey,
					cp->currentData, ikey, idata);

			if (cmp < 0) {
store_current:			CMP_STORE(cp->currentKey, cp->currentData);
				if (ret != 0)
					goto end;
			} else {
				switch (flags) {
				case DB_KEYLAST:
				case DB_KEYFIRST:
				case DB_NODUPDATA:
					if (cmp == 0 && bulk_ret == 0 &&
						F_ISSET(dbp, DB_AM_DUPSORT)) {
						bulk_ret = __db_duperr(dbp,
							flags);

						/*
						 * Continue until we store
						 * the current chunk,
						 * but don't insert any
						 * more entries.
						 */
						moreStream = 0;
						iSmallEnough = 0;

						goto store_current;
					}
					break;
				default:
					break;
				}

				CMP_STORE(ikey, idata);
				if (ret != 0)
					goto end;
				++chunk_count;

				/*
				 * prevDestKey/prevDestData now point to
				 * the same DBTs as ikey/idata. We don't
				 * want to overwrite them, so swap them
				 * to point to the other DBTs.
				 */
				if (ikey == &ikey1) {
					ikey = &ikey2;
					idata = &idata2;
					prevIkey = &ikey1;
					prevIdata = &idata1;
				} else {
					ikey = &ikey1;
					idata = &idata1;
					prevIkey = &ikey2;
					prevIdata = &idata2;
				}

				do {
					/* Get the next input key and data */
					if (stream->next(
					    stream, ikey, idata) == 0) {
						moreStream = 0;
						iSmallEnough = 0;
						break;
					}

#ifdef DIAGNOSTIC
					/* Check that the stream is sorted */
					DB_ASSERT(env, __db_compare_both(dbp,
						ikey, idata, prevIkey,
						prevIdata) >= 0);
#endif

					/* Check for duplicates in the stream */
				} while (__db_compare_both(dbp, ikey, idata,
						 prevIkey, prevIdata) == 0);

				/*
				 * Check that !nextExists ||
				 * ikey/idata < nextk/nextd
				 */
				if (moreStream != 0 && nextExists != 0 &&
					__db_compare_both(dbp, ikey,
						idata, &nextk, &nextd) >= 0)
					iSmallEnough = 0;
			}

			if (cmp <= 0) {
				ret = __bamc_next_decompress(dbc);
				if (ret == DB_NOTFOUND) {
					moreCompressed = 0;
					ret = 0;
				} else if (ret != 0)
					goto end;

			}
		}

		if (prevDestKey != NULL) {
			if ((ret = __dbc_iput(
			    dbc, &destkey, &destbuf, DB_KEYLAST)) != 0)
				goto end;

			if (countp != NULL)
				*countp += chunk_count;
			chunk_count = 0;

			prevDestKey = NULL;
			prevDestData = NULL;
			destbuf.size = 0;
		}
	}

 end:
	CMP_FREE_DBT(env, &destkey);
	CMP_FREE_DBT(env, &destbuf);
	CMP_FREE_DBT(env, &nextk);
	CMP_FREE_DBT(env, &nextc);

	return (ret != 0 ? ret : bulk_ret);
}

/******************************************************************************/

/* Remove the sorted key/data pairs in stream from the compressed database. */
static int
__bamc_compress_merge_delete(dbc, stream, countp)
	DBC *dbc;
	BTREE_COMPRESS_STREAM *stream;
	u_int32_t *countp;
{
	DBT ikey, idata, nextk, nextc, nextd, destkey, destbuf, pdestkey;
	DBT pdestdata;
#ifdef DIAGNOSTIC
	DBT pikey, pidata;
#endif
	DBT *prevDestKey, *prevDestData;
	int ret, bulk_ret, cmp, moreCompressed, moreStream, nextExists;
	int iSmallEnough;
	u_int32_t chunk_count;
	ENV *env;
	BTREE_CURSOR *cp;
	DB *dbp;

	env = dbc->env;
	cp = (BTREE_CURSOR *)dbc->internal;
	dbp = dbc->dbp;
	bulk_ret = 0;

	memset(&ikey, 0, sizeof(DBT));
	memset(&idata, 0, sizeof(DBT));

	CMP_INIT_DBT(&nextk);
	CMP_INIT_DBT(&nextc);
	memset(&nextd, 0, sizeof(DBT));

	CMP_INIT_DBT(&pdestkey);
	CMP_INIT_DBT(&pdestdata);

	CMP_INIT_DBT(&destkey);
	CMP_INIT_DBT(&destbuf);
	if ((ret = __os_malloc(env, cp->ovflsize, &destbuf.data)) != 0)
		goto end;
	destbuf.ulen = cp->ovflsize;

	if (countp != NULL)
		*countp = 0;
	chunk_count = 0;

	/* Get the first input key and data */
	ret = 0;
	if (stream->next(stream, &ikey, &idata) == 0)
		goto end;

	prevDestKey = NULL;
	prevDestData = NULL;

	moreStream = 1;
	while (moreStream != 0) {
		nextExists = 1;
		moreCompressed = 1;

		/* Seek the ikey/idata position */
		if ((ret = __bamc_compress_seek(dbc, &ikey, &idata, 0)) != 0)
			goto end;

		/*
		 * Delete the key - we might overwrite it below but it's safer
		 * to just always delete it, and it doesn't seem significantly
		 * slower to do so.
		  */
		ret = __bamc_compress_del_and_get_next(dbc, &nextk, &nextc);
		if (ret == DB_NOTFOUND) {
			ret = 0;
			nextExists = 0;
		} else if (ret == 0) {
			CMP_UNMARSHAL_DATA(&nextc, &nextd);
		} else
			goto end;

		if ((ret = __bamc_start_decompress(dbc)) != 0)
			goto end;

		/* !nextExists || ikey/idata < nextk/nextd */
		iSmallEnough = 1;

		while (moreCompressed != 0 || iSmallEnough != 0) {
			if (moreCompressed == 0)
				cmp = 1;
			else if (iSmallEnough == 0)
				cmp = -1;
			else
				cmp = __db_compare_both(dbp, cp->currentKey,
					cp->currentData, &ikey, &idata);

			if (cmp < 0) {
				if ((ret = __bamc_compress_store(dbc,
				    cp->currentKey, cp->currentData,
				    &prevDestKey, &prevDestData,
				    &destkey, &destbuf)) != 0)
					goto end;

				if ((ret = __bam_compress_set_dbt(dbp,
				    &pdestkey, cp->currentKey->data,
				    cp->currentKey->size)) != 0)
					goto end;
				if ((ret = __bam_compress_set_dbt(dbp,
				    &pdestdata, cp->currentData->data,
				    cp->currentData->size)) != 0)
					goto end;
				prevDestKey = &pdestkey;
				prevDestData = &pdestdata;
			} else {
				if (cmp != 0) {
					/*
					 * Continue until we store the current
					 * chunk, but don't delete any more
					 * entries.
					 */
					bulk_ret = DB_NOTFOUND;
					moreStream = 0;
					iSmallEnough = 0;
				} else
					++chunk_count;

#ifdef DIAGNOSTIC
				pikey = ikey;
				pidata = idata;
#endif

				/* Get the next input key and data */
				if (stream->next(stream, &ikey, &idata) == 0) {
					moreStream = 0;
					iSmallEnough = 0;
				}

#ifdef DIAGNOSTIC
				/* Check that the stream is sorted */
				DB_ASSERT(env, moreStream == 0 ||
				    __db_compare_both(dbp, &ikey, &idata,
				    &pikey, &pidata) >= 0);
#endif

				/*
				 * Check that !nextExists ||
				 * ikey/idata < nextk/nextd
				 */
				if (moreStream != 0 && nextExists != 0 &&
					__db_compare_both(dbp, &ikey,
					    &idata, &nextk, &nextd) >= 0)
					iSmallEnough = 0;
			}

			if (cmp <= 0) {
				ret = __bamc_next_decompress(dbc);
				if (ret == DB_NOTFOUND) {
					moreCompressed = 0;
					ret = 0;
				} else if (ret != 0)
					goto end;
			}
		}

		if (prevDestKey != NULL) {
			if ((ret = __dbc_iput(
			    dbc, &destkey, &destbuf, DB_KEYLAST)) != 0)
				goto end;

			if (countp)
				*countp += chunk_count;
			chunk_count = 0;

			prevDestKey = NULL;
			prevDestData = NULL;
			destbuf.size = 0;
		}
	}

 end:
	CMP_FREE_DBT(env, &destkey);
	CMP_FREE_DBT(env, &destbuf);
	CMP_FREE_DBT(env, &pdestkey);
	CMP_FREE_DBT(env, &pdestdata);
	CMP_FREE_DBT(env, &nextk);
	CMP_FREE_DBT(env, &nextc);

	return (ret != 0 ? ret : bulk_ret);
}

/*
 * Remove the sorted keys in stream along with all duplicate values from
 * the compressed database.
 */
static int
__bamc_compress_merge_delete_dups(dbc, stream, countp)
	DBC *dbc;
	BTREE_COMPRESS_STREAM *stream;
	u_int32_t *countp;
{
	DBC *dbc_n;
	DBT ikey, nextk, noread, destkey, destbuf, pdestkey, pdestdata;
#ifdef DIAGNOSTIC
	DBT pikey;
#endif
	DBT *prevDestKey, *prevDestData;
	int ret, ret_n, bulk_ret, cmp, moreCompressed, moreStream, nextExists;
	int iSmallEnough, ifound;
	u_int32_t chunk_count;
	ENV *env;
	BTREE_CURSOR *cp;
	DB *dbp;

	env = dbc->env;
	cp = (BTREE_CURSOR *)dbc->internal;
	dbp = dbc->dbp;
	bulk_ret = 0;

	memset(&ikey, 0, sizeof(DBT));

	CMP_INIT_DBT(&nextk);

	memset(&noread, 0, sizeof(DBT));
	noread.flags = DB_DBT_PARTIAL | DB_DBT_USERMEM;

	CMP_INIT_DBT(&pdestkey);
	CMP_INIT_DBT(&pdestdata);

	CMP_INIT_DBT(&destkey);
	CMP_INIT_DBT(&destbuf);
	if ((ret = __os_malloc(env, cp->ovflsize, &destbuf.data)) != 0)
		goto end;
	destbuf.ulen = cp->ovflsize;

	if (countp != NULL)
		*countp = 0;
	chunk_count = 0;

	/* Get the first input key and data */
	ret = 0;
	if (stream->next(stream, &ikey, NULL) == 0)
		goto end;
	ifound = 0;

	prevDestKey = NULL;
	prevDestData = NULL;

	moreStream = 1;
	iSmallEnough = 0;
	nextExists = 0;
	while (moreStream != 0) {
		if (iSmallEnough != 0) {
			if (nextExists == 0) {
				/*
				 * We've finished deleting the last key
				 * in the database
				 */
				if (ifound == 0) {
					bulk_ret = DB_NOTFOUND;
				} else
					++chunk_count;
				break;
			}

			/* Move to the next chunk */
			CMP_IGET_RETRY(
			    ret, dbc, &cp->key1, &cp->compressed, DB_CURRENT);
			if (ret == DB_NOTFOUND) {
				ret = 0;
				break;
			} else if (ret != 0)
				goto end;
		} else
			/* Seek the ikey position */
			if ((ret =
			    __bamc_compress_seek(dbc, &ikey, NULL, 0)) != 0)
				goto end;

		nextExists = 1;
		moreCompressed = 1;

		/*
		 * Delete the key - we might overwrite it below but it's
		 * safer to just always delete it, and it doesn't seem
		 * significantly slower to do so.
		 */
		ret = __bamc_compress_del_and_get_next(dbc, &nextk, &noread);
		if (ret == DB_NOTFOUND) {
			ret = 0;
			nextExists = 0;
		} else if (ret != 0)
			goto end;

		if ((ret = __bamc_start_decompress(dbc)) != 0)
			goto end;

		/* !nextExists || ikey <= nextk */
		iSmallEnough = 1;

		while (moreCompressed != 0) {
			if (moreCompressed == 0)
				cmp = 1;
			else if (iSmallEnough == 0)
				cmp = -1;
			else
				cmp = __db_compare_both(
				    dbp, cp->currentKey, NULL, &ikey, NULL);

			if (cmp < 0) {
				if ((ret = __bamc_compress_store(dbc,
				    cp->currentKey, cp->currentData,
				    &prevDestKey,
				    &prevDestData, &destkey, &destbuf)) != 0)
					goto end;

				if ((ret = __bam_compress_set_dbt(dbp,
				    &pdestkey, cp->currentKey->data,
				    cp->currentKey->size)) != 0)
					goto end;
				if ((ret = __bam_compress_set_dbt(dbp,
				     &pdestdata, cp->currentData->data,
				     cp->currentData->size)) != 0)
					goto end;
				prevDestKey = &pdestkey;
				prevDestData = &pdestdata;
			} else if (cmp > 0) {
				if (ifound == 0) {
					/*
					 * Continue until we store the
					 * current chunk, but don't delete
					 * any more entries.
					 */
					bulk_ret = DB_NOTFOUND;
					moreStream = 0;
					iSmallEnough = 0;
				} else
					++chunk_count;

#ifdef DIAGNOSTIC
				pikey = ikey;
#endif

				/* Get the next input key */
				if (stream->next(stream, &ikey, NULL) == 0) {
					moreStream = 0;
					iSmallEnough = 0;
				}
				ifound = 0;

#ifdef DIAGNOSTIC
				/* Check that the stream is sorted */
				DB_ASSERT(env, moreStream == 0 ||
				    __db_compare_both(dbp, &ikey, NULL,
				    &pikey, NULL) >= 0);
#endif

				/* Check that !nextExists || ikey <= nextk */
				if (moreStream != 0 && nextExists != 0 &&
				     __db_compare_both(dbp,
				     &ikey, NULL, &nextk, NULL) > 0)
					iSmallEnough = 0;
			} else /* cmp == 0 */
				ifound = 1;

			if (cmp <= 0) {
				ret = __bamc_next_decompress(dbc);
				if (ret == DB_NOTFOUND) {
					moreCompressed = 0;
					ret = 0;
				} else if (ret != 0)
					goto end;
			}
		}

		if (prevDestKey != NULL) {
			/*
			 * Do the DBC->put() with a duplicate cursor, so that
			 * the main cursor's position isn't changed - we might
			 * need it to be the same in order to use DB_CURRENT
			 * above.
			 */
			if ((ret = __dbc_dup(dbc, &dbc_n, 0)) != 0)
				goto end;
			F_SET(dbc_n, DBC_TRANSIENT);

			ret = __dbc_iput(dbc_n, &destkey, &destbuf, DB_KEYLAST);

			if ((ret_n = __dbc_close(dbc_n)) != 0 && ret == 0)
				ret = ret_n;

			if (ret != 0)
				goto end;

			if (countp)
				*countp += chunk_count;
			chunk_count = 0;

			prevDestKey = NULL;
			prevDestData = NULL;
			destbuf.size = 0;
		}
	}

 end:
	CMP_FREE_DBT(env, &destkey);
	CMP_FREE_DBT(env, &destbuf);
	CMP_FREE_DBT(env, &pdestkey);
	CMP_FREE_DBT(env, &pdestdata);
	CMP_FREE_DBT(env, &nextk);

	return (ret != 0 ? ret : bulk_ret);
}

/******************************************************************************/

/* Implements DB_PREV and DB_LAST for __bamc_compress_get() */
static int
__bamc_compress_get_prev(dbc, flags)
	DBC *dbc;
	u_int32_t flags;
{
	int ret;
	u_int32_t tofind;
	BTREE_CURSOR *cp;

	ret = 0;
	cp = (BTREE_CURSOR *)dbc->internal;

	F_CLR(cp, C_COMPRESS_DELETED);

	if (cp->prevKey != NULL) {
		/* Return the stored previous key */
		cp->currentKey = cp->prevKey;
		cp->currentData = cp->prevData;
		cp->compcursor = cp->prevcursor;
		cp->prevKey = 0;
		cp->prevData = 0;
		cp->prevcursor = cp->prev2cursor;
		cp->prev2cursor = 0;
	} else {
		if (cp->currentKey == NULL) {
			/* No current key, so fetch the last key */
			flags |= DB_LAST;
			tofind = (u_int32_t)-1;
		} else if (cp->prevcursor == 0) {
			/*
			 * The current key is at the begining of the
			 * compressed block, so get the last key from the
			 * previous block
			 */
			flags |= DB_PREV;
			tofind = (u_int32_t)-1;
		} else {
			/*
			 * We have to search for the previous key in the
			 * current block
			 */
			flags |= DB_CURRENT;
			tofind = (u_int32_t)
			    (cp->prevcursor - (u_int8_t*)cp->compressed.data);
		}

		CMP_IGET_RETRY(ret, dbc, &cp->key1, &cp->compressed, flags);
		if (ret != 0)
			return (ret);

		/* Decompress until we reach tofind */
		ret = __bamc_start_decompress(dbc);
		while (ret == 0 && tofind > (u_int32_t)
			(cp->compcursor - (u_int8_t*)cp->compressed.data)) {
			ret = __bamc_next_decompress(dbc);
		}

		if (ret == DB_NOTFOUND)
			ret = 0;
	}

	return (ret);
}

/* Implements DB_PREV_DUP for __bamc_compress_get() */
static int
__bamc_compress_get_prev_dup(dbc, flags)
	DBC *dbc;
	u_int32_t flags;
{
	int ret;
	BTREE_CURSOR *cp;
	DB *dbp;
	BTREE *t;

	ret = 0;
	cp = (BTREE_CURSOR *)dbc->internal;
	dbp = dbc->dbp;
	t = (BTREE *)dbp->bt_internal;

	if (cp->currentKey == 0)
		return (EINVAL);

	/* If this is a deleted entry, del_key is already set, otherwise we
	   have to set it now */
	if (!F_ISSET(cp, C_COMPRESS_DELETED)) {
		if ((ret = __bam_compress_set_dbt(dbp, &cp->del_key,
			    cp->currentKey->data, cp->currentKey->size)) != 0)
			return (ret);
	}

	if ((ret = __bamc_compress_get_prev(dbc, flags)) != 0)
		return (ret);

	if (t->bt_compare(dbp, cp->currentKey, &cp->del_key) != 0)
		return (DB_NOTFOUND);

	return (0);
}

/* Implements DB_PREV_NODUP for __bamc_compress_get() */
static int
__bamc_compress_get_prev_nodup(dbc, flags)
	DBC *dbc;
	u_int32_t flags;
{
	int ret;
	BTREE_CURSOR *cp;
	DB *dbp;
	BTREE *t;

	cp = (BTREE_CURSOR *)dbc->internal;
	dbp = dbc->dbp;
	t = (BTREE *)dbp->bt_internal;

	if (cp->currentKey == 0)
		return (__bamc_compress_get_prev(dbc, flags));

	/*
	 * If this is a deleted entry, del_key is already set, otherwise we
	 * have to set it now.
	 */
	if (!F_ISSET(cp, C_COMPRESS_DELETED))
		if ((ret = __bam_compress_set_dbt(dbp, &cp->del_key,
		    cp->currentKey->data, cp->currentKey->size)) != 0)
			return (ret);

	/*
	 * Linear search for the next non-duplicate key - this is
	 * especially inefficient for DB_PREV_NODUP, since we have to
	 * decompress from the begining of the chunk to find previous
	 * key/data pairs. Instead we could check for key equality as we
	 * decompress.
	 */
	do
		if ((ret = __bamc_compress_get_prev(dbc, flags)) != 0)
			return (ret);
	while (t->bt_compare(dbp, cp->currentKey, &cp->del_key) == 0);

	return (0);
}

/* Implements DB_NEXT and DB_FIRST for __bamc_compress_get() */
static int
__bamc_compress_get_next(dbc, flags)
	DBC *dbc;
	u_int32_t flags;
{
	int ret;
	BTREE_CURSOR *cp;

	cp = (BTREE_CURSOR *)dbc->internal;

	if (F_ISSET(cp, C_COMPRESS_DELETED)) {
		if (cp->currentKey == 0)
			return (DB_NOTFOUND);
		F_CLR(cp, C_COMPRESS_DELETED);
		return (0);
	} else if (cp->currentKey) {
		ret = __bamc_next_decompress(dbc);
		if (ret != DB_NOTFOUND)
			return (ret);

		flags |= DB_NEXT;
	} else
		flags |= DB_FIRST;

	CMP_IGET_RETRY(ret, dbc, &cp->key1, &cp->compressed, flags);
	if (ret == DB_NOTFOUND) {
		/*
		 * Reset the cursor, so that
		 * __bamc_compress_get_multiple_key will end up pointing
		 * to the right place
		 */
		__bamc_compress_reset(dbc);
		return (DB_NOTFOUND);
	} else if (ret != 0)
		return (ret);

	ret = __bamc_start_decompress(dbc);

	return (ret);
}

/* Implements DB_NEXT_DUP for __bamc_compress_get() */
static int
__bamc_compress_get_next_dup(dbc, key, flags)
	DBC *dbc;
	DBT *key;
	u_int32_t flags;
{
	int ret;
	BTREE_CURSOR *cp;
	DB *dbp;
	BTREE *t;

	cp = (BTREE_CURSOR *)dbc->internal;
	dbp = dbc->dbp;
	t = (BTREE *)dbp->bt_internal;

	if (cp->currentKey == 0)
		return (EINVAL);

	if (F_ISSET(cp, C_COMPRESS_DELETED)) {
		/*
		 * Check that the next entry has the same key as the
		 * deleted entry.
		 */
		if (cp->currentKey == 0)
			return (DB_NOTFOUND);
		F_CLR(cp, C_COMPRESS_DELETED);
		return (t->bt_compare(dbp,
		    cp->currentKey, &cp->del_key) == 0 ? 0 : DB_NOTFOUND);
	}

	/* Check that the next entry has the same key as the previous entry */
	ret = __bamc_next_decompress(dbc);
	if (ret == 0 && t->bt_compare(dbp, cp->currentKey, cp->prevKey) != 0)
		return (DB_NOTFOUND);
	if (ret != DB_NOTFOUND)
		return (ret);

	if (key == NULL) {
		/* Copy the current key to del_key */
		if ((ret = __bam_compress_set_dbt(dbp, &cp->del_key,
		    cp->currentKey->data, cp->currentKey->size)) != 0)
			return (ret);
		key = &cp->del_key;
	}

	/* Fetch the next chunk */
	CMP_IGET_RETRY(ret, dbc, &cp->key1, &cp->compressed, DB_NEXT | flags);
	if (ret == DB_NOTFOUND) {
		/*
		 * Reset the cursor, so that __bamc_compress_get_multiple
		 * will end up pointing to the right place
		 */
		__bamc_compress_reset(dbc);
		return (DB_NOTFOUND);
	} else if (ret != 0)
		return (ret);

	if ((ret = __bamc_start_decompress(dbc)) != 0)
		return (ret);

	/* Check the keys are the same */
	if (t->bt_compare(dbp, cp->currentKey, key) != 0)
		return (DB_NOTFOUND);

	return (0);
}

/* Implements DB_NEXT_NODUP for __bamc_compress_get() */
static int
__bamc_compress_get_next_nodup(dbc, flags)
	DBC *dbc;
	u_int32_t flags;
{
	int ret;
	BTREE_CURSOR *cp;
	DB *dbp;
	BTREE *t;

	cp = (BTREE_CURSOR *)dbc->internal;
	dbp = dbc->dbp;
	t = (BTREE *)dbp->bt_internal;

	if (cp->currentKey == 0)
		return (__bamc_compress_get_next(dbc, flags));

	/*
	 * If this is a deleted entry, del_key is already set, otherwise
	 * we have to set it now
	 */
	if (!F_ISSET(cp, C_COMPRESS_DELETED))
		if ((ret = __bam_compress_set_dbt(dbp, &cp->del_key,
		    cp->currentKey->data, cp->currentKey->size)) != 0)
			return (ret);

	/* Linear search for the next non-duplicate key */
	do
		if ((ret = __bamc_compress_get_next(dbc, flags)) != 0)
			return (ret);
	while (t->bt_compare(dbp, cp->currentKey, &cp->del_key) == 0);

	return (ret);
}

/*
 * Implements DB_SET, DB_SET_RANGE, DB_GET_BOTH, and DB_GET_BOTH_RANGE
 * for __bamc_compress_get()
 */
static int
__bamc_compress_get_set(dbc, key, data, method, flags)
	DBC *dbc;
	DBT *key;
	DBT *data;
	u_int32_t method;
	u_int32_t flags;
{
	int ret, cmp;
	BTREE_CURSOR *cp;
	DB *dbp;

	cp = (BTREE_CURSOR *)dbc->internal;
	dbp = dbc->dbp;

	if (method == DB_SET || method == DB_SET_RANGE)
	    data = NULL;

	F_CLR(cp, C_COMPRESS_DELETED);

	ret = __bamc_compress_seek(dbc, key, data, flags);
	if (ret == DB_NOTFOUND)
		CMP_IGET_RETRY(ret, dbc,
		    &cp->key1, &cp->compressed, DB_FIRST | flags);
	if (ret != 0)
		return (ret);

	/* Decompress and perform a linear search for the key */
	cmp = 0;
	ret = __bamc_start_decompress(dbc);
	while (ret == 0 && (cmp = __db_compare_both(dbp,
	   cp->currentKey, cp->currentData, key, data)) < 0) {
		ret = __bamc_next_decompress(dbc);
		if (ret == DB_NOTFOUND) {
			CMP_IGET_RETRY(ret, dbc,
			    &cp->key1, &cp->compressed, DB_NEXT | flags);
			if (ret == 0)
			    ret = __bamc_start_decompress(dbc);
		}
	}

	switch (method) {
	case DB_SET:
	case DB_GET_BOTH_RANGE:
		/*
		 * We need to exactly match the key, and if cmp != 0 we
		 * might not have - so check again here.
		 */
		if (ret == 0 &&
		    __db_compare_both(dbp, cp->currentKey, 0, key, 0) != 0) {
			/* We didn't find the key */
			ret = DB_NOTFOUND;
		}
		break;
	case DB_GET_BOTH:
		if (ret == 0 && (cmp != 0 || (!F_ISSET(dbp, DB_AM_DUPSORT) &&
		    __bam_defcmp(dbp, cp->currentData, data) != 0))) {
			/* We didn't find the key/data pair */
			ret = DB_NOTFOUND;
		}
		break;
	default:
		DB_ASSERT(dbp->env, method == 0 || method == DB_SET_RANGE);
	}

	return (ret);
}

/* Implements DB_GET_BOTHC for __bamc_compress_get() */
static int
__bamc_compress_get_bothc(dbc, data, flags)
	DBC *dbc;
	DBT *data;
	u_int32_t flags;
{
	int ret, cmp;
	BTREE_CURSOR *cp;
	DB *dbp;

	cp = (BTREE_CURSOR *)dbc->internal;
	dbp = dbc->dbp;

	/* Check that the data we are looking for comes after the current
	   position */
	if (__db_compare_both(dbp, cp->currentKey,
	    cp->currentData, cp->currentKey, data) >= 0)
		return (DB_NOTFOUND);

	cmp = 0;
	/* Perform a linear search for the data in the current chunk */
	while ((ret = __bamc_next_decompress(dbc)) == 0 &&
	    (cmp = __db_compare_both(
	    dbp, cp->currentKey, cp->currentData, cp->prevKey, data)) < 0)
		continue;

	if (ret == 0)
		return (cmp == 0 ? 0 : DB_NOTFOUND);
	if (ret != DB_NOTFOUND)
		return (ret);

	/* Copy the current key to del_key */
	if ((ret = __bam_compress_set_dbt(dbp, &cp->del_key,
		    cp->currentKey->data, cp->currentKey->size)) != 0)
		return (ret);

	/* Search for the data using DB_GET_BOTH */
	return __bamc_compress_get_set(
	    dbc, &cp->del_key, data, DB_GET_BOTH, flags);
}

/* Implements DB_MULTIPLE_KEY for __bamc_compress_get() */
static int
__bamc_compress_get_multiple_key(dbc, data, flags)
	DBC *dbc;
	DBT *data;
	u_int32_t flags;
{
	int ret;
	u_int8_t *writekey, *writedata;
	void *mptr;
	BTREE_CURSOR *cp;

	ret = 0;
	cp = (BTREE_CURSOR *)dbc->internal;

	DB_MULTIPLE_WRITE_INIT(mptr, data);
	DB_MULTIPLE_KEY_RESERVE_NEXT(mptr, data, writekey, cp->currentKey->size,
		writedata, cp->currentData->size);
	if (writekey == NULL) {
		data->size = cp->currentKey->size + cp->currentData->size +
			4 * sizeof(u_int32_t);
		return DB_BUFFER_SMALL;
	}
	DB_ASSERT(dbc->dbp->env, writedata != NULL);

	memcpy(writekey, cp->currentKey->data, cp->currentKey->size);
	memcpy(writedata, cp->currentData->data, cp->currentData->size);

	while ((ret = __bamc_compress_get_next(dbc, flags)) == 0) {
		DB_MULTIPLE_KEY_RESERVE_NEXT(mptr, data, writekey,
		    cp->currentKey->size, writedata, cp->currentData->size);
		if (writekey == NULL)
			break;
		DB_ASSERT(dbc->dbp->env, writedata != NULL);

		/*
		 * We could choose to optimize this by just storing one
		 * copy of a key for each set of duplicate data.
		 */
		memcpy(writekey, cp->currentKey->data, cp->currentKey->size);
		memcpy(writedata, cp->currentData->data, cp->currentData->size);
	}

	if (ret == DB_NOTFOUND)
		ret = 0;

	if (ret == 0)
		/*
		 * Rewind to the previous key/data, since we can't fit
		 * this one in the buffer
		 */
		ret = __bamc_compress_get_prev(dbc, flags);

	return (ret);
}

/* Implements DB_MULTIPLE for __bamc_compress_get() */
static int
__bamc_compress_get_multiple(dbc, key, data, flags)
	DBC *dbc;
	DBT *key, *data;
	u_int32_t flags;
{
	int ret;
	u_int8_t *writedata;
	void *mptr;
	BTREE_CURSOR *cp;

	ret = 0;
	cp = (BTREE_CURSOR *)dbc->internal;

	data->size = 0;

	DB_MULTIPLE_WRITE_INIT(mptr, data);
	DB_MULTIPLE_RESERVE_NEXT(mptr, data, writedata, cp->currentData->size);
	data->size += cp->currentData->size + 2 * sizeof(u_int32_t);
	if (writedata == NULL)
		return DB_BUFFER_SMALL;

	memcpy(writedata, cp->currentData->data, cp->currentData->size);

	while ((ret = __bamc_compress_get_next_dup(dbc, key, flags)) == 0) {
		DB_MULTIPLE_RESERVE_NEXT(
		    mptr, data, writedata, cp->currentData->size);
		data->size += cp->currentData->size + 2 * sizeof(u_int32_t);
		if (writedata == NULL) {
			/* DBC_FROM_DB_GET indicates we need to fit all the
			 * duplicates into the buffer or return DB_BUFFER_SMALL.
			 * [#17039]
			 */
			if (F_ISSET(dbc, DBC_FROM_DB_GET))
				return DB_BUFFER_SMALL;
			break;
		}

		memcpy(writedata, cp->currentData->data, cp->currentData->size);
	}

	if (ret == DB_NOTFOUND)
		ret = 0;

	if (ret == 0)
		/*
		 * Rewind to the previous key/data, as that's now our current
		 * entry.
		 */
		ret = __bamc_compress_get_prev(dbc, flags);

	return (ret);
}

/*
 * __bamc_compress_iget --
 *	Get using a compressed cursor. (internal)
 */
static int
__bamc_compress_iget(dbc, key, data, flags)
	DBC *dbc;
	DBT *key, *data;
	u_int32_t flags;
{
	int ret;
	u_int32_t multiple, method;
	BTREE_CURSOR *cp;
	DB *dbp;

	cp = (BTREE_CURSOR *)dbc->internal;
	dbp = dbc->dbp;
	ret = 0;

	multiple = flags & (DB_MULTIPLE|DB_MULTIPLE_KEY);
	method = flags & DB_OPFLAGS_MASK;
	flags = flags & ~(DB_OPFLAGS_MASK|DB_MULTIPLE|DB_MULTIPLE_KEY);

	switch (method) {
	case DB_CURRENT:
		if (F_ISSET(cp, C_COMPRESS_DELETED))
			ret = DB_KEYEMPTY;
		else if (cp->currentKey == NULL)
			ret = EINVAL;
		break;
	case DB_FIRST:
		__bamc_compress_reset(dbc);
		ret = __bamc_compress_get_next(dbc, flags);
		break;
	case DB_NEXT:
		ret = __bamc_compress_get_next(dbc, flags);
		break;
	case DB_NEXT_DUP:
		ret = __bamc_compress_get_next_dup(dbc, 0, flags);
		break;
	case DB_NEXT_NODUP:
		ret = __bamc_compress_get_next_nodup(dbc, flags);
		break;
	case DB_LAST:
		__bamc_compress_reset(dbc);
		ret = __bamc_compress_get_prev(dbc, flags);
		break;
	case DB_PREV:
		ret = __bamc_compress_get_prev(dbc, flags);
		break;
	case DB_PREV_DUP:
		ret = __bamc_compress_get_prev_dup(dbc, flags);
		break;
	case DB_PREV_NODUP:
		ret = __bamc_compress_get_prev_nodup(dbc, flags);
		break;
	case DB_SET:
		if (((BTREE *)
		    dbc->dbp->bt_internal)->bt_compare == __bam_defcmp)
			F_SET(key, DB_DBT_ISSET);
		/* FALL THROUGH */
	case DB_SET_RANGE:
		ret = __bamc_compress_get_set(dbc, key, 0, method, flags);
		break;
	case DB_GET_BOTH:
		if (!F_ISSET(dbc->dbp, DB_AM_DUPSORT) || ((BTREE *)dbc->dbp->
		   bt_internal)->compress_dup_compare == __bam_defcmp)
			F_SET(data, DB_DBT_ISSET);
		/* FALL THROUGH */
	case DB_GET_BOTH_RANGE:
		if (((BTREE *)
		    dbc->dbp->bt_internal)->bt_compare == __bam_defcmp)
			F_SET(key, DB_DBT_ISSET);
		ret = __bamc_compress_get_set(dbc, key, data, method, flags);
		break;
	case DB_GET_BOTHC:
		ret = __bamc_compress_get_bothc(dbc, data, flags);
		break;
	default:
		ret = __db_unknown_flag(dbp->env, "__bamc_compress_iget",
			method);
		break;
	}

	if (ret != 0)
		goto err;

	switch (multiple) {
	case 0:
		if (!F_ISSET(key, DB_DBT_ISSET))
			ret = __db_retcopy(dbc->env, key,
				cp->currentKey->data, cp->currentKey->size,
				&dbc->rkey->data, &dbc->rkey->ulen);
		if (!F_ISSET(data, DB_DBT_ISSET) && ret == 0)
			ret = __db_retcopy(dbc->env, data,
				cp->currentData->data, cp->currentData->size,
				&dbc->rdata->data, &dbc->rdata->ulen);
		break;
	case DB_MULTIPLE:
		if (!F_ISSET(key, DB_DBT_ISSET))
			ret = __db_retcopy(dbc->env, key,
				cp->currentKey->data, cp->currentKey->size,
				&dbc->rkey->data, &dbc->rkey->ulen);
		if (ret == 0)
			ret =
			    __bamc_compress_get_multiple(dbc, key, data, flags);
		break;
	case DB_MULTIPLE_KEY:
		ret = __bamc_compress_get_multiple_key(dbc, data, flags);
		break;
	default:
		ret = __db_unknown_flag(dbp->env, "__bamc_compress_iget",
			multiple);
		break;
	}

 err:
	F_CLR(key, DB_DBT_ISSET);
	F_CLR(data, DB_DBT_ISSET);

	return (ret);
}

/*
 * __bamc_compress_get --
 *	Get using a compressed cursor.
 *
 * PUBLIC: int __bamc_compress_get __P((DBC *, DBT *, DBT *, u_int32_t));
 */
int
__bamc_compress_get(dbc, key, data, flags)
	DBC *dbc;
	DBT *key, *data;
	u_int32_t flags;
{
	DBC *dbc_n;
	int ret, t_ret;
	u_int32_t tmp_flags;

	switch (flags & DB_OPFLAGS_MASK) {
	case DB_CURRENT:
	case DB_GET_BOTHC:
	case DB_NEXT:
	case DB_NEXT_DUP:
	case DB_NEXT_NODUP:
	case DB_PREV:
	case DB_PREV_DUP:
	case DB_PREV_NODUP:
		if (F_ISSET((BTREE_CURSOR *)dbc->internal, C_COMPRESS_MODIFIED)
		    && (ret = __bamc_compress_relocate(dbc)) != 0)
			return (ret);
		tmp_flags = DB_POSITION;
		break;
	default:
		F_CLR((BTREE_CURSOR *)dbc->internal, C_COMPRESS_MODIFIED);
		tmp_flags = 0;
		break;
	}

	if (F_ISSET(dbc, DBC_TRANSIENT))
		dbc_n = dbc;
	else {
		if ((ret = __dbc_dup(dbc, &dbc_n, tmp_flags)) != 0)
			goto err;

		/*
		 * We don't care about preserving the cursor's position on
		 * error.
		 */
		F_SET(dbc_n, DBC_TRANSIENT);

		COPY_RET_MEM(dbc, dbc_n);
	}

	if ((ret = __bamc_compress_iget(dbc_n, key, data, flags)) != 0)
		goto err;

err:
	/* Cleanup and cursor resolution. */
	if ((t_ret = __dbc_cleanup(dbc, dbc_n, ret)) != 0 &&
		(ret == 0 || ret == DB_BUFFER_SMALL))
		ret = t_ret;
	return (ret);
}

/*
 * __bamc_compress_iput --
 *	Put using a compressed cursor (internal)
 */
static int
__bamc_compress_iput(dbc, key, data, flags)
	DBC *dbc;
	DBT *key, *data;
	u_int32_t flags;
{
	int ret;
	u_int32_t multi;
	DBT kcpy, pdata, empty;
	BTREE_COMPRESS_STREAM stream;
	BTREE_CURSOR *cp;
	DB *dbp;
	ENV *env;

	cp = (BTREE_CURSOR *)dbc->internal;
	dbp = dbc->dbp;
	env = dbc->env;

	memset(&pdata, 0, sizeof(DBT));
	memset(&empty, 0, sizeof(DBT));

	multi = LF_ISSET(DB_MULTIPLE|DB_MULTIPLE_KEY);
	LF_CLR(DB_MULTIPLE|DB_MULTIPLE_KEY);

	switch (flags) {
	case DB_CURRENT:
		if (cp->currentKey == 0 || F_ISSET(cp, C_COMPRESS_DELETED)) {
			ret = DB_NOTFOUND;
			goto end;
		}

		if (F_ISSET(data, DB_DBT_PARTIAL)) {
			if ((ret = __db_buildpartial(
			    dbp, cp->currentData, data, &pdata)) != 0)
				goto end;
			data = &pdata;
		}

		if (F_ISSET(dbp, DB_AM_DUPSORT) &&
		    ((BTREE *)dbp->bt_internal)->compress_dup_compare(
		    dbp, cp->currentData, data) != 0) {
			__db_errx(env,
			    "Existing data sorts differently from put data");
			ret = EINVAL;
			goto end;
		}
		CMP_INIT_DBT(&kcpy);
		if ((ret = __bam_compress_set_dbt(dbp,
		    &kcpy, cp->currentKey->data, cp->currentKey->size)) != 0)
			goto end;

		__bam_cs_create_single(&stream, &kcpy, data);
		ret = __bamc_compress_merge_insert(dbc, &stream, NULL, flags);

		if (ret == 0)
			/* Position the cursor on the entry written */
			ret = __bamc_compress_get_set(
			    dbc, &kcpy, data, DB_GET_BOTH_RANGE, 0);

		CMP_FREE_DBT(env, &kcpy);
		break;
	case DB_KEYFIRST:
	case DB_KEYLAST:
	case DB_NODUPDATA:
	case DB_OVERWRITE_DUP:
		switch (multi) {
		case 0:
			if (F_ISSET(data, DB_DBT_PARTIAL)) {
				if ((ret = __bamc_compress_get_set(dbc, key,
				    data, DB_SET, 0)) != 0 &&
				    ret != DB_NOTFOUND)
					goto end;
				if ((ret = __db_buildpartial(dbp,
				    ret == DB_NOTFOUND ? &empty :
				    cp->currentData, data, &pdata)) != 0)
					goto end;
				data = &pdata;
			}

			__bam_cs_create_single(&stream, key, data);
			ret = __bamc_compress_merge_insert(
			    dbc, &stream, NULL,	flags);

			if (ret == 0)
				/* Position the cursor on the entry written */
				ret = __bamc_compress_get_set(
				    dbc, key, data, DB_GET_BOTH_RANGE, 0);
			break;
		case DB_MULTIPLE:
			__bam_cs_create_multiple(&stream, key, data);
			ret = __bamc_compress_merge_insert(
			    dbc, &stream, &key->doff, flags);
			break;
		case DB_MULTIPLE_KEY:
			__bam_cs_create_multiple_key(&stream, key);
			ret = __bamc_compress_merge_insert(
			    dbc, &stream, &key->doff, flags);
			break;
		default:
			return (__db_unknown_flag(
			    dbp->env, "__bamc_compress_iput", multi));
		}
		break;
	case DB_NOOVERWRITE:
		/* Check key doesn't already exist */
		ret = __bamc_compress_get_set(dbc, key, 0, DB_SET, 0);
		if (ret != DB_NOTFOUND) {
			if (ret == 0)
				ret = DB_KEYEXIST;
			goto end;
		}

		if (F_ISSET(data, DB_DBT_PARTIAL)) {
			if ((ret = __db_buildpartial(
			    dbp, &empty, data, &pdata))	!= 0)
				goto end;
			data = &pdata;
		}

		__bam_cs_create_single(&stream, key, data);
		ret = __bamc_compress_merge_insert(dbc, &stream, NULL, flags);

		if (ret == 0)
			/* Position the cursor on the entry written */
			ret = __bamc_compress_get_set(
			    dbc, key, data, DB_GET_BOTH_RANGE, 0);
		break;
	default:
		return (__db_unknown_flag(
		    dbp->env, "__bamc_compress_iput", flags));
	}

 end:
	if (pdata.data != NULL)
		__os_free(env, pdata.data);
	return (ret);
}

/*
 * __bamc_compress_put --
 *	Put using a compressed cursor.
 *
 * PUBLIC: int __bamc_compress_put __P((DBC *, DBT *, DBT *, u_int32_t));
 */
int
__bamc_compress_put(dbc, key, data, flags)
	DBC *dbc;
	DBT *key, *data;
	u_int32_t flags;
{
	DBC *dbc_n;
	int ret, t_ret;

	if (F_ISSET((BTREE_CURSOR *)dbc->internal, C_COMPRESS_MODIFIED)) {
		if ((flags & DB_OPFLAGS_MASK) == DB_CURRENT &&
		    (ret = __bamc_compress_relocate(dbc)) != 0)
			return (ret);
		F_CLR((BTREE_CURSOR *)dbc->internal, C_COMPRESS_MODIFIED);
	}

	if (F_ISSET(dbc, DBC_TRANSIENT))
		dbc_n = dbc;
	else {
		if ((ret = __dbc_dup(dbc, &dbc_n,
		    (flags & DB_OPFLAGS_MASK) == DB_CURRENT ?
		    DB_POSITION : 0)) != 0)
			goto err;

		/*
		 * We don't care about preserving the cursor's position on
		 * error.
		 */
		F_SET(dbc_n, DBC_TRANSIENT);
	}

	if ((ret = __bamc_compress_iput(dbc_n, key, data, flags)) != 0)
		goto err;

err:
	/* Cleanup and cursor resolution. */
	if ((t_ret = __dbc_cleanup(dbc, dbc_n, ret)) != 0 &&
		(ret == 0 || ret == DB_BUFFER_SMALL))
		ret = t_ret;
	return (ret);
}

/*
 * __bamc_compress_idel --
 *	Del using a compressed cursor. (internal)
 */
static int
__bamc_compress_idel(dbc, flags)
	DBC *dbc;
	u_int32_t flags;
{
	int ret;
	BTREE_COMPRESS_STREAM stream;
	DB *dbp;
	BTREE_CURSOR *cp;

	COMPQUIET(flags, 0);

	dbp = dbc->dbp;
	cp = (BTREE_CURSOR *)dbc->internal;

	if (F_ISSET(cp, C_COMPRESS_DELETED))
		return DB_KEYEMPTY;
	if (cp->currentKey == 0)
		return DB_NOTFOUND;

	if ((ret = __bam_compress_set_dbt(dbp, &cp->del_key,
		     cp->currentKey->data, cp->currentKey->size)) != 0)
		goto err;
	if ((ret = __bam_compress_set_dbt(dbp, &cp->del_data,
		     cp->currentData->data, cp->currentData->size)) != 0)
		goto err;

	__bam_cs_create_single(&stream, &cp->del_key, &cp->del_data);
	if ((ret = __bamc_compress_merge_delete(dbc, &stream, NULL)) != 0)
		goto err;

	/* Position the cursor on the entry after the key/data deleted */
	ret = __bamc_compress_get_set(dbc, &cp->del_key, &cp->del_data, 0, 0);
	if (ret == DB_NOTFOUND) {
		__bamc_compress_reset(dbc);
		ret = 0;
	} else if (ret != 0)
		goto err;

	/* Mark current as being deleted */
	F_SET(cp, C_COMPRESS_DELETED);

 err:
	return (ret);
}

/*
 * __bamc_compress_del --
 *	Del using a compressed cursor.
 *
 * PUBLIC: int __bamc_compress_del __P((DBC *, u_int32_t));
 */
int
__bamc_compress_del(dbc, flags)
	DBC *dbc;
	u_int32_t flags;
{
	int ret, t_ret;
	DBC *dbc_n;

	if (F_ISSET((BTREE_CURSOR *)dbc->internal, C_COMPRESS_MODIFIED) &&
	    (ret = __bamc_compress_relocate(dbc)) != 0)
		return (ret);

	if (F_ISSET(dbc, DBC_TRANSIENT))
		dbc_n = dbc;
	else {
		if ((ret = __dbc_dup(dbc, &dbc_n, DB_POSITION)) != 0)
			goto err;

		/*
		 * We don't care about preserving the cursor's position on
		 * error.
		 */
		F_SET(dbc_n, DBC_TRANSIENT);

		COPY_RET_MEM(dbc, dbc_n);
	}

	if ((ret = __bamc_compress_idel(dbc_n, flags)) != 0)
		goto err;

err:
	/* Cleanup and cursor resolution. */
	if ((t_ret = __dbc_cleanup(dbc, dbc_n, ret)) != 0 &&
		(ret == 0 || ret == DB_BUFFER_SMALL))
		ret = t_ret;
	return (ret);
}

/*
 * __bamc_compress_ibulk_del --
 *	Bulk del using a compressed cursor. (internal)
 */
static int
__bamc_compress_ibulk_del(dbc, key, flags)
	DBC *dbc;
	DBT *key;
	u_int32_t flags;
{
	BTREE_COMPRESS_STREAM stream;

	switch (flags) {
	case 0:
		__bam_cs_create_single_keyonly(&stream, key);
		return (__bamc_compress_merge_delete_dups(dbc, &stream, NULL));
	case DB_MULTIPLE:
		__bam_cs_create_multiple_keyonly(&stream, key);
		return (__bamc_compress_merge_delete_dups(
		    dbc, &stream, &key->doff));
	case DB_MULTIPLE_KEY:
		__bam_cs_create_multiple_key(&stream, key);
		return (__bamc_compress_merge_delete(dbc, &stream, &key->doff));
	default:
		break;
	}

	return (__db_unknown_flag(
	    dbc->env, "__bamc_compress_ibulk_del", flags));
}

/*
 * __bamc_compress_bulk_del --
 *	Bulk del using a compressed cursor.
 *
 * PUBLIC: int __bamc_compress_bulk_del __P((DBC *, DBT *, u_int32_t));
 */
int
__bamc_compress_bulk_del(dbc, key, flags)
	DBC *dbc;
	DBT *key;
	u_int32_t flags;
{
	int ret, t_ret;
	DBC *dbc_n;

	F_CLR((BTREE_CURSOR *)dbc->internal, C_COMPRESS_MODIFIED);

	if (F_ISSET(dbc, DBC_TRANSIENT))
		dbc_n = dbc;
	else {
		if ((ret = __dbc_dup(dbc, &dbc_n, 0)) != 0)
			goto err;

		/*
		 * We don't care about preserving the cursor's position on
		 * error.
		 */
		F_SET(dbc_n, DBC_TRANSIENT);
	}

	if ((ret = __bamc_compress_ibulk_del(dbc_n, key, flags)) != 0)
		goto err;

err:
	/* Cleanup and cursor resolution. */
	if ((t_ret = __dbc_cleanup(dbc, dbc_n, ret)) != 0 &&
		(ret == 0 || ret == DB_BUFFER_SMALL))
		ret = t_ret;
	return (ret);
}

/*
 * __bamc_compress_count --
 *	Count using a compressed cursor.
 *
 * PUBLIC: int __bamc_compress_count __P((DBC *, db_recno_t *));
 */
int
__bamc_compress_count(dbc, countp)
	DBC *dbc;
	db_recno_t *countp;
{
	int ret, t_ret;
	db_recno_t count;
	DBT *key;
	DBC *dbc_n;
	BTREE_CURSOR *cp;

	cp = (BTREE_CURSOR *)dbc->internal;

	/*
	 * If the current entry is deleted use del_key, otherwise use
	 * currentKey.
	 */
	if (F_ISSET(cp, C_COMPRESS_DELETED))
		key = &cp->del_key;
	else
		key = cp->currentKey;

	/* Duplicate the cursor */
	if ((ret = __dbc_dup(dbc, &dbc_n, 0)) != 0)
		return (ret);

	/* We don't care about preserving the cursor's position on error */
	F_SET(dbc_n, DBC_TRANSIENT);

	/* Find the first duplicate */
	if ((ret = __bamc_compress_get_set(dbc_n, key, 0, DB_SET, 0)) != 0)
		goto err;
	count = 1;

	/* Count subsequent duplicates */
	while ((ret = __bamc_compress_get_next_dup(dbc_n, key, 0)) == 0)
		++count;

	if (ret == DB_NOTFOUND)
		ret = 0;
	else if (ret != 0)
		goto err;

	*countp = count;

 err:
	if ((t_ret = __dbc_close(dbc_n)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

/*
 * __bamc_compress_cmp --
 *	Compare which compressed value is pointed to.
 *
 * PUBLIC: int __bamc_compress_cmp __P((DBC *, DBC *, int *));
 */
int
__bamc_compress_cmp(dbc, other_dbc, result)
	DBC *dbc, *other_dbc;
	int *result;
{
	DB *dbp;
	BTREE_CURSOR *cp, *ocp;

	/*
	 * At this point, we already know that the cursors point to the same
	 * DB.
	 */

	dbp = dbc->dbp;
	cp = (BTREE_CURSOR *)dbc->internal;
	ocp = (BTREE_CURSOR *)other_dbc->internal;

	if (F_ISSET(cp, C_COMPRESS_DELETED))
		if (F_ISSET(ocp, C_COMPRESS_DELETED))
			*result = __db_compare_both(
			     dbp, &cp->del_key, &cp->del_data,
			     &ocp->del_key, &ocp->del_data) == 0 ? 0 : 1;
		else {
			if (ocp->currentKey == 0)
				goto err;

			*result = __db_compare_both(
			     dbp, &cp->del_key, &cp->del_data,
			     ocp->currentKey, ocp->currentData) == 0 ? 0 : 1;
		}
	else {
		if (cp->currentKey == 0)
			goto err;

		if (F_ISSET(ocp, C_COMPRESS_DELETED))
			*result = __db_compare_both(
			     dbp, cp->currentKey, cp->currentData,
			     &ocp->del_key, &ocp->del_data) == 0 ? 0 : 1;
		else {
			if (ocp->currentKey == 0)
				goto err;

			*result = __db_compare_both(
			     dbp, cp->currentKey, cp->currentData,
			     ocp->currentKey, ocp->currentData) == 0 ? 0 : 1;
		}
	}
	return (0);

 err:
	__db_errx(dbc->env,
		"Both cursors must be initialized before calling DBC->cmp.");
	return (EINVAL);
}

/*
 * __bamc_compress_dup --
 *	Duplicate the compression specific part of a btree cursor.
 *
 * PUBLIC: int __bamc_compress_dup __P((DBC *, DBC *, u_int32_t));
 */
int
__bamc_compress_dup(orig_dbc, new_dbc, flags)
	DBC *orig_dbc, *new_dbc;
	u_int32_t flags;
{
	int ret;
	DB *dbp;
	BTREE_CURSOR *orig, *new;

	dbp = new_dbc->dbp;

	orig = (BTREE_CURSOR *)orig_dbc->internal;
	new = (BTREE_CURSOR *)new_dbc->internal;

	if (orig->currentKey != NULL && !LF_ISSET(DB_SHALLOW_DUP)) {
		new->currentKey = &new->key1;
		new->currentData = &new->data1;

		if ((ret = __bam_compress_set_dbt(dbp, new->currentKey,
		    orig->currentKey->data, orig->currentKey->size)) != 0)
			return (ret);
		if ((ret = __bam_compress_set_dbt(dbp, new->currentData,
		    orig->currentData->data, orig->currentData->size)) != 0)
			return (ret);

		if (orig->prevKey) {
			new->prevKey = &new->key2;
			new->prevData = &new->data2;

			if ((ret = __bam_compress_set_dbt(dbp, new->prevKey,
			    orig->prevKey->data, orig->prevKey->size)) != 0)
				return (ret);
			if ((ret = __bam_compress_set_dbt(dbp, new->prevData,
			    orig->prevData->data, orig->prevData->size)) != 0)
				return (ret);
		}

		if ((ret = __bam_compress_set_dbt(dbp, &new->compressed,
		    orig->compressed.data, orig->compressed.size)) != 0)
			return (ret);

		new->compcursor = (u_int8_t*)new->compressed.data +
			(orig->compcursor - (u_int8_t*)orig->compressed.data);
		new->compend = (u_int8_t*)new->compressed.data +
			(orig->compend - (u_int8_t*)orig->compressed.data);
		new->prevcursor = orig->prevcursor == NULL ? NULL :
			(u_int8_t*)new->compressed.data + (orig->prevcursor -
				(u_int8_t*)orig->compressed.data);
		new->prev2cursor = orig->prev2cursor == NULL ? NULL :
			(u_int8_t*)new->compressed.data + (orig->prev2cursor -
				(u_int8_t*)orig->compressed.data);

		if (F_ISSET(orig, C_COMPRESS_DELETED)) {
			if ((ret = __bam_compress_set_dbt(dbp, &new->del_key,
			    orig->del_key.data, orig->del_key.size)) != 0)
				return (ret);
			if ((ret = __bam_compress_set_dbt(dbp, &new->del_data,
			    orig->del_data.data, orig->del_data.size)) != 0)
				return (ret);
		}
	}

	return (0);
}

/*
 * __bam_compress_salvage --
 *	Salvage the compressed data from the key/data pair
 *
 * PUBLIC: int __bam_compress_salvage __P((DB *, VRFY_DBINFO *,
 * PUBLIC:     void *, int (*)(void *, const void *), DBT *, DBT *));
 */
int
__bam_compress_salvage(dbp, vdp, handle, callback, key, data)
	DB *dbp;
	VRFY_DBINFO *vdp;
	void *handle;
	int (*callback) __P((void *, const void *));
	DBT *key, *data;
{
	DBT key1, key2, data1, data2, compressed;
	DBT *currentKey, *currentData, *prevKey, *prevData;
	ENV *env;
	int ret, t_ret;
	u_int8_t *compcursor, *compend;
	u_int32_t datasize, size;

	env = dbp->env;

	memset(&key1, 0, sizeof(DBT));
	memset(&key2, 0, sizeof(DBT));
	memset(&data1, 0, sizeof(DBT));
	memset(&data2, 0, sizeof(DBT));
	memset(&compressed, 0, sizeof(DBT));

	key1.flags = DB_DBT_USERMEM;
	key2.flags = DB_DBT_USERMEM;
	data1.flags = DB_DBT_USERMEM;
	data2.flags = DB_DBT_USERMEM;
	compressed.flags = DB_DBT_USERMEM;

	prevKey = NULL;
	prevData = NULL;
	currentKey = key;
	currentData = &data2;
	compcursor = (u_int8_t*)data->data;
	compend = compcursor + data->size;

	if (data->size == 0) {
		ret = DB_VERIFY_FATAL;
		goto unknown_data;
	}
	
	/* Unmarshal the first data */
	size = __db_decompress_count_int(compcursor);
	if (size == 0xFF || compcursor + size > compend) {
		ret = DB_VERIFY_FATAL;
		goto unknown_data;
	}
	compcursor += __db_decompress_int32(compcursor, &datasize);

	if (compcursor + datasize > compend) {
		ret = DB_VERIFY_FATAL;
		goto unknown_data;
	}
	if ((ret = __bam_compress_set_dbt(
	    dbp, currentData, compcursor, datasize)) != 0)
		goto err;
	compcursor += datasize;

	/* Output first data (first key has already been output by our caller */
	if ((ret = __db_vrfy_prdbt(
	    currentData, 0, " ", handle, callback, 0, vdp)) != 0)
		goto err;

	while (compcursor < compend) {
		prevKey = currentKey;
		prevData = currentData;

		if (currentKey == &key1) {
			currentKey = &key2;
			currentData = &data2;
		} else {
			currentKey = &key1;
			currentData = &data1;
		}

		compressed.data = (void*)compcursor;
		compressed.ulen = compressed.size =
		    (u_int32_t)(compend - compcursor);

		/* Decompress the next key/data pair */
		while ((ret = ((BTREE *)dbp->bt_internal)->bt_decompress(
		    dbp, prevKey, prevData,
		    &compressed, currentKey, currentData)) == DB_BUFFER_SMALL) {
			if (CMP_RESIZE_DBT(ret, env, currentKey) != 0)
				break;
			if (CMP_RESIZE_DBT(ret, env, currentData) != 0)
				break;
		}

		if (ret == EINVAL) {
			ret = DB_VERIFY_FATAL;
			goto err;
		}
		if (ret != 0)
			goto err;

		compcursor += compressed.size;

		if (compcursor > compend) {
			ret = DB_VERIFY_FATAL;
			goto err;
		}

		/* Output the next key/data pair */
		if ((ret = __db_vrfy_prdbt(
		    currentKey, 0, " ", handle, callback, 0, vdp)) != 0)
			goto err;
		if ((ret = __db_vrfy_prdbt(
		    currentData, 0, " ", handle, callback, 0, vdp)) != 0)
			goto err;
	}

	if (0) {
 unknown_data:
		/*
		 * Make sure we output a data value for the key that's
		 * already been output
		 */
		DB_INIT_DBT(
		    compressed, "UNKNOWN_DATA",	sizeof("UNKNOWN_DATA") - 1);
		if ((t_ret = __db_vrfy_prdbt(
		    &compressed, 0, " ", handle, callback, 0, vdp)) != 0)
			ret = t_ret;
	}

 err:
	__os_free(env, key1.data);
	__os_free(env, key2.data);
	__os_free(env, data1.data);
	__os_free(env, data2.data);
	return (ret);
}

/*
 * __bam_compress_count --
 *	Calculate key and entry counts for the compressed BTree
 *
 * PUBLIC: int __bam_compress_count __P((DBC *, u_int32_t *, u_int32_t *));
 */
int
__bam_compress_count(dbc, nkeysp, ndatap)
	DBC *dbc;
	u_int32_t *nkeysp, *ndatap;
{
	int ret, t_ret;
	u_int32_t nkeys, ndata;
	DB *dbp;
	BTREE *t;
	DBC *dbc_n;
	BTREE_CURSOR *cp_n;

	dbp = dbc->dbp;
	t = (BTREE *)dbp->bt_internal;

	/* Duplicate the cursor */
	if ((ret = __dbc_dup(dbc, &dbc_n, 0)) != 0)
		return (ret);

	/* We don't care about preserving the cursor's position on error */
	F_SET(dbc_n, DBC_TRANSIENT);

	cp_n = (BTREE_CURSOR *)dbc_n->internal;

	nkeys = 0;
	ndata = 0;

	CMP_IGET_RETRY(ret, dbc_n, &cp_n->key1, &cp_n->compressed, DB_FIRST);
	if (ret != 0)
		goto err;

	if ((ret = __bamc_start_decompress(dbc_n)) != 0)
		goto err;
	nkeys += 1;

	for (;;) {
		ndata += 1;

		ret = __bamc_next_decompress(dbc_n);
		if (ret == DB_NOTFOUND) {
			if (cp_n->currentKey == &cp_n->key1) {
				/*
				 * Make sure that the previous key isn't
				 * overwritten when we fetch the next chunk.
				 */
				if ((ret = __bam_compress_set_dbt(dbp,
					     &cp_n->key2, cp_n->key1.data,
					     cp_n->key1.size)) != 0)
					goto err;
			}

			CMP_IGET_RETRY(ret, dbc_n, &cp_n->key1,
				&cp_n->compressed, DB_NEXT);
			if (ret != 0)
				goto err;

			ret = __bamc_start_decompress(dbc_n);

			cp_n->prevKey = &cp_n->key2;
		}

		if (ret != 0)
			goto err;

		if (t->bt_compare(dbp, cp_n->currentKey, cp_n->prevKey) != 0)
			nkeys += 1;
	}

err:
	if (ret == DB_NOTFOUND)
		ret = 0;

	if ((t_ret = __dbc_close(dbc_n)) != 0 && ret == 0)
		ret = t_ret;

	if (ret == 0) {
		if (nkeysp != NULL)
			*nkeysp = nkeys;
		if (ndatap != NULL)
			*ndatap = ndata;
	}

	return (ret);
}

#endif
