/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef _mutex_ext_h_
#define _mutex_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

int __mutex_alloc __P((ENV *, int, u_int32_t, db_mutex_t *));
int __mutex_alloc_int __P((ENV *, int, int, u_int32_t, db_mutex_t *));
int __mutex_free __P((ENV *, db_mutex_t *));
int __mutex_free_int __P((ENV *, int, db_mutex_t *));
int __mut_failchk __P((ENV *));
int __db_fcntl_mutex_init __P((ENV *, db_mutex_t, u_int32_t));
int __db_fcntl_mutex_lock __P((ENV *, db_mutex_t));
int __db_fcntl_mutex_trylock __P((ENV *, db_mutex_t));
int __db_fcntl_mutex_unlock __P((ENV *, db_mutex_t));
int __db_fcntl_mutex_destroy __P((ENV *, db_mutex_t));
int __mutex_alloc_pp __P((DB_ENV *, u_int32_t, db_mutex_t *));
int __mutex_free_pp __P((DB_ENV *, db_mutex_t));
int __mutex_lock_pp __P((DB_ENV *, db_mutex_t));
int __mutex_unlock_pp __P((DB_ENV *, db_mutex_t));
int __mutex_get_align __P((DB_ENV *, u_int32_t *));
int __mutex_set_align __P((DB_ENV *, u_int32_t));
int __mutex_get_increment __P((DB_ENV *, u_int32_t *));
int __mutex_set_increment __P((DB_ENV *, u_int32_t));
int __mutex_get_max __P((DB_ENV *, u_int32_t *));
int __mutex_set_max __P((DB_ENV *, u_int32_t));
int __mutex_get_tas_spins __P((DB_ENV *, u_int32_t *));
int __mutex_set_tas_spins __P((DB_ENV *, u_int32_t));
#if !defined(HAVE_ATOMIC_SUPPORT) && defined(HAVE_MUTEX_SUPPORT)
atomic_value_t __atomic_inc __P((ENV *, db_atomic_t *));
#endif
#if !defined(HAVE_ATOMIC_SUPPORT) && defined(HAVE_MUTEX_SUPPORT)
atomic_value_t __atomic_dec __P((ENV *, db_atomic_t *));
#endif
int __db_pthread_mutex_init __P((ENV *, db_mutex_t, u_int32_t));
int __db_pthread_mutex_lock __P((ENV *, db_mutex_t));
#if defined(HAVE_SHARED_LATCHES)
int __db_pthread_mutex_readlock __P((ENV *, db_mutex_t));
#endif
int __db_pthread_mutex_unlock __P((ENV *, db_mutex_t));
int __db_pthread_mutex_destroy __P((ENV *, db_mutex_t));
int __mutex_open __P((ENV *, int));
int __mutex_env_refresh __P((ENV *));
void __mutex_resource_return __P((ENV *, REGINFO *));
int __mutex_stat_pp __P((DB_ENV *, DB_MUTEX_STAT **, u_int32_t));
int __mutex_stat_print_pp __P((DB_ENV *, u_int32_t));
int __mutex_stat_print __P((ENV *, u_int32_t));
void __mutex_print_debug_single __P((ENV *, const char *, db_mutex_t, u_int32_t));
void __mutex_print_debug_stats __P((ENV *, DB_MSGBUF *, db_mutex_t, u_int32_t));
void __mutex_set_wait_info __P((ENV *, db_mutex_t, uintmax_t *, uintmax_t *));
void __mutex_clear __P((ENV *, db_mutex_t));
int __db_tas_mutex_init __P((ENV *, db_mutex_t, u_int32_t));
int __db_tas_mutex_lock __P((ENV *, db_mutex_t));
int __db_tas_mutex_trylock __P((ENV *, db_mutex_t));
#if defined(HAVE_SHARED_LATCHES)
int __db_tas_mutex_readlock __P((ENV *, db_mutex_t));
#endif
#if defined(HAVE_SHARED_LATCHES)
int __db_tas_mutex_tryreadlock __P((ENV *, db_mutex_t));
#endif
int __db_tas_mutex_unlock __P((ENV *, db_mutex_t));
int __db_tas_mutex_destroy __P((ENV *, db_mutex_t));
int __db_win32_mutex_init __P((ENV *, db_mutex_t, u_int32_t));
int __db_win32_mutex_lock __P((ENV *, db_mutex_t));
int __db_win32_mutex_trylock __P((ENV *, db_mutex_t));
#if defined(HAVE_SHARED_LATCHES)
int __db_win32_mutex_readlock __P((ENV *, db_mutex_t));
#endif
#if defined(HAVE_SHARED_LATCHES)
int __db_win32_mutex_tryreadlock __P((ENV *, db_mutex_t));
#endif
int __db_win32_mutex_unlock __P((ENV *, db_mutex_t));
int __db_win32_mutex_destroy __P((ENV *, db_mutex_t));

#if defined(__cplusplus)
}
#endif
#endif /* !_mutex_ext_h_ */
