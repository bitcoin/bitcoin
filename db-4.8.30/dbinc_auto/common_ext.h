/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef _common_ext_h_
#define _common_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

int __crypto_region_init __P((ENV *));
int __db_isbigendian __P((void));
int __db_byteorder __P((ENV *, int));
u_int32_t __db_compress_count_int __P((u_int64_t));
int __db_compress_int __P((u_int8_t *, u_int64_t));
u_int32_t __db_decompress_count_int __P((const u_int8_t *));
int __db_decompress_int __P((const u_int8_t *, u_int64_t *));
int __db_decompress_int32 __P((const u_int8_t *, u_int32_t *));
int __db_fchk __P((ENV *, const char *, u_int32_t, u_int32_t));
int __db_fcchk __P((ENV *, const char *, u_int32_t, u_int32_t, u_int32_t));
int __db_ferr __P((const ENV *, const char *, int));
int __db_fnl __P((const ENV *, const char *));
int __db_pgerr __P((DB *, db_pgno_t, int));
int __db_pgfmt __P((ENV *, db_pgno_t));
#ifdef DIAGNOSTIC
void __db_assert __P((ENV *, const char *, const char *, int));
#endif
int __env_panic_msg __P((ENV *));
int __env_panic __P((ENV *, int));
char *__db_unknown_error __P((int));
void __db_syserr __P((const ENV *, int, const char *, ...)) __attribute__ ((__format__ (__printf__, 3, 4)));
void __db_err __P((const ENV *, int, const char *, ...)) __attribute__ ((__format__ (__printf__, 3, 4)));
void __db_errx __P((const ENV *, const char *, ...)) __attribute__ ((__format__ (__printf__, 2, 3)));
void __db_errcall __P((const DB_ENV *, int, db_error_set_t, const char *, va_list));
void __db_errfile __P((const DB_ENV *, int, db_error_set_t, const char *, va_list));
void __db_msgadd __P((ENV *, DB_MSGBUF *, const char *, ...)) __attribute__ ((__format__ (__printf__, 3, 4)));
void __db_msgadd_ap __P((ENV *, DB_MSGBUF *, const char *, va_list));
void __db_msg __P((const ENV *, const char *, ...)) __attribute__ ((__format__ (__printf__, 2, 3)));
int __db_unknown_flag __P((ENV *, char *, u_int32_t));
int __db_unknown_type __P((ENV *, char *, DBTYPE));
int __db_unknown_path __P((ENV *, char *));
int __db_check_txn __P((DB *, DB_TXN *, DB_LOCKER *, int));
int __db_txn_deadlock_err __P((ENV *, DB_TXN *));
int __db_not_txn_env __P((ENV *));
int __db_rec_toobig __P((ENV *, u_int32_t, u_int32_t));
int __db_rec_repl __P((ENV *, u_int32_t, u_int32_t));
int __dbc_logging __P((DBC *));
int __db_check_lsn __P((ENV *, DB_LSN *, DB_LSN *));
int __db_rdonly __P((const ENV *, const char *));
int __db_space_err __P((const DB *));
int __db_failed __P((const ENV *, const char *, pid_t, db_threadid_t));
int __db_getlong __P((DB_ENV *, const char *, char *, long, long, long *));
int __db_getulong __P((DB_ENV *, const char *, char *, u_long, u_long, u_long *));
void __db_idspace __P((u_int32_t *, int, u_int32_t *, u_int32_t *));
u_int32_t __db_log2 __P((u_int32_t));
u_int32_t __db_tablesize __P((u_int32_t));
void __db_hashinit __P((void *, u_int32_t));
int __dbt_usercopy __P((ENV *, DBT *));
void __dbt_userfree __P((ENV *, DBT *, DBT *, DBT *));
int __db_mkpath __P((ENV *, const char *));
u_int32_t __db_openflags __P((int));
int __db_util_arg __P((char *, char *, int *, char ***));
int __db_util_cache __P((DB *, u_int32_t *, int *));
int __db_util_logset __P((const char *, char *));
void __db_util_siginit __P((void));
int __db_util_interrupted __P((void));
void __db_util_sigresend __P((void));
int __db_zero_fill __P((ENV *, DB_FH *));
int __db_zero_extend __P((ENV *, DB_FH *, db_pgno_t, db_pgno_t, u_int32_t));

#if defined(__cplusplus)
}
#endif
#endif /* !_common_ext_h_ */
