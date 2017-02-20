/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef _tcl_ext_h_
#define _tcl_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

int bdb_HCommand __P((Tcl_Interp *, int, Tcl_Obj * CONST*));
#if DB_DBM_HSEARCH != 0
int bdb_NdbmOpen __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DBM **));
#endif
#if DB_DBM_HSEARCH != 0
int bdb_DbmCommand __P((Tcl_Interp *, int, Tcl_Obj * CONST*, int, DBM *));
#endif
int ndbm_Cmd __P((ClientData, Tcl_Interp *, int, Tcl_Obj * CONST*));
void _DbInfoDelete __P((Tcl_Interp *, DBTCL_INFO *));
int db_Cmd __P((ClientData, Tcl_Interp *, int, Tcl_Obj * CONST*));
int tcl_CompactStat __P((Tcl_Interp *, DBTCL_INFO *));
int tcl_rep_send __P((DB_ENV *, const DBT *, const DBT *, const DB_LSN *, int, u_int32_t));
int dbc_Cmd __P((ClientData, Tcl_Interp *, int, Tcl_Obj * CONST*));
int env_Cmd __P((ClientData, Tcl_Interp *, int, Tcl_Obj * CONST*));
int tcl_EnvRemove __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *, DBTCL_INFO *));
int tcl_EnvIdReset __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
int tcl_EnvLsnReset __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
int tcl_EnvVerbose __P((Tcl_Interp *, DB_ENV *, Tcl_Obj *, Tcl_Obj *));
int tcl_EnvAttr __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
int tcl_EventNotify  __P((Tcl_Interp *, DB_ENV *, Tcl_Obj *, DBTCL_INFO *));
int tcl_EnvSetFlags __P((Tcl_Interp *, DB_ENV *, Tcl_Obj *, Tcl_Obj *));
int tcl_EnvTest __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
int tcl_EnvGetEncryptFlags __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
void tcl_EnvSetErrfile __P((Tcl_Interp *, DB_ENV *, DBTCL_INFO *, char *));
int tcl_EnvSetErrpfx __P((Tcl_Interp *, DB_ENV *, DBTCL_INFO *, char *));
DBTCL_INFO *_NewInfo __P((Tcl_Interp *, void *, char *, enum INFOTYPE));
void *_NameToPtr __P((CONST char *));
DBTCL_INFO *_PtrToInfo __P((CONST void *));
DBTCL_INFO *_NameToInfo __P((CONST char *));
void  _SetInfoData __P((DBTCL_INFO *, void *));
void  _DeleteInfo __P((DBTCL_INFO *));
int _SetListElem __P((Tcl_Interp *, Tcl_Obj *, void *, u_int32_t, void *, u_int32_t));
int _SetListElemInt __P((Tcl_Interp *, Tcl_Obj *, void *, long));
int _SetListElemWideInt __P((Tcl_Interp *, Tcl_Obj *, void *, int64_t));
int _SetListRecnoElem __P((Tcl_Interp *, Tcl_Obj *, db_recno_t, u_char *, u_int32_t));
int _Set3DBTList __P((Tcl_Interp *, Tcl_Obj *, DBT *, int, DBT *, int, DBT *));
int _SetMultiList __P((Tcl_Interp *, Tcl_Obj *, DBT *, DBT*, DBTYPE, u_int32_t));
int _GetGlobPrefix __P((char *, char **));
int _ReturnSetup __P((Tcl_Interp *, int, int, char *));
int _ErrorSetup __P((Tcl_Interp *, int, char *));
void _ErrorFunc __P((const DB_ENV *, CONST char *, const char *));
void _EventFunc __P((DB_ENV *, u_int32_t, void *));
int _GetLsn __P((Tcl_Interp *, Tcl_Obj *, DB_LSN *));
int _GetUInt32 __P((Tcl_Interp *, Tcl_Obj *, u_int32_t *));
Tcl_Obj *_GetFlagsList __P((Tcl_Interp *, u_int32_t, const FN *));
void _debug_check  __P((void));
int _CopyObjBytes  __P((Tcl_Interp *, Tcl_Obj *obj, void *, u_int32_t *, int *));
int tcl_LockDetect __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
int tcl_LockGet __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
int tcl_LockStat __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
int tcl_LockTimeout __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
int tcl_LockVec __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
int tcl_LogArchive __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
int tcl_LogCompare __P((Tcl_Interp *, int, Tcl_Obj * CONST*));
int tcl_LogFile __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
int tcl_LogFlush __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
int tcl_LogGet __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
int tcl_LogPut __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
int tcl_LogStat __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
int logc_Cmd __P((ClientData, Tcl_Interp *, int, Tcl_Obj * CONST*));
int tcl_LogConfig __P((Tcl_Interp *, DB_ENV *, Tcl_Obj *));
int tcl_LogGetConfig __P((Tcl_Interp *, DB_ENV *, Tcl_Obj *));
void _MpInfoDelete __P((Tcl_Interp *, DBTCL_INFO *));
int tcl_MpSync __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
int tcl_MpTrickle __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
int tcl_Mp __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *, DBTCL_INFO *));
int tcl_MpStat __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
int tcl_Mutex __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
int tcl_MutFree __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
int tcl_MutGet __P((Tcl_Interp *, DB_ENV *, int));
int tcl_MutLock __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
int tcl_MutSet __P((Tcl_Interp *, Tcl_Obj *, DB_ENV *, int));
int tcl_MutStat __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
int tcl_MutUnlock __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
int tcl_RepConfig __P((Tcl_Interp *, DB_ENV *, Tcl_Obj *));
int tcl_RepGetTwo __P((Tcl_Interp *, DB_ENV *, int));
int tcl_RepGetConfig __P((Tcl_Interp *, DB_ENV *, Tcl_Obj *));
int tcl_RepGetTimeout __P((Tcl_Interp *, DB_ENV *, Tcl_Obj *));
int tcl_RepElect __P((Tcl_Interp *, int, Tcl_Obj * CONST *, DB_ENV *));
int tcl_RepFlush __P((Tcl_Interp *, int, Tcl_Obj * CONST *, DB_ENV *));
int tcl_RepSync __P((Tcl_Interp *, int, Tcl_Obj * CONST *, DB_ENV *));
int tcl_RepLease  __P((Tcl_Interp *, int, Tcl_Obj * CONST *, DB_ENV *));
int tcl_RepInmemFiles  __P((Tcl_Interp *, DB_ENV *));
int tcl_RepLimit __P((Tcl_Interp *, int, Tcl_Obj * CONST *, DB_ENV *));
int tcl_RepRequest __P((Tcl_Interp *, int, Tcl_Obj * CONST *, DB_ENV *));
int tcl_RepNoarchiveTimeout __P((Tcl_Interp *, DB_ENV *));
int tcl_RepTransport  __P((Tcl_Interp *, int, Tcl_Obj * CONST *, DB_ENV *, DBTCL_INFO *));
int tcl_RepStart __P((Tcl_Interp *, int, Tcl_Obj * CONST *, DB_ENV *));
int tcl_RepProcessMessage __P((Tcl_Interp *, int, Tcl_Obj * CONST *, DB_ENV *));
int tcl_RepStat __P((Tcl_Interp *, int, Tcl_Obj * CONST *, DB_ENV *));
int tcl_RepMgr __P((Tcl_Interp *, int, Tcl_Obj * CONST *, DB_ENV *));
int tcl_RepMgrSiteList __P((Tcl_Interp *, int, Tcl_Obj * CONST *, DB_ENV *));
int tcl_RepMgrStat __P((Tcl_Interp *, int, Tcl_Obj * CONST *, DB_ENV *));
int seq_Cmd __P((ClientData, Tcl_Interp *, int, Tcl_Obj * CONST*));
void _TxnInfoDelete __P((Tcl_Interp *, DBTCL_INFO *));
int tcl_TxnCheckpoint __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
int tcl_Txn __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *, DBTCL_INFO *));
int tcl_CDSGroup __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *, DBTCL_INFO *));
int tcl_TxnStat __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
int tcl_TxnTimeout __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
int tcl_TxnRecover __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *, DBTCL_INFO *));
int bdb_RandCommand __P((Tcl_Interp *, int, Tcl_Obj * CONST*));

#if defined(__cplusplus)
}
#endif
#endif /* !_tcl_ext_h_ */
