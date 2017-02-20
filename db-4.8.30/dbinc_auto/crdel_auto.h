/* Do not edit: automatically built by gen_rec.awk. */

#ifndef __crdel_AUTO_H
#define __crdel_AUTO_H
#define DB___crdel_metasub  142
typedef struct ___crdel_metasub_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    int32_t fileid;
    db_pgno_t   pgno;
    DBT page;
    DB_LSN  lsn;
} __crdel_metasub_args;

#define DB___crdel_inmem_create 138
typedef struct ___crdel_inmem_create_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    int32_t fileid;
    DBT name;
    DBT fid;
    u_int32_t   pgsize;
} __crdel_inmem_create_args;

#define DB___crdel_inmem_rename 139
typedef struct ___crdel_inmem_rename_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    DBT oldname;
    DBT newname;
    DBT fid;
} __crdel_inmem_rename_args;

#define DB___crdel_inmem_remove 140
typedef struct ___crdel_inmem_remove_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    DBT name;
    DBT fid;
} __crdel_inmem_remove_args;

#endif
