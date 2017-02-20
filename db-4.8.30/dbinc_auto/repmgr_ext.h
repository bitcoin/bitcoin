/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef _repmgr_ext_h_
#define _repmgr_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

void __repmgr_handshake_marshal __P((ENV *, __repmgr_handshake_args *, u_int8_t *));
int __repmgr_handshake_unmarshal __P((ENV *, __repmgr_handshake_args *, u_int8_t *, size_t, u_int8_t **));
void __repmgr_v2handshake_marshal __P((ENV *, __repmgr_v2handshake_args *, u_int8_t *));
int __repmgr_v2handshake_unmarshal __P((ENV *, __repmgr_v2handshake_args *, u_int8_t *, size_t, u_int8_t **));
void __repmgr_ack_marshal __P((ENV *, __repmgr_ack_args *, u_int8_t *));
int __repmgr_ack_unmarshal __P((ENV *, __repmgr_ack_args *, u_int8_t *, size_t, u_int8_t **));
void __repmgr_version_proposal_marshal __P((ENV *, __repmgr_version_proposal_args *, u_int8_t *));
int __repmgr_version_proposal_unmarshal __P((ENV *, __repmgr_version_proposal_args *, u_int8_t *, size_t, u_int8_t **));
void __repmgr_version_confirmation_marshal __P((ENV *, __repmgr_version_confirmation_args *, u_int8_t *));
int __repmgr_version_confirmation_unmarshal __P((ENV *, __repmgr_version_confirmation_args *, u_int8_t *, size_t, u_int8_t **));
int __repmgr_init_election __P((ENV *, int));
int __repmgr_become_master __P((ENV *));
int __repmgr_start __P((DB_ENV *, int, u_int32_t));
int __repmgr_autostart __P((ENV *));
int __repmgr_start_selector __P((ENV *));
int __repmgr_close __P((ENV *));
int __repmgr_set_ack_policy __P((DB_ENV *, int));
int __repmgr_get_ack_policy __P((DB_ENV *, int *));
int __repmgr_env_create __P((ENV *, DB_REP *));
void __repmgr_env_destroy __P((ENV *, DB_REP *));
int __repmgr_stop_threads __P((ENV *));
int __repmgr_set_local_site __P((DB_ENV *, const char *, u_int, u_int32_t));
int __repmgr_add_remote_site __P((DB_ENV *, const char *, u_int, int *, u_int32_t));
void *__repmgr_msg_thread __P((void *));
int __repmgr_handle_event __P((ENV *, u_int32_t, void *));
int __repmgr_send __P((DB_ENV *, const DBT *, const DBT *, const DB_LSN *, int, u_int32_t));
int __repmgr_sync_siteaddr __P((ENV *));
int __repmgr_send_broadcast __P((ENV *, u_int, const DBT *, const DBT *, u_int *, u_int *));
int __repmgr_send_one __P((ENV *, REPMGR_CONNECTION *, u_int, const DBT *, const DBT *, int));
int __repmgr_is_permanent __P((ENV *, const DB_LSN *));
int __repmgr_bust_connection __P((ENV *, REPMGR_CONNECTION *));
void __repmgr_disable_connection __P((ENV *, REPMGR_CONNECTION *));
int __repmgr_cleanup_connection __P((ENV *, REPMGR_CONNECTION *));
REPMGR_SITE *__repmgr_find_site __P((ENV *, const char *, u_int));
int __repmgr_pack_netaddr __P((ENV *, const char *, u_int, ADDRINFO *, repmgr_netaddr_t *));
int __repmgr_getaddr __P((ENV *, const char *, u_int, int, ADDRINFO **));
int __repmgr_add_site __P((ENV *, const char *, u_int, REPMGR_SITE **, u_int32_t));
int __repmgr_add_site_int __P((ENV *, const char *, u_int, REPMGR_SITE **, int, int));
int __repmgr_listen __P((ENV *));
int __repmgr_net_close __P((ENV *));
void __repmgr_net_destroy __P((ENV *, DB_REP *));
int __repmgr_thread_start __P((ENV *, REPMGR_RUNNABLE *));
int __repmgr_thread_join __P((REPMGR_RUNNABLE *));
int __repmgr_set_nonblocking __P((socket_t));
int __repmgr_wake_waiting_senders __P((ENV *));
int __repmgr_await_ack __P((ENV *, const DB_LSN *));
void __repmgr_compute_wait_deadline __P((ENV*, struct timespec *, db_timeout_t));
int __repmgr_await_drain __P((ENV *, REPMGR_CONNECTION *, db_timeout_t));
int __repmgr_alloc_cond __P((cond_var_t *));
int __repmgr_free_cond __P((cond_var_t *));
void __repmgr_env_create_pf __P((DB_REP *));
int __repmgr_create_mutex_pf __P((mgr_mutex_t *));
int __repmgr_destroy_mutex_pf __P((mgr_mutex_t *));
int __repmgr_init __P((ENV *));
int __repmgr_deinit __P((ENV *));
int __repmgr_lock_mutex __P((mgr_mutex_t *));
int __repmgr_unlock_mutex __P((mgr_mutex_t *));
int __repmgr_signal __P((cond_var_t *));
int __repmgr_wake_main_thread __P((ENV*));
int __repmgr_writev __P((socket_t, db_iovec_t *, int, size_t *));
int __repmgr_readv __P((socket_t, db_iovec_t *, int, size_t *));
int __repmgr_select_loop __P((ENV *));
void __repmgr_queue_destroy __P((ENV *));
int __repmgr_queue_get __P((ENV *, REPMGR_MESSAGE **));
int __repmgr_queue_put __P((ENV *, REPMGR_MESSAGE *));
int __repmgr_queue_size __P((ENV *));
void *__repmgr_select_thread __P((void *));
int __repmgr_accept __P((ENV *));
int __repmgr_compute_timeout __P((ENV *, db_timespec *));
int __repmgr_check_timeouts __P((ENV *));
int __repmgr_first_try_connections __P((ENV *));
int __repmgr_connect_site __P((ENV *, u_int eid));
int __repmgr_propose_version __P((ENV *, REPMGR_CONNECTION *));
int __repmgr_read_from_site __P((ENV *, REPMGR_CONNECTION *));
int __repmgr_write_some __P((ENV *, REPMGR_CONNECTION *));
int __repmgr_stat_pp __P((DB_ENV *, DB_REPMGR_STAT **, u_int32_t));
int __repmgr_stat_print_pp __P((DB_ENV *, u_int32_t));
int __repmgr_site_list __P((DB_ENV *, u_int *, DB_REPMGR_SITE **));
#ifndef HAVE_REPLICATION_THREADS
int __repmgr_close __P((ENV *));
#endif
#ifndef HAVE_REPLICATION_THREADS
int __repmgr_add_remote_site __P((DB_ENV *, const char *, u_int, int *, u_int32_t));
#endif
#ifndef HAVE_REPLICATION_THREADS
int __repmgr_get_ack_policy __P((DB_ENV *, int *));
#endif
#ifndef HAVE_REPLICATION_THREADS
int __repmgr_set_ack_policy __P((DB_ENV *, int));
#endif
#ifndef HAVE_REPLICATION_THREADS
int __repmgr_set_local_site __P((DB_ENV *, const char *, u_int, u_int32_t));
#endif
#ifndef HAVE_REPLICATION_THREADS
int __repmgr_site_list __P((DB_ENV *, u_int *, DB_REPMGR_SITE **));
#endif
#ifndef HAVE_REPLICATION_THREADS
int __repmgr_start __P((DB_ENV *, int, u_int32_t));
#endif
#ifndef HAVE_REPLICATION_THREADS
int __repmgr_stat_pp __P((DB_ENV *, DB_REPMGR_STAT **, u_int32_t));
#endif
#ifndef HAVE_REPLICATION_THREADS
int __repmgr_stat_print_pp __P((DB_ENV *, u_int32_t));
#endif
#ifndef HAVE_REPLICATION_THREADS
int __repmgr_handle_event __P((ENV *, u_int32_t, void *));
#endif
int __repmgr_schedule_connection_attempt __P((ENV *, u_int, int));
void __repmgr_reset_for_reading __P((REPMGR_CONNECTION *));
int __repmgr_new_connection __P((ENV *, REPMGR_CONNECTION **, socket_t, int));
int __repmgr_new_site __P((ENV *, REPMGR_SITE**, const char *, u_int, int));
void __repmgr_cleanup_netaddr __P((ENV *, repmgr_netaddr_t *));
void __repmgr_iovec_init __P((REPMGR_IOVECS *));
void __repmgr_add_buffer __P((REPMGR_IOVECS *, void *, size_t));
void __repmgr_add_dbt __P((REPMGR_IOVECS *, const DBT *));
int __repmgr_update_consumed __P((REPMGR_IOVECS *, size_t));
int __repmgr_prepare_my_addr __P((ENV *, DBT *));
u_int __repmgr_get_nsites __P((DB_REP *));
void __repmgr_thread_failure __P((ENV *, int));
char *__repmgr_format_eid_loc __P((DB_REP *, int, char *));
char *__repmgr_format_site_loc __P((REPMGR_SITE *, char *));
int __repmgr_repstart __P((ENV *, u_int32_t));
int __repmgr_each_connection __P((ENV *, CONNECTION_ACTION, void *, int));
int __repmgr_open __P((ENV *, void *));
int __repmgr_join __P((ENV *, void *));
int __repmgr_env_refresh __P((ENV *env));
int __repmgr_share_netaddrs __P((ENV *, void *, u_int, u_int));
int __repmgr_copy_in_added_sites __P((ENV *));
int __repmgr_init_new_sites __P((ENV *, u_int, u_int));
int __repmgr_check_host_name __P((ENV *, int));
int __repmgr_failchk __P((ENV *));

#if defined(__cplusplus)
}
#endif
#endif /* !_repmgr_ext_h_ */
