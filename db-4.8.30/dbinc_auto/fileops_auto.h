/* Do not edit: automatically built by gen_rec.awk. */

#ifndef __fop_AUTO_H
#define __fop_AUTO_H
#define DB___fop_create_42  143
typedef struct ___fop_create_42_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    DBT name;
    u_int32_t   appname;
    u_int32_t   mode;
} __fop_create_42_args;

#define DB___fop_create 143
typedef struct ___fop_create_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    DBT name;
    DBT dirname;
    u_int32_t   appname;
    u_int32_t   mode;
} __fop_create_args;

#define DB___fop_remove 144
typedef struct ___fop_remove_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    DBT name;
    DBT fid;
    u_int32_t   appname;
} __fop_remove_args;

#define DB___fop_write_42   145
typedef struct ___fop_write_42_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    DBT name;
    u_int32_t   appname;
    u_int32_t   pgsize;
    db_pgno_t   pageno;
    u_int32_t   offset;
    DBT page;
    u_int32_t   flag;
} __fop_write_42_args;

#define DB___fop_write  145
typedef struct ___fop_write_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    DBT name;
    DBT dirname;
    u_int32_t   appname;
    u_int32_t   pgsize;
    db_pgno_t   pageno;
    u_int32_t   offset;
    DBT page;
    u_int32_t   flag;
} __fop_write_args;

#define DB___fop_rename_42  146
#define DB___fop_rename_noundo_46   150
typedef struct ___fop_rename_42_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    DBT oldname;
    DBT newname;
    DBT fileid;
    u_int32_t   appname;
} __fop_rename_42_args;

#define DB___fop_rename 146
#define DB___fop_rename_noundo  150
typedef struct ___fop_rename_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    DBT oldname;
    DBT newname;
    DBT dirname;
    DBT fileid;
    u_int32_t   appname;
} __fop_rename_args;

#define DB___fop_file_remove    141
typedef struct ___fop_file_remove_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    DBT real_fid;
    DBT tmp_fid;
    DBT name;
    u_int32_t   appname;
    u_int32_t   child;
} __fop_file_remove_args;

#endif
