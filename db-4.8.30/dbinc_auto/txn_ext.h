/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef _txn_ext_h_
#define _txn_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

int __txn_begin_pp __P((DB_ENV *, DB_TXN *, DB_TXN **, u_int32_t));
int __txn_begin __P((ENV *, DB_THREAD_INFO *, DB_TXN *, DB_TXN **, u_int32_t));
int __txn_recycle_id __P((ENV *));
int __txn_compensate_begin __P((ENV *, DB_TXN **));
int __txn_continue __P((ENV *, DB_TXN *, TXN_DETAIL *));
int __txn_commit __P((DB_TXN *, u_int32_t));
int __txn_abort __P((DB_TXN *));
int __txn_discard_int __P((DB_TXN *, u_int32_t flags));
int __txn_prepare __P((DB_TXN *, u_int8_t *));
u_int32_t __txn_id __P((DB_TXN *));
int __txn_get_name __P((DB_TXN *, const char **));
int __txn_set_name __P((DB_TXN *, const char *));
int  __txn_set_timeout __P((DB_TXN *, db_timeout_t, u_int32_t));
int __txn_activekids __P((ENV *, u_int32_t, DB_TXN *));
int __txn_force_abort __P((ENV *, u_int8_t *));
int __txn_preclose __P((ENV *));
int __txn_reset __P((ENV *));
int __txn_regop_42_read __P((ENV *, void *, __txn_regop_42_args **));
int __txn_regop_read __P((ENV *, void *, __txn_regop_args **));
int __txn_regop_log __P((ENV *, DB_TXN *, DB_LSN *, u_int32_t, u_int32_t, int32_t, u_int32_t, const DBT *));
int __txn_ckp_42_read __P((ENV *, void *, __txn_ckp_42_args **));
int __txn_ckp_read __P((ENV *, void *, __txn_ckp_args **));
int __txn_ckp_log __P((ENV *, DB_TXN *, DB_LSN *, u_int32_t, DB_LSN *, DB_LSN *, int32_t, u_int32_t, u_int32_t));
int __txn_child_read __P((ENV *, void *, __txn_child_args **));
int __txn_child_log __P((ENV *, DB_TXN *, DB_LSN *, u_int32_t, u_int32_t, DB_LSN *));
int __txn_xa_regop_42_read __P((ENV *, void *, __txn_xa_regop_42_args **));
int __txn_prepare_read __P((ENV *, void *, __txn_prepare_args **));
int __txn_prepare_log __P((ENV *, DB_TXN *, DB_LSN *, u_int32_t, u_int32_t, const DBT *, DB_LSN *, const DBT *));
int __txn_recycle_read __P((ENV *, void *, __txn_recycle_args **));
int __txn_recycle_log __P((ENV *, DB_TXN *, DB_LSN *, u_int32_t, u_int32_t, u_int32_t));
int __txn_init_recover __P((ENV *, DB_DISTAB *));
int __txn_regop_42_print __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __txn_regop_print __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __txn_ckp_42_print __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __txn_ckp_print __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __txn_child_print __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __txn_xa_regop_42_print __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __txn_prepare_print __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __txn_recycle_print __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __txn_init_print __P((ENV *, DB_DISTAB *));
int __txn_checkpoint_pp __P((DB_ENV *, u_int32_t, u_int32_t, u_int32_t));
int __txn_checkpoint __P((ENV *, u_int32_t, u_int32_t, u_int32_t));
int __txn_getactive __P((ENV *, DB_LSN *));
int __txn_getckp __P((ENV *, DB_LSN *));
int __txn_updateckp __P((ENV *, DB_LSN *));
int __txn_failchk __P((ENV *));
int __txn_env_create __P((DB_ENV *));
void __txn_env_destroy __P((DB_ENV *));
int __txn_get_tx_max __P((DB_ENV *, u_int32_t *));
int __txn_set_tx_max __P((DB_ENV *, u_int32_t));
int __txn_get_tx_timestamp __P((DB_ENV *, time_t *));
int __txn_set_tx_timestamp __P((DB_ENV *, time_t *));
int __txn_regop_recover __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __txn_prepare_recover __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __txn_ckp_recover __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __txn_child_recover __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __txn_restore_txn __P((ENV *, DB_LSN *, __txn_prepare_args *));
int __txn_recycle_recover __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __txn_regop_42_recover __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __txn_ckp_42_recover __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __txn_map_gid __P((ENV *, u_int8_t *, TXN_DETAIL **, roff_t *));
int __txn_recover_pp __P((DB_ENV *, DB_PREPLIST *, u_int32_t, u_int32_t *, u_int32_t));
int __txn_recover __P((ENV *, DB_PREPLIST *, u_int32_t, u_int32_t *, u_int32_t));
int __txn_openfiles __P((ENV *, DB_THREAD_INFO *, DB_LSN *, int));
int __txn_open __P((ENV *, int));
int __txn_findlastckp __P((ENV *, DB_LSN *, DB_LSN *));
int __txn_env_refresh __P((ENV *));
u_int32_t __txn_region_mutex_count __P((ENV *));
int __txn_id_set __P((ENV *, u_int32_t, u_int32_t));
int __txn_oldest_reader __P((ENV *, DB_LSN *));
int __txn_add_buffer __P((ENV *, TXN_DETAIL *));
int __txn_remove_buffer __P((ENV *, TXN_DETAIL *, db_mutex_t));
int __txn_stat_pp __P((DB_ENV *, DB_TXN_STAT **, u_int32_t));
int __txn_stat_print_pp __P((DB_ENV *, u_int32_t));
int  __txn_stat_print __P((ENV *, u_int32_t));
int __txn_closeevent __P((ENV *, DB_TXN *, DB *));
int __txn_remevent __P((ENV *, DB_TXN *, const char *, u_int8_t *, int));
void __txn_remrem __P((ENV *, DB_TXN *, const char *));
int __txn_lockevent __P((ENV *, DB_TXN *, DB *, DB_LOCK *, DB_LOCKER *));
void __txn_remlock __P((ENV *, DB_TXN *, DB_LOCK *, DB_LOCKER *));
int __txn_doevents __P((ENV *, DB_TXN *, int, int));
int __txn_record_fname __P((ENV *, DB_TXN *, FNAME *));
int __txn_dref_fname __P((ENV *, DB_TXN *));

#if defined(__cplusplus)
}
#endif
#endif /* !_txn_ext_h_ */
