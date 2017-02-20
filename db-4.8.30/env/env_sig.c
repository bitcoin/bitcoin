/*-
 * DO NOT EDIT: automatically built by dist/s_sig.
 *
 * $Id$
 */

#include "db_config.h"

#define	__INCLUDE_NETWORKING	1
#define __INCLUDE_SELECT_H      1
#include "db_int.h"

#include "dbinc/db_page.h"
#include "dbinc/btree.h"
#include "dbinc/crypto.h"
#include "dbinc/db_join.h"
#include "dbinc/db_verify.h"
#include "dbinc/hash.h"
#include "dbinc/lock.h"
#include "dbinc/log.h"
#include "dbinc/mp.h"
#include "dbinc/partition.h"
#include "dbinc/qam.h"
#include "dbinc/txn.h"

#define	__STRUCTURE_COUNT	114

/*
 * __env_struct_sig --
 *	Compute signature of structures.
 *
 * PUBLIC: u_int32_t __env_struct_sig __P((void));
 */
u_int32_t
__env_struct_sig()
{
	u_short t[__STRUCTURE_COUNT + 5];
	u_int i;

	i = 0;
#define	__ADD(s)	(t[i++] = sizeof(struct s))

	__ADD(__db_dbt);
#ifdef	HAVE_MUTEX_SUPPORT
	__ADD(__db_mutex_stat);
#endif
	__ADD(__db_lock_stat);
	__ADD(__db_lock_hstat);
	__ADD(__db_lock_pstat);
	__ADD(__db_ilock);
	__ADD(__db_lock_u);
	__ADD(__db_lockreq);
	__ADD(__db_lsn);
	__ADD(__db_log_cursor);
	__ADD(__db_log_stat);
	__ADD(__db_mpoolfile);
	__ADD(__db_mpool_stat);
	__ADD(__db_mpool_fstat);
	__ADD(__db_txn);
	__ADD(__kids);
	__ADD(__db_preplist);
	__ADD(__db_txn_active);
	__ADD(__db_txn_stat);
	__ADD(__db_repmgr_site);
	__ADD(__db_rep_stat);
	__ADD(__db_repmgr_stat);
	__ADD(__db_seq_record);
	__ADD(__db_sequence);
	__ADD(__db_seq_stat);
	__ADD(__db);
	__ADD(__cq_fq);
	__ADD(__cq_aq);
	__ADD(__cq_jq);
	__ADD(__dbc);
	__ADD(__key_range);
	__ADD(__db_bt_stat);
	__ADD(__db_compact);
	__ADD(__db_h_stat);
	__ADD(__db_qam_stat);
	__ADD(__db_env);
	__ADD(__db_distab);
	__ADD(__fn);
	__ADD(__db_msgbuf);
	__ADD(__pin_list);
	__ADD(__db_thread_info);
	__ADD(__env_thread_info);
	__ADD(__flag_map);
	__ADD(__env);
	__ADD(__dbc_internal);
	__ADD(__dbpginfo);
	__ADD(__epg);
	__ADD(__cursor);
	__ADD(__btree);
	__ADD(__db_cipher);
	__ADD(__db_foreign_info);
	__ADD(__db_txnhead);
	__ADD(__db_txnlist);
	__ADD(__join_cursor);
	__ADD(__pg_chksum);
	__ADD(__pg_crypto);
	__ADD(__pglist);
	__ADD(__vrfy_dbinfo);
	__ADD(__vrfy_pageinfo);
	__ADD(__vrfy_childinfo);
	__ADD(__db_globals);
	__ADD(__db_lockregion);
	__ADD(__sh_dbt);
	__ADD(__db_lockobj);
	__ADD(__db_locker);
	__ADD(__db_lockpart);
	__ADD(__db_locktab);
	__ADD(__db_lock);
	__ADD(__db_entry);
	__ADD(__fname);
	__ADD(__db_log);
	__ADD(__hdr);
	__ADD(__log_persist);
	__ADD(__log);
	__ADD(__db_commit);
	__ADD(__db_filestart);
	__ADD(__db_mpool);
	__ADD(__db_mpreg);
	__ADD(__mpool);
	__ADD(__db_mpool_hash);
	__ADD(__mpoolfile);
	__ADD(__bh);
	__ADD(__bh_frozen_p);
	__ADD(__bh_frozen_a);
#ifdef	HAVE_MUTEX_SUPPORT
	__ADD(__db_mutexmgr);
#endif
#ifdef	HAVE_MUTEX_SUPPORT
	__ADD(__db_mutexregion);
#endif
#ifdef	HAVE_MUTEX_SUPPORT
	__ADD(__db_mutex_t);
#endif
	__ADD(__fh_t);
	__ADD(__db_partition);
	__ADD(__part_internal);
	__ADD(__qcursor);
	__ADD(__mpfarray);
	__ADD(__qmpf);
	__ADD(__queue);
	__ADD(__qam_filelist);
	__ADD(__db_reg_env_ref);
	__ADD(__db_reg_env);
	__ADD(__db_region);
	__ADD(__db_reginfo_t);
	__ADD(__rep);
	__ADD(__db_rep);
	__ADD(__rep_lease_entry);
	__ADD(__txn_detail);
	__ADD(__db_txnmgr);
	__ADD(__db_txnregion);
	__ADD(__txn_logrec);

	return (__ham_func5(NULL, t, i * sizeof(t[0])));
}
