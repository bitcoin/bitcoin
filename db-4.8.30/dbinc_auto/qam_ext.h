/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef _qam_ext_h_
#define _qam_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

int __qam_position __P((DBC *, db_recno_t *, u_int32_t, int *));
int __qam_pitem __P((DBC *,  QPAGE *, u_int32_t, db_recno_t, DBT *));
int __qam_append __P((DBC *, DBT *, DBT *));
int __qamc_dup __P((DBC *, DBC *));
int __qamc_init __P((DBC *));
int __qam_truncate __P((DBC *, u_int32_t *));
int __qam_delete __P((DBC *,  DBT *, u_int32_t));
int __qam_incfirst_read __P((ENV *, DB **, void *, void *, __qam_incfirst_args **));
int __qam_incfirst_log __P((DB *, DB_TXN *, DB_LSN *, u_int32_t, db_recno_t, db_pgno_t));
int __qam_mvptr_read __P((ENV *, DB **, void *, void *, __qam_mvptr_args **));
int __qam_mvptr_log __P((DB *, DB_TXN *, DB_LSN *, u_int32_t, u_int32_t, db_recno_t, db_recno_t, db_recno_t, db_recno_t, DB_LSN *, db_pgno_t));
int __qam_del_read __P((ENV *, DB **, void *, void *, __qam_del_args **));
int __qam_del_log __P((DB *, DB_TXN *, DB_LSN *, u_int32_t, DB_LSN *, db_pgno_t, u_int32_t, db_recno_t));
int __qam_add_read __P((ENV *, DB **, void *, void *, __qam_add_args **));
int __qam_add_log __P((DB *, DB_TXN *, DB_LSN *, u_int32_t, DB_LSN *, db_pgno_t, u_int32_t, db_recno_t, const DBT *, u_int32_t, const DBT *));
int __qam_delext_read __P((ENV *, DB **, void *, void *, __qam_delext_args **));
int __qam_delext_log __P((DB *, DB_TXN *, DB_LSN *, u_int32_t, DB_LSN *, db_pgno_t, u_int32_t, db_recno_t, const DBT *));
int __qam_init_recover __P((ENV *, DB_DISTAB *));
int __qam_incfirst_print __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_mvptr_print __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_del_print __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_add_print __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_delext_print __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_init_print __P((ENV *, DB_DISTAB *));
int __qam_mswap __P((ENV *, PAGE *));
int __qam_pgin_out __P((ENV *, db_pgno_t, void *, DBT *));
int __qam_fprobe __P((DBC *, db_pgno_t, void *, qam_probe_mode, DB_CACHE_PRIORITY, u_int32_t));
int __qam_fclose __P((DB *, db_pgno_t));
int __qam_fremove __P((DB *, db_pgno_t));
int __qam_sync __P((DB *));
int __qam_gen_filelist __P((DB *, DB_THREAD_INFO *, QUEUE_FILELIST **));
int __qam_extent_names __P((ENV *, char *, char ***));
void __qam_exid __P((DB *, u_int8_t *, u_int32_t));
int __qam_nameop __P((DB *, DB_TXN *, const char *, qam_name_op));
int __qam_lsn_reset __P((DB *, DB_THREAD_INFO *));
int __qam_db_create __P((DB *));
int __qam_db_close __P((DB *, u_int32_t));
int __qam_get_extentsize __P((DB *, u_int32_t *));
int __queue_pageinfo __P((DB *, db_pgno_t *, db_pgno_t *, int *, int, u_int32_t));
int __db_prqueue __P((DB *, u_int32_t));
int __qam_remove __P((DB *, DB_THREAD_INFO *, DB_TXN *, const char *, const char *, u_int32_t));
int __qam_rename __P((DB *, DB_THREAD_INFO *, DB_TXN *, const char *, const char *, const char *));
void __qam_map_flags __P((DB *, u_int32_t *, u_int32_t *));
int __qam_set_flags __P((DB *, u_int32_t *flagsp));
int __qam_open __P((DB *, DB_THREAD_INFO *, DB_TXN *, const char *, db_pgno_t, int, u_int32_t));
int __qam_set_ext_data __P((DB*, const char *));
int __qam_metachk __P((DB *, const char *, QMETA *));
int __qam_new_file __P((DB *, DB_THREAD_INFO *, DB_TXN *, DB_FH *, const char *));
int __qam_incfirst_recover __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_mvptr_recover __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_del_recover __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_delext_recover __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_add_recover __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_stat __P((DBC *, void *, u_int32_t));
int __qam_stat_print __P((DBC *, u_int32_t));
int __db_no_queue_am __P((ENV *));
int __qam_31_qammeta __P((DB *, char *, u_int8_t *));
int __qam_32_qammeta __P((DB *, char *, u_int8_t *));
int __qam_vrfy_meta __P((DB *, VRFY_DBINFO *, QMETA *, db_pgno_t, u_int32_t));
int __qam_meta2pgset __P((DB *, VRFY_DBINFO *, DB *));
int __qam_vrfy_data __P((DB *, VRFY_DBINFO *, QPAGE *, db_pgno_t, u_int32_t));
int __qam_vrfy_structure __P((DB *, VRFY_DBINFO *, u_int32_t));
int __qam_vrfy_walkqueue __P((DB *, VRFY_DBINFO *, void *, int (*)(void *, const void *), u_int32_t));
int __qam_salvage __P((DB *, VRFY_DBINFO *, db_pgno_t, PAGE *, void *, int (*)(void *, const void *), u_int32_t));

#if defined(__cplusplus)
}
#endif
#endif /* !_qam_ext_h_ */
