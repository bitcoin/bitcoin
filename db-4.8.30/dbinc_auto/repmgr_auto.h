/* Do not edit: automatically built by gen_msg.awk. */

#ifndef __repmgr_AUTO_H
#define __repmgr_AUTO_H

/*
 * Message sizes are simply the sum of field sizes (not
 * counting variable size parts, when DBTs are present),
 * and may be different from struct sizes due to padding.
 */
#define __REPMGR_HANDSHAKE_SIZE 10
typedef struct ___repmgr_handshake_args {
    u_int16_t   port;
    u_int32_t   priority;
    u_int32_t   flags;
} __repmgr_handshake_args;

#define __REPMGR_V2HANDSHAKE_SIZE   6
typedef struct ___repmgr_v2handshake_args {
    u_int16_t   port;
    u_int32_t   priority;
} __repmgr_v2handshake_args;

#define __REPMGR_ACK_SIZE   12
typedef struct ___repmgr_ack_args {
    u_int32_t   generation;
    DB_LSN      lsn;
} __repmgr_ack_args;

#define __REPMGR_VERSION_PROPOSAL_SIZE  8
typedef struct ___repmgr_version_proposal_args {
    u_int32_t   min;
    u_int32_t   max;
} __repmgr_version_proposal_args;

#define __REPMGR_VERSION_CONFIRMATION_SIZE  4
typedef struct ___repmgr_version_confirmation_args {
    u_int32_t   version;
} __repmgr_version_confirmation_args;

#define __REPMGR_MAXMSG_SIZE    12
#endif
