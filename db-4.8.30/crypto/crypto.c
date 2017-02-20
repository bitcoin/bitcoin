/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * Some parts of this code originally written by Adam Stubblefield
 * -- astubble@rice.edu
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/crypto.h"

/*
 * __crypto_region_init --
 *	Initialize crypto.
 */
int
__crypto_region_init(env)
	ENV *env;
{
	CIPHER *cipher;
	DB_CIPHER *db_cipher;
	DB_ENV *dbenv;
	REGENV *renv;
	REGINFO *infop;
	char *sh_passwd;
	int ret;

	dbenv = env->dbenv;
	infop = env->reginfo;
	renv = infop->primary;
	db_cipher = env->crypto_handle;
	ret = 0;

	if (renv->cipher_off == INVALID_ROFF) {
		if (!CRYPTO_ON(env))
			return (0);
		if (!F_ISSET(infop, REGION_CREATE)) {
			__db_errx(env,
    "Joining non-encrypted environment with encryption key");
			return (EINVAL);
		}
		if (F_ISSET(db_cipher, CIPHER_ANY)) {
			__db_errx(env, "Encryption algorithm not supplied");
			return (EINVAL);
		}
		/*
		 * Must create the shared information.  We need: Shared cipher
		 * information that contains the passwd.  After we copy the
		 * passwd, we smash and free the one in the env.
		 */
		MUTEX_LOCK(env, renv->mtx_regenv);
		if ((ret = __env_alloc(infop, sizeof(CIPHER), &cipher)) != 0) {
			MUTEX_UNLOCK(env, renv->mtx_regenv);
			return (ret);
		}
		memset(cipher, 0, sizeof(*cipher));
		if ((ret =
		    __env_alloc(infop, dbenv->passwd_len, &sh_passwd)) != 0) {
			__env_alloc_free(infop, cipher);
			MUTEX_UNLOCK(env, renv->mtx_regenv);
			return (ret);
		}
		MUTEX_UNLOCK(env, renv->mtx_regenv);
		memset(sh_passwd, 0, dbenv->passwd_len);
		cipher->passwd = R_OFFSET(infop, sh_passwd);
		cipher->passwd_len = dbenv->passwd_len;
		cipher->flags = db_cipher->alg;
		memcpy(sh_passwd, dbenv->passwd, cipher->passwd_len);
		renv->cipher_off = R_OFFSET(infop, cipher);
	} else {
		if (!CRYPTO_ON(env)) {
			__db_errx(env,
		    "Encrypted environment: no encryption key supplied");
			return (EINVAL);
		}
		cipher = R_ADDR(infop, renv->cipher_off);
		sh_passwd = R_ADDR(infop, cipher->passwd);
		if ((cipher->passwd_len != dbenv->passwd_len) ||
		    memcmp(dbenv->passwd, sh_passwd, cipher->passwd_len) != 0) {
			__db_errx(env, "Invalid password");
			return (EPERM);
		}
		if (!F_ISSET(db_cipher, CIPHER_ANY) &&
		    db_cipher->alg != cipher->flags) {
			__db_errx(env,
    "Environment encrypted using a different algorithm");
			return (EINVAL);
		}
		if (F_ISSET(db_cipher, CIPHER_ANY))
			/*
			 * We have CIPHER_ANY and we are joining the existing
			 * env.  Setup our cipher structure for whatever
			 * algorithm this env has.
			 */
			if ((ret = __crypto_algsetup(env, db_cipher,
			    cipher->flags, 0)) != 0)
				return (ret);
	}
	ret = db_cipher->init(env, db_cipher);

	/*
	 * On success, no matter if we allocated it or are using the already
	 * existing one, we are done with the passwd in the env.  We smash
	 * N-1 bytes so that we don't overwrite the nul.
	 */
	memset(dbenv->passwd, 0xff, dbenv->passwd_len-1);
	__os_free(env, dbenv->passwd);
	dbenv->passwd = NULL;
	dbenv->passwd_len = 0;

	return (ret);
}

/*
 * __crypto_env_close --
 *	Crypto-specific destruction of ENV structure.
 *
 * PUBLIC: int __crypto_env_close __P((ENV *));
 */
int
__crypto_env_close(env)
	ENV *env;
{
	DB_CIPHER *db_cipher;
	DB_ENV *dbenv;
	int ret;

	dbenv = env->dbenv;

	if (dbenv->passwd != NULL) {
		memset(dbenv->passwd, 0xff, dbenv->passwd_len-1);
		__os_free(env, dbenv->passwd);
		dbenv->passwd = NULL;
	}

	if (!CRYPTO_ON(env))
		return (0);

	ret = 0;
	db_cipher = env->crypto_handle;
	if (!F_ISSET(db_cipher, CIPHER_ANY))
		ret = db_cipher->close(env, db_cipher->data);
	__os_free(env, db_cipher);

	env->crypto_handle = NULL;
	return (ret);
}

/*
 * __crypto_env_refresh --
 *	Clean up after the crpto system on a close or failed open.
 *
 * PUBLIC: int __crypto_env_refresh __P((ENV *));
 */
int
__crypto_env_refresh(env)
	ENV *env;
{
	CIPHER *cipher;
	REGENV *renv;
	REGINFO *infop;

	/*
	 * If a private region, return the memory to the heap.  Not needed for
	 * filesystem-backed or system shared memory regions, that memory isn't
	 * owned by any particular process.
	 */
	if (F_ISSET(env, ENV_PRIVATE)) {
		infop = env->reginfo;
		renv = infop->primary;
		if (renv->cipher_off != INVALID_ROFF) {
			cipher = R_ADDR(infop, renv->cipher_off);
			MUTEX_LOCK(env, renv->mtx_regenv);
			__env_alloc_free(infop, R_ADDR(infop, cipher->passwd));
			__env_alloc_free(infop, cipher);
			MUTEX_UNLOCK(env, renv->mtx_regenv);
		}
	}
	return (0);
}

/*
 * __crypto_algsetup --
 *	Given a db_cipher structure and a valid algorithm flag, call
 * the specific algorithm setup function.
 *
 * PUBLIC: int __crypto_algsetup __P((ENV *, DB_CIPHER *, u_int32_t, int));
 */
int
__crypto_algsetup(env, db_cipher, alg, do_init)
	ENV *env;
	DB_CIPHER *db_cipher;
	u_int32_t alg;
	int do_init;
{
	int ret;

	ret = 0;
	if (!CRYPTO_ON(env)) {
		__db_errx(env, "No cipher structure given");
		return (EINVAL);
	}
	F_CLR(db_cipher, CIPHER_ANY);
	switch (alg) {
	case CIPHER_AES:
		db_cipher->alg = CIPHER_AES;
		ret = __aes_setup(env, db_cipher);
		break;
	default:
		ret = __env_panic(env, EINVAL);
		break;
	}
	if (ret == 0 && do_init)
		ret = db_cipher->init(env, db_cipher);
	return (ret);
}

/*
 * __crypto_decrypt_meta --
 *	Perform decryption on a metapage if needed.
 *
 * PUBLIC:  int __crypto_decrypt_meta __P((ENV *, DB *, u_int8_t *, int));
 */
int
__crypto_decrypt_meta(env, dbp, mbuf, do_metachk)
	ENV *env;
	DB *dbp;
	u_int8_t *mbuf;
	int do_metachk;
{
	DB dummydb;
	DBMETA *meta;
	DB_CIPHER *db_cipher;
	size_t pg_off;
	int ret;
	u_int8_t *iv;

	/*
	 * If we weren't given a dbp, we just want to decrypt the page on
	 * behalf of some internal subsystem, not on behalf of a user with
	 * a dbp.  Therefore, set up a dummy dbp so that the call to
	 * P_OVERHEAD below works.
	 */
	if (dbp == NULL) {
		memset(&dummydb, 0, sizeof(DB));
		dbp = &dummydb;
	}

	ret = 0;
	meta = (DBMETA *)mbuf;

	/*
	 * !!!
	 * We used an "unused" field in the meta-data page to flag whether or
	 * not the database is encrypted.  Unfortunately, that unused field
	 * was used in Berkeley DB releases before 3.0 (for example, 2.7.7).
	 * It would have been OK, except encryption doesn't follow the usual
	 * rules of "upgrade before doing anything else", we check encryption
	 * before checking for old versions of the database.
	 *
	 * We don't have to check Btree databases -- before 3.0, the field of
	 * interest was the bt_maxkey field (which was never supported and has
	 * since been removed).
	 *
	 * Ugly check to jump out if this format is older than what we support.
	 * This works because we do not encrypt the page header.
	 */
	if (meta->magic == DB_HASHMAGIC && meta->version <= 5)
		return (0);

	/*
	 * Meta-pages may be encrypted for DBMETASIZE bytes.  If we have a
	 * non-zero IV (that is written after encryption) then we decrypt (or
	 * error if the user isn't set up for security).  We guarantee that
	 * the IV space on non-encrypted pages will be zero and a zero-IV is
	 * illegal for encryption.  Therefore any non-zero IV means an
	 * encrypted database.  This basically checks the passwd on the file
	 * if we cannot find a good magic number.  We walk through all the
	 * algorithms we know about attempting to decrypt (and possibly
	 * byteswap).
	 *
	 * !!!
	 * All method meta pages have the IV and checksum at the exact same
	 * location, but not in DBMETA, use BTMETA.
	 */
	if (meta->encrypt_alg != 0) {
		db_cipher = env->crypto_handle;
		if (!F_ISSET(dbp, DB_AM_ENCRYPT)) {
			if (!CRYPTO_ON(env)) {
				__db_errx(env,
    "Encrypted database: no encryption flag specified");
				return (EINVAL);
			}
			/*
			 * User has a correct, secure env, but has encountered
			 * a database in that env that is secure, but user
			 * didn't dbp->set_flags.  Since it is existing, use
			 * encryption if it is that way already.
			 */
			F_SET(dbp, DB_AM_ENCRYPT|DB_AM_CHKSUM);
		}
		/*
		 * This was checked in set_flags when DB_AM_ENCRYPT was set.
		 * So it better still be true here.
		 */
		DB_ASSERT(env, CRYPTO_ON(env));
		if (!F_ISSET(db_cipher, CIPHER_ANY) &&
		    meta->encrypt_alg != db_cipher->alg) {
			__db_errx(env,
			    "Database encrypted using a different algorithm");
			return (EINVAL);
		}
		DB_ASSERT(env, F_ISSET(dbp, DB_AM_CHKSUM));
		iv = ((BTMETA *)mbuf)->iv;
		/*
		 * For ALL pages, we do not encrypt the beginning of the page
		 * that contains overhead information.  This is true of meta
		 * and all other pages.
		 */
		pg_off = P_OVERHEAD(dbp);
alg_retry:
		/*
		 * If they asked for a specific algorithm, then
		 * use it.  Otherwise walk through those we know.
		 */
		if (!F_ISSET(db_cipher, CIPHER_ANY)) {
			if (do_metachk && (ret = db_cipher->decrypt(env,
			    db_cipher->data, iv, mbuf + pg_off,
			    DBMETASIZE - pg_off)))
				return (ret);
			if (((BTMETA *)meta)->crypto_magic !=
			    meta->magic) {
				__db_errx(env, "Invalid password");
				return (EINVAL);
			}
			/*
			 * Success here.  The algorithm asked for and the one
			 * on the file match.  We've just decrypted the meta
			 * page and checked the magic numbers.  They match,
			 * indicating the password is right.  All is right
			 * with the world.
			 */
			return (0);
		}
		/*
		 * If we get here, CIPHER_ANY must be set.
		 */
		ret = __crypto_algsetup(env, db_cipher, meta->encrypt_alg, 1);
		goto alg_retry;
	} else if (F_ISSET(dbp, DB_AM_ENCRYPT)) {
		/*
		 * They gave us a passwd, but the database is not encrypted.
		 * This is an error.  We do NOT want to silently allow them
		 * to write data in the clear when the user set up and expects
		 * encrypted data.
		 *
		 * This covers at least the following scenario.
		 * 1.  User creates and sets up an encrypted database.
		 * 2.  Attacker cannot read the actual data in the database
		 * because it is encrypted, but can remove/replace the file
		 * with an empty, unencrypted database file.
		 * 3.  User sets encryption and we get to this code now.
		 * If we allowed the file to be used in the clear since
		 * it is that way on disk, the user would unsuspectingly
		 * write sensitive data in the clear.
		 * 4.  Attacker reads data that user thought was encrypted.
		 *
		 * Therefore, asking for encryption with a database that
		 * was not encrypted is an error.
		 */
		__db_errx(env,
		    "Unencrypted database with a supplied encryption key");
		return (EINVAL);
	}
	return (ret);
}

/*
 * __crypto_set_passwd --
 *	Get the password from the shared region; and set it in a new
 * environment handle.  Use this to duplicate environment handles.
 *
 * PUBLIC: int __crypto_set_passwd __P((ENV *, ENV *));
 */
int
__crypto_set_passwd(env_src, env_dest)
	ENV *env_src, *env_dest;
{
	CIPHER *cipher;
	REGENV *renv;
	REGINFO *infop;
	char *sh_passwd;

	infop = env_src->reginfo;
	renv = infop->primary;

	DB_ASSERT(env_src, CRYPTO_ON(env_src));

	cipher = R_ADDR(infop, renv->cipher_off);
	sh_passwd = R_ADDR(infop, cipher->passwd);
	return (__env_set_encrypt(env_dest->dbenv, sh_passwd, DB_ENCRYPT_AES));
}
