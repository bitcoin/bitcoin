/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef _dbreg_ext_h_
#define _dbreg_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

int __dbreg_setup __P((DB *, const char *, const char *, u_int32_t));
int __dbreg_teardown __P((DB *));
int __dbreg_teardown_int __P((ENV *, FNAME *));
int __dbreg_new_id __P((DB *, DB_TXN *));
int __dbreg_get_id __P((DB *, DB_TXN *, int32_t *));
int __dbreg_assign_id __P((DB *, int32_t, int));
int __dbreg_revoke_id __P((DB *, int, int32_t));
int __dbreg_revoke_id_int __P((ENV *, FNAME *, int, int, int32_t));
int __dbreg_close_id __P((DB *, DB_TXN *, u_int32_t));
int __dbreg_close_id_int __P((ENV *, FNAME *, u_int32_t, int));
int __dbreg_failchk __P((ENV *));
int __dbreg_log_close __P((ENV *, FNAME *, DB_TXN *, u_int32_t));
int __dbreg_log_id __P((DB *, DB_TXN *, int32_t, int));
int __dbreg_register_read __P((ENV *, void *, __dbreg_register_args **));
int __dbreg_register_log __P((ENV *, DB_TXN *, DB_LSN *, u_int32_t, u_int32_t, const DBT *, const DBT *, int32_t, DBTYPE, db_pgno_t, u_int32_t));
int __dbreg_init_recover __P((ENV *, DB_DISTAB *));
int __dbreg_register_print __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __dbreg_init_print __P((ENV *, DB_DISTAB *));
int __dbreg_register_recover __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __dbreg_stat_print __P((ENV *, u_int32_t));
void __dbreg_print_fname __P((ENV *, FNAME *));
int __dbreg_add_dbentry __P((ENV *, DB_LOG *, DB *, int32_t));
int __dbreg_rem_dbentry __P((DB_LOG *, int32_t));
int __dbreg_log_files __P((ENV *, u_int32_t));
int __dbreg_close_files __P((ENV *, int));
int __dbreg_close_file __P((ENV *, FNAME *));
int __dbreg_mark_restored __P((ENV *));
int __dbreg_invalidate_files __P((ENV *, int));
int __dbreg_id_to_db __P((ENV *, DB_TXN *, DB **, int32_t, int));
int __dbreg_id_to_fname __P((DB_LOG *, int32_t, int, FNAME **));
int __dbreg_fid_to_fname __P((DB_LOG *, u_int8_t *, int, FNAME **));
int __dbreg_get_name __P((ENV *, u_int8_t *, char **, char **));
int __dbreg_do_open __P((ENV *, DB_TXN *, DB_LOG *, u_int8_t *, char *, DBTYPE, int32_t, db_pgno_t, void *, u_int32_t, u_int32_t));
int __dbreg_lazy_id __P((DB *));

#if defined(__cplusplus)
}
#endif
#endif /* !_dbreg_ext_h_ */
