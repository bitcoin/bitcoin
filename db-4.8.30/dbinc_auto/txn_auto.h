/* Do not edit: automatically built by gen_rec.awk. */

#ifndef __txn_AUTO_H
#define __txn_AUTO_H
#define DB___txn_regop_42   10
typedef struct ___txn_regop_42_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    u_int32_t   opcode;
    int32_t timestamp;
    DBT locks;
} __txn_regop_42_args;

#define DB___txn_regop  10
typedef struct ___txn_regop_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    u_int32_t   opcode;
    int32_t timestamp;
    u_int32_t   envid;
    DBT locks;
} __txn_regop_args;

#define DB___txn_ckp_42 11
typedef struct ___txn_ckp_42_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    DB_LSN  ckp_lsn;
    DB_LSN  last_ckp;
    int32_t timestamp;
    u_int32_t   rep_gen;
} __txn_ckp_42_args;

#define DB___txn_ckp    11
typedef struct ___txn_ckp_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    DB_LSN  ckp_lsn;
    DB_LSN  last_ckp;
    int32_t timestamp;
    u_int32_t   envid;
    u_int32_t   spare;
} __txn_ckp_args;

#define DB___txn_child  12
typedef struct ___txn_child_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    u_int32_t   child;
    DB_LSN  c_lsn;
} __txn_child_args;

#define DB___txn_xa_regop_42    13
typedef struct ___txn_xa_regop_42_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    u_int32_t   opcode;
    DBT xid;
    int32_t formatID;
    u_int32_t   gtrid;
    u_int32_t   bqual;
    DB_LSN  begin_lsn;
    DBT locks;
} __txn_xa_regop_42_args;

#define DB___txn_prepare    13
typedef struct ___txn_prepare_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    u_int32_t   opcode;
    DBT gid;
    DB_LSN  begin_lsn;
    DBT locks;
} __txn_prepare_args;

#define DB___txn_recycle    14
typedef struct ___txn_recycle_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    u_int32_t   min;
    u_int32_t   max;
} __txn_recycle_args;

#endif
