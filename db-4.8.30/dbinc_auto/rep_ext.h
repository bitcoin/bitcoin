/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef _rep_ext_h_
#define _rep_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

int __rep_bulk_marshal __P((ENV *, __rep_bulk_args *, u_int8_t *, size_t, size_t *));
int __rep_bulk_unmarshal __P((ENV *, __rep_bulk_args *, u_int8_t *, size_t, u_int8_t **));
int __rep_control_marshal __P((ENV *, __rep_control_args *, u_int8_t *, size_t, size_t *));
int __rep_control_unmarshal __P((ENV *, __rep_control_args *, u_int8_t *, size_t, u_int8_t **));
int __rep_egen_marshal __P((ENV *, __rep_egen_args *, u_int8_t *, size_t, size_t *));
int __rep_egen_unmarshal __P((ENV *, __rep_egen_args *, u_int8_t *, size_t, u_int8_t **));
int __rep_fileinfo_marshal __P((ENV *, u_int32_t, __rep_fileinfo_args *, u_int8_t *, size_t, size_t *));
int __rep_fileinfo_unmarshal __P((ENV *, u_int32_t, __rep_fileinfo_args **, u_int8_t *, size_t, u_int8_t **));
int __rep_grant_info_marshal __P((ENV *, __rep_grant_info_args *, u_int8_t *, size_t, size_t *));
int __rep_grant_info_unmarshal __P((ENV *, __rep_grant_info_args *, u_int8_t *, size_t, u_int8_t **));
int __rep_logreq_marshal __P((ENV *, __rep_logreq_args *, u_int8_t *, size_t, size_t *));
int __rep_logreq_unmarshal __P((ENV *, __rep_logreq_args *, u_int8_t *, size_t, u_int8_t **));
int __rep_newfile_marshal __P((ENV *, __rep_newfile_args *, u_int8_t *, size_t, size_t *));
int __rep_newfile_unmarshal __P((ENV *, __rep_newfile_args *, u_int8_t *, size_t, u_int8_t **));
int __rep_update_marshal __P((ENV *, u_int32_t, __rep_update_args *, u_int8_t *, size_t, size_t *));
int __rep_update_unmarshal __P((ENV *, u_int32_t, __rep_update_args **, u_int8_t *, size_t, u_int8_t **));
int __rep_vote_info_marshal __P((ENV *, __rep_vote_info_args *, u_int8_t *, size_t, size_t *));
int __rep_vote_info_unmarshal __P((ENV *, __rep_vote_info_args *, u_int8_t *, size_t, u_int8_t **));
int __rep_update_req __P((ENV *, __rep_control_args *, int));
int __rep_page_req __P((ENV *, DB_THREAD_INFO *, int, __rep_control_args *, DBT *));
int __rep_update_setup __P((ENV *, int, __rep_control_args *, DBT *, time_t));
int __rep_bulk_page __P((ENV *, DB_THREAD_INFO *, int, __rep_control_args *, DBT *));
int __rep_page __P((ENV *, DB_THREAD_INFO *, int, __rep_control_args *, DBT *));
int __rep_page_fail __P((ENV *, DB_THREAD_INFO *, int, __rep_control_args *, DBT *));
int __rep_init_cleanup __P((ENV *, REP *, int));
int __rep_pggap_req __P((ENV *, REP *, __rep_fileinfo_args *, u_int32_t));
int __rep_finfo_alloc __P((ENV *, __rep_fileinfo_args *, __rep_fileinfo_args **));
int __rep_remove_init_file __P((ENV *));
int __rep_reset_init __P((ENV *));
int __rep_elect_pp __P((DB_ENV *, u_int32_t, u_int32_t, u_int32_t));
int __rep_elect_int __P((ENV *, u_int32_t, u_int32_t, u_int32_t));
int __rep_vote1 __P((ENV *, __rep_control_args *, DBT *, int));
int __rep_vote2 __P((ENV *, __rep_control_args *, DBT *, int));
int __rep_update_grant __P((ENV *, db_timespec *));
int __rep_islease_granted __P((ENV *));
int __rep_lease_table_alloc __P((ENV *, u_int32_t));
int __rep_lease_grant __P((ENV *, __rep_control_args *, DBT *, int));
int __rep_lease_check __P((ENV *, int));
int __rep_lease_refresh __P((ENV *));
int __rep_lease_expire __P((ENV *));
db_timeout_t __rep_lease_waittime __P((ENV *));
int __rep_allreq __P((ENV *, __rep_control_args *, int));
int __rep_log __P((ENV *, DB_THREAD_INFO *, __rep_control_args *, DBT *, time_t, DB_LSN *));
int __rep_bulk_log __P((ENV *, DB_THREAD_INFO *, __rep_control_args *, DBT *, time_t, DB_LSN *));
int __rep_logreq __P((ENV *, __rep_control_args *, DBT *, int));
int __rep_loggap_req __P((ENV *, REP *, DB_LSN *, u_int32_t));
int __rep_logready __P((ENV *, REP *, time_t, DB_LSN *));
int __rep_env_create __P((DB_ENV *));
void __rep_env_destroy __P((DB_ENV *));
int __rep_get_config __P((DB_ENV *, u_int32_t, int *));
int __rep_set_config __P((DB_ENV *, u_int32_t, int));
int __rep_start_pp __P((DB_ENV *, DBT *, u_int32_t));
int __rep_start_int __P((ENV *, DBT *, u_int32_t));
int __rep_client_dbinit __P((ENV *, int, repdb_t));
int __rep_get_limit __P((DB_ENV *, u_int32_t *, u_int32_t *));
int __rep_set_limit __P((DB_ENV *, u_int32_t, u_int32_t));
int __rep_set_nsites __P((DB_ENV *, u_int32_t));
int __rep_get_nsites __P((DB_ENV *, u_int32_t *));
int __rep_set_priority __P((DB_ENV *, u_int32_t));
int __rep_get_priority __P((DB_ENV *, u_int32_t *));
int __rep_set_timeout __P((DB_ENV *, int, db_timeout_t));
int __rep_get_timeout __P((DB_ENV *, int, db_timeout_t *));
int __rep_get_request __P((DB_ENV *, db_timeout_t *, db_timeout_t *));
int __rep_set_request __P((DB_ENV *, db_timeout_t, db_timeout_t));
int __rep_set_transport_pp __P((DB_ENV *, int, int (*)(DB_ENV *, const DBT *, const DBT *, const DB_LSN *, int, u_int32_t)));
int __rep_set_transport_int __P((ENV *, int, int (*)(DB_ENV *, const DBT *, const DBT *, const DB_LSN *, int, u_int32_t)));
int __rep_get_clockskew __P((DB_ENV *, u_int32_t *, u_int32_t *));
int __rep_set_clockskew __P((DB_ENV *, u_int32_t, u_int32_t));
int __rep_flush __P((DB_ENV *));
int __rep_sync __P((DB_ENV *, u_int32_t));
int __rep_process_message_pp __P((DB_ENV *, DBT *, DBT *, int, DB_LSN *));
int __rep_process_message_int __P((ENV *, DBT *, DBT *, int, DB_LSN *));
int __rep_apply __P((ENV *, DB_THREAD_INFO *, __rep_control_args *, DBT *, DB_LSN *, int *, DB_LSN *));
int __rep_process_txn __P((ENV *, DBT *));
int __rep_resend_req __P((ENV *, int));
int __rep_check_doreq __P((ENV *, REP *));
int __rep_open __P((ENV *));
int __rep_env_refresh __P((ENV *));
int __rep_env_close __P((ENV *));
int __rep_preclose __P((ENV *));
int __rep_closefiles __P((ENV *));
int __rep_write_egen __P((ENV *, REP *, u_int32_t));
int __rep_write_gen __P((ENV *, REP *, u_int32_t));
int __rep_stat_pp __P((DB_ENV *, DB_REP_STAT **, u_int32_t));
int __rep_stat_print_pp __P((DB_ENV *, u_int32_t));
int __rep_stat_print __P((ENV *, u_int32_t));
int __rep_bulk_message __P((ENV *, REP_BULK *, REP_THROTTLE *, DB_LSN *, const DBT *, u_int32_t));
int __rep_send_bulk __P((ENV *, REP_BULK *, u_int32_t));
int __rep_bulk_alloc __P((ENV *, REP_BULK *, int, uintptr_t *, u_int32_t *, u_int32_t));
int __rep_bulk_free __P((ENV *, REP_BULK *, u_int32_t));
int __rep_send_message __P((ENV *, int, u_int32_t, DB_LSN *, const DBT *, u_int32_t, u_int32_t));
int __rep_new_master __P((ENV *, __rep_control_args *, int));
int __rep_noarchive __P((ENV *));
void __rep_send_vote __P((ENV *, DB_LSN *, u_int32_t, u_int32_t, u_int32_t, u_int32_t, u_int32_t, int, u_int32_t, u_int32_t));
void __rep_elect_done __P((ENV *, REP *, int));
int __env_rep_enter __P((ENV *, int));
int __env_db_rep_exit __P((ENV *));
int __db_rep_enter __P((DB *, int, int, int));
int __op_rep_enter __P((ENV *));
int __op_rep_exit __P((ENV *));
int __rep_lockout_api __P((ENV *, REP *));
int __rep_lockout_apply __P((ENV *, REP *, u_int32_t));
int __rep_lockout_msg __P((ENV *, REP *, u_int32_t));
int __rep_send_throttle __P((ENV *, int, REP_THROTTLE *, u_int32_t, u_int32_t));
u_int32_t __rep_msg_to_old __P((u_int32_t, u_int32_t));
u_int32_t __rep_msg_from_old __P((u_int32_t, u_int32_t));
void __rep_print __P((ENV *, const char *, ...)) __attribute__ ((__format__ (__printf__, 2, 3)));
void __rep_print_message __P((ENV *, int, __rep_control_args *, char *, u_int32_t));
void __rep_fire_event __P((ENV *, u_int32_t, void *));
int __rep_verify __P((ENV *, __rep_control_args *, DBT *, int, time_t));
int __rep_verify_fail __P((ENV *, __rep_control_args *));
int __rep_verify_req __P((ENV *, __rep_control_args *, int));
int __rep_dorecovery __P((ENV *, DB_LSN *, DB_LSN *));
int __rep_verify_match __P((ENV *, DB_LSN *, time_t));
int __rep_log_backup __P((ENV *, REP *, DB_LOGC *, DB_LSN *));

#if defined(__cplusplus)
}
#endif
#endif /* !_rep_ext_h_ */
