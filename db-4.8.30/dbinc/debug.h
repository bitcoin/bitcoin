/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1998-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef _DB_DEBUG_H_
#define _DB_DEBUG_H_

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Turn on additional error checking in gcc 3.X.
 */
#if !defined(__GNUC__) || __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 5)
#define __attribute__(s)
#endif

/*
 * When running with #DIAGNOSTIC defined, we smash memory and do memory
 * guarding with a special byte value.
 */
#define CLEAR_BYTE  0xdb
#define GUARD_BYTE  0xdc

/*
 * DB assertions.
 *
 * Use __STDC__ rather than STDC_HEADERS, the #e construct is ANSI C specific.
 */
#if defined(DIAGNOSTIC) && defined(__STDC__)
#define DB_ASSERT(env, e)                       \
    ((e) ? (void)0 : __db_assert(env, #e, __FILE__, __LINE__))
#else
#define DB_ASSERT(env, e)
#endif

/*
 * "Shut that bloody compiler up!"
 *
 * Unused, or not-used-yet variable.  We need to write and then read the
 * variable, some compilers are too bloody clever by half.
 */
#define COMPQUIET(n, v) do {                            \
    (n) = (v);                              \
    (n) = (n);                              \
} while (0)

/*
 * Purify and other run-time tools complain about uninitialized reads/writes
 * of structure fields whose only purpose is padding, as well as when heap
 * memory that was never initialized is written to disk.
 */
#ifdef  UMRW
#define UMRW_SET(v) (v) = 0
#else
#define UMRW_SET(v)
#endif

/*
 * Errors are in one of two areas: a Berkeley DB error, or a system-level
 * error.  We use db_strerror to translate the former and __os_strerror to
 * translate the latter.
 */
typedef enum {
    DB_ERROR_NOT_SET=0,
    DB_ERROR_SET=1,
    DB_ERROR_SYSTEM=2
} db_error_set_t;

/*
 * Message handling.  Use a macro instead of a function because va_list
 * references to variadic arguments cannot be reset to the beginning of the
 * variadic argument list (and then rescanned), by functions other than the
 * original routine that took the variadic list of arguments.
 */
#if defined(STDC_HEADERS) || defined(__cplusplus)
#define DB_REAL_ERR(dbenv, error, error_set, app_call, fmt) {       \
    va_list __ap;                           \
                                    \
    /* Call the application's callback function, if specified. */   \
    va_start(__ap, fmt);                        \
    if ((dbenv) != NULL && (dbenv)->db_errcall != NULL)     \
        __db_errcall(dbenv, error, error_set, fmt, __ap);   \
    va_end(__ap);                           \
                                    \
    /*                              \
     * If the application specified a file descriptor, write to it. \
     * If we wrote to neither the application's callback routine or \
     * its file descriptor, and it's an application error message   \
     * using {DbEnv,Db}.{err,errx} or the application has never \
     * configured an output channel, default by writing to stderr.  \
     */                             \
    va_start(__ap, fmt);                        \
    if ((dbenv) == NULL ||                      \
        (dbenv)->db_errfile != NULL ||              \
        ((dbenv)->db_errcall == NULL &&             \
        ((app_call) || F_ISSET((dbenv)->env, ENV_NO_OUTPUT_SET))))  \
        __db_errfile(dbenv, error, error_set, fmt, __ap);   \
    va_end(__ap);                           \
}
#else
#define DB_REAL_ERR(dbenv, error, error_set, app_call, fmt) {       \
    va_list __ap;                           \
                                    \
    /* Call the application's callback function, if specified. */   \
    va_start(__ap);                         \
    if ((dbenv) != NULL && (dbenv)->db_errcall != NULL)     \
        __db_errcall(dbenv, error, error_set, fmt, __ap);   \
    va_end(__ap);                           \
                                    \
    /*                              \
     * If the application specified a file descriptor, write to it. \
     * If we wrote to neither the application's callback routine or \
     * its file descriptor, and it's an application error message   \
     * using {DbEnv,Db}.{err,errx} or the application has never \
     * configured an output channel, default by writing to stderr.  \
     */                             \
    va_start(__ap);                         \
    if ((dbenv) == NULL ||                      \
        (dbenv)->db_errfile != NULL ||              \
        ((dbenv)->db_errcall == NULL &&             \
        ((app_call) || F_ISSET((dbenv)->env, ENV_NO_OUTPUT_SET))))  \
         __db_errfile(env, error, error_set, fmt, __ap);    \
    va_end(__ap);                           \
}
#endif
#if defined(STDC_HEADERS) || defined(__cplusplus)
#define DB_REAL_MSG(dbenv, fmt) {                   \
    va_list __ap;                           \
                                    \
    /* Call the application's callback function, if specified. */   \
    va_start(__ap, fmt);                        \
    if ((dbenv) != NULL && (dbenv)->db_msgcall != NULL)     \
        __db_msgcall(dbenv, fmt, __ap);             \
    va_end(__ap);                           \
                                    \
    /*                              \
     * If the application specified a file descriptor, write to it. \
     * If we wrote to neither the application's callback routine or \
     * its file descriptor, write to stdout.            \
     */                             \
    va_start(__ap, fmt);                        \
    if ((dbenv) == NULL ||                      \
        (dbenv)->db_msgfile != NULL ||              \
        (dbenv)->db_msgcall == NULL) {              \
        __db_msgfile(dbenv, fmt, __ap);             \
    }                               \
    va_end(__ap);                           \
}
#else
#define DB_REAL_MSG(dbenv, fmt) {                   \
    va_list __ap;                           \
                                    \
    /* Call the application's callback function, if specified. */   \
    va_start(__ap);                         \
    if ((dbenv) != NULL && (dbenv)->db_msgcall != NULL)     \
        __db_msgcall(dbenv, fmt, __ap);             \
    va_end(__ap);                           \
                                    \
    /*                              \
     * If the application specified a file descriptor, write to it. \
     * If we wrote to neither the application's callback routine or \
     * its file descriptor, write to stdout.            \
     */                             \
    va_start(__ap);                         \
    if ((dbenv) == NULL ||                      \
        (dbenv)->db_msgfile != NULL ||              \
        (dbenv)->db_msgcall == NULL) {              \
        __db_msgfile(dbenv, fmt, __ap);             \
    }                               \
    va_end(__ap);                           \
}
#endif

/*
 * Debugging macro to log operations.
 *  If DEBUG_WOP is defined, log operations that modify the database.
 *  If DEBUG_ROP is defined, log operations that read the database.
 *
 * D dbp
 * T txn
 * O operation (string)
 * K key
 * A data
 * F flags
 */
#define LOG_OP(C, T, O, K, A, F) {                  \
    DB_LSN __lsn;                           \
    DBT __op;                           \
    if (DBC_LOGGING((C))) {                     \
        memset(&__op, 0, sizeof(__op));             \
        __op.data = O;                      \
        __op.size = strlen(O) + 1;              \
        (void)__db_debug_log((C)->env, T, &__lsn, 0,        \
            &__op, (C)->dbp->log_filename->id, K, A, F);    \
    }                               \
}
#ifdef  DEBUG_ROP
#define DEBUG_LREAD(C, T, O, K, A, F)   LOG_OP(C, T, O, K, A, F)
#else
#define DEBUG_LREAD(C, T, O, K, A, F)
#endif
#ifdef  DEBUG_WOP
#define DEBUG_LWRITE(C, T, O, K, A, F)  LOG_OP(C, T, O, K, A, F)
#else
#define DEBUG_LWRITE(C, T, O, K, A, F)
#endif

/*
 * Hook for testing recovery at various places in the create/delete paths.
 * Hook for testing subdb locks.
 */
#if CONFIG_TEST
#define DB_TEST_SUBLOCKS(env, flags) do {               \
    if ((env)->test_abort == DB_TEST_SUBDB_LOCKS)           \
        (flags) |= DB_LOCK_NOWAIT;              \
} while (0)

#define DB_ENV_TEST_RECOVERY(env, val, ret, name) do {          \
    int __ret;                          \
    PANIC_CHECK((env));                     \
    if ((env)->test_copy == (val)) {                \
        /* COPY the FILE */                 \
        if ((__ret = __db_testcopy((env), NULL, (name))) != 0)  \
            (ret) = __env_panic((env), __ret);      \
    }                               \
    if ((env)->test_abort == (val)) {               \
        /* ABORT the TXN */                 \
        (env)->test_abort = 0;                  \
        (ret) = EINVAL;                     \
        goto db_tr_err;                     \
    }                               \
} while (0)

#define DB_TEST_RECOVERY(dbp, val, ret, name) do {          \
    ENV *__env = (dbp)->env;                    \
    int __ret;                          \
    PANIC_CHECK(__env);                     \
    if (__env->test_copy == (val)) {                \
        /* Copy the file. */                    \
        if (F_ISSET((dbp),                  \
            DB_AM_OPEN_CALLED) && (dbp)->mpf != NULL)       \
            (void)__db_sync(dbp);               \
        if ((__ret =                        \
            __db_testcopy(__env, (dbp), (name))) != 0)      \
            (ret) = __env_panic(__env, __ret);      \
    }                               \
    if (__env->test_abort == (val)) {               \
        /* Abort the transaction. */                \
        __env->test_abort = 0;                  \
        (ret) = EINVAL;                     \
        goto db_tr_err;                     \
    }                               \
} while (0)

#define DB_TEST_RECOVERY_LABEL  db_tr_err:

#define DB_TEST_WAIT(env, val)                      \
    if ((val) != 0)                         \
        __os_yield((env), (u_long)(val), 0)
#else
#define DB_TEST_SUBLOCKS(env, flags)
#define DB_ENV_TEST_RECOVERY(env, val, ret, name)
#define DB_TEST_RECOVERY(dbp, val, ret, name)
#define DB_TEST_RECOVERY_LABEL
#define DB_TEST_WAIT(env, val)
#endif

#if defined(__cplusplus)
}
#endif
#endif /* !_DB_DEBUG_H_ */
