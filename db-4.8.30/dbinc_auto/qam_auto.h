/* Do not edit: automatically built by gen_rec.awk. */

#ifndef __qam_AUTO_H
#define __qam_AUTO_H
#define DB___qam_incfirst   84
typedef struct ___qam_incfirst_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    int32_t fileid;
    db_recno_t  recno;
    db_pgno_t   meta_pgno;
} __qam_incfirst_args;

#define DB___qam_mvptr  85
typedef struct ___qam_mvptr_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    u_int32_t   opcode;
    int32_t fileid;
    db_recno_t  old_first;
    db_recno_t  new_first;
    db_recno_t  old_cur;
    db_recno_t  new_cur;
    DB_LSN  metalsn;
    db_pgno_t   meta_pgno;
} __qam_mvptr_args;

#define DB___qam_del    79
typedef struct ___qam_del_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    int32_t fileid;
    DB_LSN  lsn;
    db_pgno_t   pgno;
    u_int32_t   indx;
    db_recno_t  recno;
} __qam_del_args;

#define DB___qam_add    80
typedef struct ___qam_add_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    int32_t fileid;
    DB_LSN  lsn;
    db_pgno_t   pgno;
    u_int32_t   indx;
    db_recno_t  recno;
    DBT data;
    u_int32_t   vflag;
    DBT olddata;
} __qam_add_args;

#define DB___qam_delext 83
typedef struct ___qam_delext_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    int32_t fileid;
    DB_LSN  lsn;
    db_pgno_t   pgno;
    u_int32_t   indx;
    db_recno_t  recno;
    DBT data;
} __qam_delext_args;

#endif
