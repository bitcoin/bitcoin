/* Do not edit: automatically built by gen_msg.awk. */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_swap.h"

/*
 * PUBLIC: void __repmgr_handshake_marshal __P((ENV *,
 * PUBLIC:	 __repmgr_handshake_args *, u_int8_t *));
 */
void
__repmgr_handshake_marshal(env, argp, bp)
	ENV *env;
	__repmgr_handshake_args *argp;
	u_int8_t *bp;
{
	DB_HTONS_COPYOUT(env, bp, argp->port);
	DB_HTONL_COPYOUT(env, bp, argp->priority);
	DB_HTONL_COPYOUT(env, bp, argp->flags);
}

/*
 * PUBLIC: int __repmgr_handshake_unmarshal __P((ENV *,
 * PUBLIC:	 __repmgr_handshake_args *, u_int8_t *, size_t, u_int8_t **));
 */
int
__repmgr_handshake_unmarshal(env, argp, bp, max, nextp)
	ENV *env;
	__repmgr_handshake_args *argp;
	u_int8_t *bp;
	size_t max;
	u_int8_t **nextp;
{
	if (max < __REPMGR_HANDSHAKE_SIZE)
		goto too_few;
	DB_NTOHS_COPYIN(env, argp->port, bp);
	DB_NTOHL_COPYIN(env, argp->priority, bp);
	DB_NTOHL_COPYIN(env, argp->flags, bp);

	if (nextp != NULL)
		*nextp = bp;
	return (0);

too_few:
	__db_errx(env,
	    "Not enough input bytes to fill a __repmgr_handshake message");
	return (EINVAL);
}

/*
 * PUBLIC: void __repmgr_v2handshake_marshal __P((ENV *,
 * PUBLIC:	 __repmgr_v2handshake_args *, u_int8_t *));
 */
void
__repmgr_v2handshake_marshal(env, argp, bp)
	ENV *env;
	__repmgr_v2handshake_args *argp;
	u_int8_t *bp;
{
	DB_HTONS_COPYOUT(env, bp, argp->port);
	DB_HTONL_COPYOUT(env, bp, argp->priority);
}

/*
 * PUBLIC: int __repmgr_v2handshake_unmarshal __P((ENV *,
 * PUBLIC:	 __repmgr_v2handshake_args *, u_int8_t *, size_t, u_int8_t **));
 */
int
__repmgr_v2handshake_unmarshal(env, argp, bp, max, nextp)
	ENV *env;
	__repmgr_v2handshake_args *argp;
	u_int8_t *bp;
	size_t max;
	u_int8_t **nextp;
{
	if (max < __REPMGR_V2HANDSHAKE_SIZE)
		goto too_few;
	DB_NTOHS_COPYIN(env, argp->port, bp);
	DB_NTOHL_COPYIN(env, argp->priority, bp);

	if (nextp != NULL)
		*nextp = bp;
	return (0);

too_few:
	__db_errx(env,
	    "Not enough input bytes to fill a __repmgr_v2handshake message");
	return (EINVAL);
}

/*
 * PUBLIC: void __repmgr_ack_marshal __P((ENV *, __repmgr_ack_args *,
 * PUBLIC:	 u_int8_t *));
 */
void
__repmgr_ack_marshal(env, argp, bp)
	ENV *env;
	__repmgr_ack_args *argp;
	u_int8_t *bp;
{
	DB_HTONL_COPYOUT(env, bp, argp->generation);
	DB_HTONL_COPYOUT(env, bp, argp->lsn.file);
	DB_HTONL_COPYOUT(env, bp, argp->lsn.offset);
}

/*
 * PUBLIC: int __repmgr_ack_unmarshal __P((ENV *, __repmgr_ack_args *,
 * PUBLIC:	 u_int8_t *, size_t, u_int8_t **));
 */
int
__repmgr_ack_unmarshal(env, argp, bp, max, nextp)
	ENV *env;
	__repmgr_ack_args *argp;
	u_int8_t *bp;
	size_t max;
	u_int8_t **nextp;
{
	if (max < __REPMGR_ACK_SIZE)
		goto too_few;
	DB_NTOHL_COPYIN(env, argp->generation, bp);
	DB_NTOHL_COPYIN(env, argp->lsn.file, bp);
	DB_NTOHL_COPYIN(env, argp->lsn.offset, bp);

	if (nextp != NULL)
		*nextp = bp;
	return (0);

too_few:
	__db_errx(env,
	    "Not enough input bytes to fill a __repmgr_ack message");
	return (EINVAL);
}

/*
 * PUBLIC: void __repmgr_version_proposal_marshal __P((ENV *,
 * PUBLIC:	 __repmgr_version_proposal_args *, u_int8_t *));
 */
void
__repmgr_version_proposal_marshal(env, argp, bp)
	ENV *env;
	__repmgr_version_proposal_args *argp;
	u_int8_t *bp;
{
	DB_HTONL_COPYOUT(env, bp, argp->min);
	DB_HTONL_COPYOUT(env, bp, argp->max);
}

/*
 * PUBLIC: int __repmgr_version_proposal_unmarshal __P((ENV *,
 * PUBLIC:	 __repmgr_version_proposal_args *, u_int8_t *, size_t,
 * PUBLIC:	 u_int8_t **));
 */
int
__repmgr_version_proposal_unmarshal(env, argp, bp, max, nextp)
	ENV *env;
	__repmgr_version_proposal_args *argp;
	u_int8_t *bp;
	size_t max;
	u_int8_t **nextp;
{
	if (max < __REPMGR_VERSION_PROPOSAL_SIZE)
		goto too_few;
	DB_NTOHL_COPYIN(env, argp->min, bp);
	DB_NTOHL_COPYIN(env, argp->max, bp);

	if (nextp != NULL)
		*nextp = bp;
	return (0);

too_few:
	__db_errx(env,
	    "Not enough input bytes to fill a __repmgr_version_proposal message");
	return (EINVAL);
}

/*
 * PUBLIC: void __repmgr_version_confirmation_marshal __P((ENV *,
 * PUBLIC:	 __repmgr_version_confirmation_args *, u_int8_t *));
 */
void
__repmgr_version_confirmation_marshal(env, argp, bp)
	ENV *env;
	__repmgr_version_confirmation_args *argp;
	u_int8_t *bp;
{
	DB_HTONL_COPYOUT(env, bp, argp->version);
}

/*
 * PUBLIC: int __repmgr_version_confirmation_unmarshal __P((ENV *,
 * PUBLIC:	 __repmgr_version_confirmation_args *, u_int8_t *, size_t,
 * PUBLIC:	 u_int8_t **));
 */
int
__repmgr_version_confirmation_unmarshal(env, argp, bp, max, nextp)
	ENV *env;
	__repmgr_version_confirmation_args *argp;
	u_int8_t *bp;
	size_t max;
	u_int8_t **nextp;
{
	if (max < __REPMGR_VERSION_CONFIRMATION_SIZE)
		goto too_few;
	DB_NTOHL_COPYIN(env, argp->version, bp);

	if (nextp != NULL)
		*nextp = bp;
	return (0);

too_few:
	__db_errx(env,
	    "Not enough input bytes to fill a __repmgr_version_confirmation message");
	return (EINVAL);
}

