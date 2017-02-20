/* Do not edit: automatically built by gen_rec.awk. */

#ifndef __dbreg_AUTO_H
#define __dbreg_AUTO_H
#define DB___dbreg_register 2
typedef struct ___dbreg_register_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    u_int32_t   opcode;
    DBT name;
    DBT uid;
    int32_t fileid;
    DBTYPE  ftype;
    db_pgno_t   meta_pgno;
    u_int32_t   id;
} __dbreg_register_args;

#endif
