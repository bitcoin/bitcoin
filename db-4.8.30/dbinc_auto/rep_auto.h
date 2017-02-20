/* Do not edit: automatically built by gen_msg.awk. */

#ifndef __rep_AUTO_H
#define __rep_AUTO_H

/*
 * Message sizes are simply the sum of field sizes (not
 * counting variable size parts, when DBTs are present),
 * and may be different from struct sizes due to padding.
 */
#define __REP_BULK_SIZE 16
typedef struct ___rep_bulk_args {
    u_int32_t   len;
    DB_LSN      lsn;
    DBT     bulkdata;
} __rep_bulk_args;

#define __REP_CONTROL_SIZE  36
typedef struct ___rep_control_args {
    u_int32_t   rep_version;
    u_int32_t   log_version;
    DB_LSN      lsn;
    u_int32_t   rectype;
    u_int32_t   gen;
    u_int32_t   msg_sec;
    u_int32_t   msg_nsec;
    u_int32_t   flags;
} __rep_control_args;

#define __REP_EGEN_SIZE 4
typedef struct ___rep_egen_args {
    u_int32_t   egen;
} __rep_egen_args;

#define __REP_FILEINFO_SIZE 36
typedef struct ___rep_fileinfo_args {
    u_int32_t   pgsize;
    db_pgno_t   pgno;
    db_pgno_t   max_pgno;
    u_int32_t   filenum;
    u_int32_t   finfo_flags;
    u_int32_t   type;
    u_int32_t   db_flags;
    DBT     uid;
    DBT     info;
} __rep_fileinfo_args;

#define __REP_GRANT_INFO_SIZE   8
typedef struct ___rep_grant_info_args {
    u_int32_t   msg_sec;
    u_int32_t   msg_nsec;
} __rep_grant_info_args;

#define __REP_LOGREQ_SIZE   8
typedef struct ___rep_logreq_args {
    DB_LSN      endlsn;
} __rep_logreq_args;

#define __REP_NEWFILE_SIZE  4
typedef struct ___rep_newfile_args {
    u_int32_t   version;
} __rep_newfile_args;

#define __REP_UPDATE_SIZE   16
typedef struct ___rep_update_args {
    DB_LSN      first_lsn;
    u_int32_t   first_vers;
    u_int32_t   num_files;
} __rep_update_args;

#define __REP_VOTE_INFO_SIZE    20
typedef struct ___rep_vote_info_args {
    u_int32_t   egen;
    u_int32_t   nsites;
    u_int32_t   nvotes;
    u_int32_t   priority;
    u_int32_t   tiebreaker;
} __rep_vote_info_args;

#define __REP_MAXMSG_SIZE   36
#endif
