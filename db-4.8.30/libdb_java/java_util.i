%pragma(java) jniclasscode=%{
	static {
		/* An alternate library name can be specified via a property. */
		String libname;
		int v_major, v_minor, v_patch;

		v_major = DbConstants.DB_VERSION_MAJOR;
		v_minor = DbConstants.DB_VERSION_MINOR;
		v_patch = DbConstants.DB_VERSION_PATCH;

		if ((libname =
		    System.getProperty("sleepycat.db.libfile")) != null)
			System.load(libname);
		else if ((libname =
		    System.getProperty("sleepycat.db.libname")) != null)
			System.loadLibrary(libname);
		else {
			String os = System.getProperty("os.name");
			if (os != null && os.startsWith("Windows")) {
				/*
				 * On Windows, library name is something like
				 * "libdb_java42.dll" or "libdb_java42d.dll".
				 */
				libname = "libdb_java" + v_major + v_minor;

				try {
					System.loadLibrary(libname);
				} catch (UnsatisfiedLinkError e) {
					try {
						libname += "d";
						System.loadLibrary(libname);
					} catch (UnsatisfiedLinkError e2) {
						throw e;
					}
				}
			} else {
				/*
				 * On UNIX, library name is something like
				 * "libdb_java-3.0.so".
				 */
				System.loadLibrary("db_java-" +
				    v_major + "." + v_minor);
			}
		}

		initialize();

		if (DbEnv_get_version_major() != v_major ||
		    DbEnv_get_version_minor() != v_minor ||
		    DbEnv_get_version_patch() != v_patch)
			throw new RuntimeException(
		      "Berkeley DB library version " + 
		      DbEnv_get_version_major() + "." +
		      DbEnv_get_version_minor() + "." + 
		      DbEnv_get_version_patch() +
		      " doesn't match Java class library version " + 
		      v_major + "." + v_minor + "." + v_patch);
	}

	static native final void initialize();
%}

%{
/* don't use SWIG's array handling - save code space */
#define	SWIG_NOINCLUDE 1

#define	DB_ENV_INTERNAL(dbenv) ((dbenv)->api2_internal)
#define	DB_INTERNAL(db) ((db)->api_internal)

#define	DB_PKG "com/sleepycat/db/"

/* Forward declarations */
static int __dbj_throw(JNIEnv *jenv,
    int err, const char *msg, jobject obj, jobject jdbenv);

/* Global data - JVM handle, classes, fields and methods */
static JavaVM *javavm;

static jclass db_class, dbc_class, dbenv_class, dbt_class, dblsn_class;
static jclass dbpreplist_class, dbtxn_class;
static jclass keyrange_class;
static jclass bt_stat_class, compact_class, h_stat_class, lock_stat_class;
static jclass log_stat_class, mpool_stat_class, mpool_fstat_class;
static jclass mutex_stat_class, qam_stat_class, rep_stat_class;
static jclass repmgr_stat_class, repmgr_siteinfo_class, rephost_class;
static jclass seq_stat_class, txn_stat_class;
static jclass txn_active_class;
static jclass lock_class, lockreq_class;
static jclass dbex_class, deadex_class, lockex_class, memex_class;
static jclass repdupmasterex_class, rephandledeadex_class;
static jclass repholdelectionex_class, repjoinfailex_class;
static jclass repleaseexpiredex_class, repleasetimeoutex_class;
static jclass replockoutex_class, repunavailex_class;
static jclass runrecex_class, versionex_class;
static jclass filenotfoundex_class, illegalargex_class, outofmemerr_class;
static jclass bytearray_class, string_class, outputstream_class;

static jfieldID dbc_cptr_fid;
static jfieldID dblsn_file_fid, dblsn_offset_fid;
static jfieldID dbt_data_fid, dbt_data_nio_fid, dbt_size_fid, dbt_ulen_fid;
static jfieldID dbt_dlen_fid, dbt_doff_fid, dbt_flags_fid, dbt_offset_fid;
static jfieldID kr_less_fid, kr_equal_fid, kr_greater_fid;
static jfieldID lock_cptr_fid;
static jfieldID lockreq_op_fid, lockreq_modeflag_fid, lockreq_timeout_fid;
static jfieldID lockreq_obj_fid, lockreq_lock_fid;
static jfieldID repmgr_siteinfo_status_fid;

/* BEGIN-STAT-FIELD-DECLS */
static jfieldID bt_stat_bt_magic_fid;
static jfieldID bt_stat_bt_version_fid;
static jfieldID bt_stat_bt_metaflags_fid;
static jfieldID bt_stat_bt_nkeys_fid;
static jfieldID bt_stat_bt_ndata_fid;
static jfieldID bt_stat_bt_pagecnt_fid;
static jfieldID bt_stat_bt_pagesize_fid;
static jfieldID bt_stat_bt_minkey_fid;
static jfieldID bt_stat_bt_re_len_fid;
static jfieldID bt_stat_bt_re_pad_fid;
static jfieldID bt_stat_bt_levels_fid;
static jfieldID bt_stat_bt_int_pg_fid;
static jfieldID bt_stat_bt_leaf_pg_fid;
static jfieldID bt_stat_bt_dup_pg_fid;
static jfieldID bt_stat_bt_over_pg_fid;
static jfieldID bt_stat_bt_empty_pg_fid;
static jfieldID bt_stat_bt_free_fid;
static jfieldID bt_stat_bt_int_pgfree_fid;
static jfieldID bt_stat_bt_leaf_pgfree_fid;
static jfieldID bt_stat_bt_dup_pgfree_fid;
static jfieldID bt_stat_bt_over_pgfree_fid;
static jfieldID compact_compact_fillpercent_fid;
static jfieldID compact_compact_timeout_fid;
static jfieldID compact_compact_pages_fid;
static jfieldID compact_compact_pages_free_fid;
static jfieldID compact_compact_pages_examine_fid;
static jfieldID compact_compact_levels_fid;
static jfieldID compact_compact_deadlock_fid;
static jfieldID compact_compact_pages_truncated_fid;
static jfieldID compact_compact_truncate_fid;
static jfieldID h_stat_hash_magic_fid;
static jfieldID h_stat_hash_version_fid;
static jfieldID h_stat_hash_metaflags_fid;
static jfieldID h_stat_hash_nkeys_fid;
static jfieldID h_stat_hash_ndata_fid;
static jfieldID h_stat_hash_pagecnt_fid;
static jfieldID h_stat_hash_pagesize_fid;
static jfieldID h_stat_hash_ffactor_fid;
static jfieldID h_stat_hash_buckets_fid;
static jfieldID h_stat_hash_free_fid;
static jfieldID h_stat_hash_bfree_fid;
static jfieldID h_stat_hash_bigpages_fid;
static jfieldID h_stat_hash_big_bfree_fid;
static jfieldID h_stat_hash_overflows_fid;
static jfieldID h_stat_hash_ovfl_free_fid;
static jfieldID h_stat_hash_dup_fid;
static jfieldID h_stat_hash_dup_free_fid;
static jfieldID lock_stat_st_id_fid;
static jfieldID lock_stat_st_cur_maxid_fid;
static jfieldID lock_stat_st_maxlocks_fid;
static jfieldID lock_stat_st_maxlockers_fid;
static jfieldID lock_stat_st_maxobjects_fid;
static jfieldID lock_stat_st_partitions_fid;
static jfieldID lock_stat_st_nmodes_fid;
static jfieldID lock_stat_st_nlockers_fid;
static jfieldID lock_stat_st_nlocks_fid;
static jfieldID lock_stat_st_maxnlocks_fid;
static jfieldID lock_stat_st_maxhlocks_fid;
static jfieldID lock_stat_st_locksteals_fid;
static jfieldID lock_stat_st_maxlsteals_fid;
static jfieldID lock_stat_st_maxnlockers_fid;
static jfieldID lock_stat_st_nobjects_fid;
static jfieldID lock_stat_st_maxnobjects_fid;
static jfieldID lock_stat_st_maxhobjects_fid;
static jfieldID lock_stat_st_objectsteals_fid;
static jfieldID lock_stat_st_maxosteals_fid;
static jfieldID lock_stat_st_nrequests_fid;
static jfieldID lock_stat_st_nreleases_fid;
static jfieldID lock_stat_st_nupgrade_fid;
static jfieldID lock_stat_st_ndowngrade_fid;
static jfieldID lock_stat_st_lock_wait_fid;
static jfieldID lock_stat_st_lock_nowait_fid;
static jfieldID lock_stat_st_ndeadlocks_fid;
static jfieldID lock_stat_st_locktimeout_fid;
static jfieldID lock_stat_st_nlocktimeouts_fid;
static jfieldID lock_stat_st_txntimeout_fid;
static jfieldID lock_stat_st_ntxntimeouts_fid;
static jfieldID lock_stat_st_part_wait_fid;
static jfieldID lock_stat_st_part_nowait_fid;
static jfieldID lock_stat_st_part_max_wait_fid;
static jfieldID lock_stat_st_part_max_nowait_fid;
static jfieldID lock_stat_st_objs_wait_fid;
static jfieldID lock_stat_st_objs_nowait_fid;
static jfieldID lock_stat_st_lockers_wait_fid;
static jfieldID lock_stat_st_lockers_nowait_fid;
static jfieldID lock_stat_st_region_wait_fid;
static jfieldID lock_stat_st_region_nowait_fid;
static jfieldID lock_stat_st_hash_len_fid;
static jfieldID lock_stat_st_regsize_fid;
static jfieldID log_stat_st_magic_fid;
static jfieldID log_stat_st_version_fid;
static jfieldID log_stat_st_mode_fid;
static jfieldID log_stat_st_lg_bsize_fid;
static jfieldID log_stat_st_lg_size_fid;
static jfieldID log_stat_st_wc_bytes_fid;
static jfieldID log_stat_st_wc_mbytes_fid;
static jfieldID log_stat_st_record_fid;
static jfieldID log_stat_st_w_bytes_fid;
static jfieldID log_stat_st_w_mbytes_fid;
static jfieldID log_stat_st_wcount_fid;
static jfieldID log_stat_st_wcount_fill_fid;
static jfieldID log_stat_st_rcount_fid;
static jfieldID log_stat_st_scount_fid;
static jfieldID log_stat_st_region_wait_fid;
static jfieldID log_stat_st_region_nowait_fid;
static jfieldID log_stat_st_cur_file_fid;
static jfieldID log_stat_st_cur_offset_fid;
static jfieldID log_stat_st_disk_file_fid;
static jfieldID log_stat_st_disk_offset_fid;
static jfieldID log_stat_st_maxcommitperflush_fid;
static jfieldID log_stat_st_mincommitperflush_fid;
static jfieldID log_stat_st_regsize_fid;
static jfieldID mpool_fstat_file_name_fid;
static jfieldID mpool_fstat_st_pagesize_fid;
static jfieldID mpool_fstat_st_map_fid;
static jfieldID mpool_fstat_st_cache_hit_fid;
static jfieldID mpool_fstat_st_cache_miss_fid;
static jfieldID mpool_fstat_st_page_create_fid;
static jfieldID mpool_fstat_st_page_in_fid;
static jfieldID mpool_fstat_st_page_out_fid;
static jfieldID mpool_stat_st_gbytes_fid;
static jfieldID mpool_stat_st_bytes_fid;
static jfieldID mpool_stat_st_ncache_fid;
static jfieldID mpool_stat_st_max_ncache_fid;
static jfieldID mpool_stat_st_mmapsize_fid;
static jfieldID mpool_stat_st_maxopenfd_fid;
static jfieldID mpool_stat_st_maxwrite_fid;
static jfieldID mpool_stat_st_maxwrite_sleep_fid;
static jfieldID mpool_stat_st_pages_fid;
static jfieldID mpool_stat_st_map_fid;
static jfieldID mpool_stat_st_cache_hit_fid;
static jfieldID mpool_stat_st_cache_miss_fid;
static jfieldID mpool_stat_st_page_create_fid;
static jfieldID mpool_stat_st_page_in_fid;
static jfieldID mpool_stat_st_page_out_fid;
static jfieldID mpool_stat_st_ro_evict_fid;
static jfieldID mpool_stat_st_rw_evict_fid;
static jfieldID mpool_stat_st_page_trickle_fid;
static jfieldID mpool_stat_st_page_clean_fid;
static jfieldID mpool_stat_st_page_dirty_fid;
static jfieldID mpool_stat_st_hash_buckets_fid;
static jfieldID mpool_stat_st_pagesize_fid;
static jfieldID mpool_stat_st_hash_searches_fid;
static jfieldID mpool_stat_st_hash_longest_fid;
static jfieldID mpool_stat_st_hash_examined_fid;
static jfieldID mpool_stat_st_hash_nowait_fid;
static jfieldID mpool_stat_st_hash_wait_fid;
static jfieldID mpool_stat_st_hash_max_nowait_fid;
static jfieldID mpool_stat_st_hash_max_wait_fid;
static jfieldID mpool_stat_st_region_nowait_fid;
static jfieldID mpool_stat_st_region_wait_fid;
static jfieldID mpool_stat_st_mvcc_frozen_fid;
static jfieldID mpool_stat_st_mvcc_thawed_fid;
static jfieldID mpool_stat_st_mvcc_freed_fid;
static jfieldID mpool_stat_st_alloc_fid;
static jfieldID mpool_stat_st_alloc_buckets_fid;
static jfieldID mpool_stat_st_alloc_max_buckets_fid;
static jfieldID mpool_stat_st_alloc_pages_fid;
static jfieldID mpool_stat_st_alloc_max_pages_fid;
static jfieldID mpool_stat_st_io_wait_fid;
static jfieldID mpool_stat_st_sync_interrupted_fid;
static jfieldID mpool_stat_st_regsize_fid;
static jfieldID mutex_stat_st_mutex_align_fid;
static jfieldID mutex_stat_st_mutex_tas_spins_fid;
static jfieldID mutex_stat_st_mutex_cnt_fid;
static jfieldID mutex_stat_st_mutex_free_fid;
static jfieldID mutex_stat_st_mutex_inuse_fid;
static jfieldID mutex_stat_st_mutex_inuse_max_fid;
static jfieldID mutex_stat_st_region_wait_fid;
static jfieldID mutex_stat_st_region_nowait_fid;
static jfieldID mutex_stat_st_regsize_fid;
static jfieldID qam_stat_qs_magic_fid;
static jfieldID qam_stat_qs_version_fid;
static jfieldID qam_stat_qs_metaflags_fid;
static jfieldID qam_stat_qs_nkeys_fid;
static jfieldID qam_stat_qs_ndata_fid;
static jfieldID qam_stat_qs_pagesize_fid;
static jfieldID qam_stat_qs_extentsize_fid;
static jfieldID qam_stat_qs_pages_fid;
static jfieldID qam_stat_qs_re_len_fid;
static jfieldID qam_stat_qs_re_pad_fid;
static jfieldID qam_stat_qs_pgfree_fid;
static jfieldID qam_stat_qs_first_recno_fid;
static jfieldID qam_stat_qs_cur_recno_fid;
static jfieldID rep_stat_st_log_queued_fid;
static jfieldID rep_stat_st_startup_complete_fid;
static jfieldID rep_stat_st_status_fid;
static jfieldID rep_stat_st_next_lsn_fid;
static jfieldID rep_stat_st_waiting_lsn_fid;
static jfieldID rep_stat_st_max_perm_lsn_fid;
static jfieldID rep_stat_st_next_pg_fid;
static jfieldID rep_stat_st_waiting_pg_fid;
static jfieldID rep_stat_st_dupmasters_fid;
static jfieldID rep_stat_st_env_id_fid;
static jfieldID rep_stat_st_env_priority_fid;
static jfieldID rep_stat_st_bulk_fills_fid;
static jfieldID rep_stat_st_bulk_overflows_fid;
static jfieldID rep_stat_st_bulk_records_fid;
static jfieldID rep_stat_st_bulk_transfers_fid;
static jfieldID rep_stat_st_client_rerequests_fid;
static jfieldID rep_stat_st_client_svc_req_fid;
static jfieldID rep_stat_st_client_svc_miss_fid;
static jfieldID rep_stat_st_gen_fid;
static jfieldID rep_stat_st_egen_fid;
static jfieldID rep_stat_st_log_duplicated_fid;
static jfieldID rep_stat_st_log_queued_max_fid;
static jfieldID rep_stat_st_log_queued_total_fid;
static jfieldID rep_stat_st_log_records_fid;
static jfieldID rep_stat_st_log_requested_fid;
static jfieldID rep_stat_st_master_fid;
static jfieldID rep_stat_st_master_changes_fid;
static jfieldID rep_stat_st_msgs_badgen_fid;
static jfieldID rep_stat_st_msgs_processed_fid;
static jfieldID rep_stat_st_msgs_recover_fid;
static jfieldID rep_stat_st_msgs_send_failures_fid;
static jfieldID rep_stat_st_msgs_sent_fid;
static jfieldID rep_stat_st_newsites_fid;
static jfieldID rep_stat_st_nsites_fid;
static jfieldID rep_stat_st_nthrottles_fid;
static jfieldID rep_stat_st_outdated_fid;
static jfieldID rep_stat_st_pg_duplicated_fid;
static jfieldID rep_stat_st_pg_records_fid;
static jfieldID rep_stat_st_pg_requested_fid;
static jfieldID rep_stat_st_txns_applied_fid;
static jfieldID rep_stat_st_startsync_delayed_fid;
static jfieldID rep_stat_st_elections_fid;
static jfieldID rep_stat_st_elections_won_fid;
static jfieldID rep_stat_st_election_cur_winner_fid;
static jfieldID rep_stat_st_election_gen_fid;
static jfieldID rep_stat_st_election_lsn_fid;
static jfieldID rep_stat_st_election_nsites_fid;
static jfieldID rep_stat_st_election_nvotes_fid;
static jfieldID rep_stat_st_election_priority_fid;
static jfieldID rep_stat_st_election_status_fid;
static jfieldID rep_stat_st_election_tiebreaker_fid;
static jfieldID rep_stat_st_election_votes_fid;
static jfieldID rep_stat_st_election_sec_fid;
static jfieldID rep_stat_st_election_usec_fid;
static jfieldID rep_stat_st_max_lease_sec_fid;
static jfieldID rep_stat_st_max_lease_usec_fid;
static jfieldID repmgr_stat_st_perm_failed_fid;
static jfieldID repmgr_stat_st_msgs_queued_fid;
static jfieldID repmgr_stat_st_msgs_dropped_fid;
static jfieldID repmgr_stat_st_connection_drop_fid;
static jfieldID repmgr_stat_st_connect_fail_fid;
static jfieldID seq_stat_st_wait_fid;
static jfieldID seq_stat_st_nowait_fid;
static jfieldID seq_stat_st_current_fid;
static jfieldID seq_stat_st_value_fid;
static jfieldID seq_stat_st_last_value_fid;
static jfieldID seq_stat_st_min_fid;
static jfieldID seq_stat_st_max_fid;
static jfieldID seq_stat_st_cache_size_fid;
static jfieldID seq_stat_st_flags_fid;
static jfieldID txn_stat_st_nrestores_fid;
static jfieldID txn_stat_st_last_ckp_fid;
static jfieldID txn_stat_st_time_ckp_fid;
static jfieldID txn_stat_st_last_txnid_fid;
static jfieldID txn_stat_st_maxtxns_fid;
static jfieldID txn_stat_st_naborts_fid;
static jfieldID txn_stat_st_nbegins_fid;
static jfieldID txn_stat_st_ncommits_fid;
static jfieldID txn_stat_st_nactive_fid;
static jfieldID txn_stat_st_nsnapshot_fid;
static jfieldID txn_stat_st_maxnactive_fid;
static jfieldID txn_stat_st_maxnsnapshot_fid;
static jfieldID txn_stat_st_txnarray_fid;
static jfieldID txn_stat_st_region_wait_fid;
static jfieldID txn_stat_st_region_nowait_fid;
static jfieldID txn_stat_st_regsize_fid;
static jfieldID txn_active_txnid_fid;
static jfieldID txn_active_parentid_fid;
static jfieldID txn_active_pid_fid;
static jfieldID txn_active_lsn_fid;
static jfieldID txn_active_read_lsn_fid;
static jfieldID txn_active_mvcc_ref_fid;
static jfieldID txn_active_status_fid;
static jfieldID txn_active_gid_fid;
static jfieldID txn_active_name_fid;
/* END-STAT-FIELD-DECLS */

static jmethodID dbenv_construct, dbt_construct, dblsn_construct;
static jmethodID dbpreplist_construct, dbtxn_construct;
static jmethodID bt_stat_construct, get_err_msg_method, h_stat_construct;
static jmethodID lock_stat_construct, log_stat_construct;
static jmethodID mpool_stat_construct, mpool_fstat_construct;
static jmethodID mutex_stat_construct, qam_stat_construct;
static jmethodID rep_stat_construct, repmgr_stat_construct, seq_stat_construct;
static jmethodID txn_stat_construct, txn_active_construct;
static jmethodID dbex_construct, deadex_construct, lockex_construct;
static jmethodID memex_construct, memex_update_method;
static jmethodID repdupmasterex_construct, rephandledeadex_construct;
static jmethodID repholdelectionex_construct, repjoinfailex_construct;
static jmethodID repmgr_siteinfo_construct, rephost_construct, repleaseexpiredex_construct;
static jmethodID repleasetimeoutex_construct, replockoutex_construct;
static jmethodID repunavailex_construct;
static jmethodID runrecex_construct, versionex_construct;
static jmethodID filenotfoundex_construct, illegalargex_construct;
static jmethodID outofmemerr_construct;
static jmethodID lock_construct;

static jmethodID app_dispatch_method, errcall_method, env_feedback_method;
static jmethodID msgcall_method, paniccall_method, rep_transport_method;
static jmethodID panic_event_notify_method, rep_client_event_notify_method;
static jmethodID rep_elected_event_notify_method;
static jmethodID rep_master_event_notify_method;
static jmethodID rep_new_master_event_notify_method;
static jmethodID rep_perm_failed_event_notify_method;
static jmethodID rep_startup_done_event_notify_method;
static jmethodID write_failed_event_notify_method;

static jmethodID append_recno_method, bt_compare_method, bt_compress_method;
static jmethodID bt_decompress_method, bt_prefix_method;
static jmethodID db_feedback_method, dup_compare_method;
static jmethodID foreignkey_nullify_method, h_compare_method, h_hash_method;
static jmethodID partition_method, seckey_create_method;

static jmethodID outputstream_write_method;

const struct {
	jclass *cl;
	const char *name;
} all_classes[] = {
	{ &dbenv_class, DB_PKG "internal/DbEnv" },
	{ &db_class, DB_PKG "internal/Db" },
	{ &dbc_class, DB_PKG "internal/Dbc" },
	{ &dbt_class, DB_PKG "DatabaseEntry" },
	{ &dblsn_class, DB_PKG "LogSequenceNumber" },
	{ &dbpreplist_class, DB_PKG "PreparedTransaction" },
	{ &dbtxn_class, DB_PKG "internal/DbTxn" },

	{ &bt_stat_class, DB_PKG "BtreeStats" },
	{ &compact_class, DB_PKG "CompactStats" },
	{ &h_stat_class, DB_PKG "HashStats" },
	{ &lock_stat_class, DB_PKG "LockStats" },
	{ &log_stat_class, DB_PKG "LogStats" },
	{ &mpool_fstat_class, DB_PKG "CacheFileStats" },
	{ &mpool_stat_class, DB_PKG "CacheStats" },
	{ &mutex_stat_class, DB_PKG "MutexStats" },
	{ &qam_stat_class, DB_PKG "QueueStats" },
	{ &rep_stat_class, DB_PKG "ReplicationStats" },
	{ &repmgr_stat_class, DB_PKG "ReplicationManagerStats" },
	{ &seq_stat_class, DB_PKG "SequenceStats" },
	{ &txn_stat_class, DB_PKG "TransactionStats" },
	{ &txn_active_class, DB_PKG "TransactionStats$Active" },

	{ &keyrange_class, DB_PKG "KeyRange" },
	{ &lock_class, DB_PKG "internal/DbLock" },
	{ &lockreq_class, DB_PKG "LockRequest" },

	{ &dbex_class, DB_PKG "DatabaseException" },
	{ &deadex_class, DB_PKG "DeadlockException" },
	{ &lockex_class, DB_PKG "LockNotGrantedException" },
	{ &memex_class, DB_PKG "MemoryException" },
	{ &repdupmasterex_class, DB_PKG "ReplicationDuplicateMasterException" },
	{ &rephandledeadex_class, DB_PKG "ReplicationHandleDeadException" },
	{ &repholdelectionex_class, DB_PKG "ReplicationHoldElectionException" },
	{ &rephost_class, DB_PKG "ReplicationHostAddress" },
	{ &repmgr_siteinfo_class, DB_PKG "ReplicationManagerSiteInfo" },
	{ &repjoinfailex_class, DB_PKG "ReplicationJoinFailureException" },
	{ &repleaseexpiredex_class, DB_PKG "ReplicationLeaseExpiredException" },
	{ &repleasetimeoutex_class, DB_PKG "ReplicationLeaseTimeoutException" },
	{ &replockoutex_class, DB_PKG "ReplicationLockoutException" },
	{ &repunavailex_class, DB_PKG "ReplicationSiteUnavailableException" },
	{ &runrecex_class, DB_PKG "RunRecoveryException" },
	{ &versionex_class, DB_PKG "VersionMismatchException" },
	{ &filenotfoundex_class, "java/io/FileNotFoundException" },
	{ &illegalargex_class, "java/lang/IllegalArgumentException" },
	{ &outofmemerr_class, "java/lang/OutOfMemoryError" },

	{ &bytearray_class, "[B" },
	{ &string_class, "java/lang/String" },
	{ &outputstream_class, "java/io/OutputStream" }
};

const struct {
	jfieldID *fid;
	jclass *cl;
	const char *name;
	const char *sig;
} all_fields[] = {
	{ &dbc_cptr_fid, &dbc_class, "swigCPtr", "J" },

	{ &dblsn_file_fid, &dblsn_class, "file", "I" },
	{ &dblsn_offset_fid, &dblsn_class, "offset", "I" },

	{ &dbt_data_fid, &dbt_class, "data", "[B" },
	{ &dbt_data_nio_fid, &dbt_class, "data_nio", "Ljava/nio/ByteBuffer;" },
	{ &dbt_size_fid, &dbt_class, "size", "I" },
	{ &dbt_ulen_fid, &dbt_class, "ulen", "I" },
	{ &dbt_dlen_fid, &dbt_class, "dlen", "I" },
	{ &dbt_doff_fid, &dbt_class, "doff", "I" },
	{ &dbt_flags_fid, &dbt_class, "flags", "I" },
	{ &dbt_offset_fid, &dbt_class, "offset", "I" },

	{ &kr_less_fid, &keyrange_class, "less", "D" },
	{ &kr_equal_fid, &keyrange_class, "equal", "D" },
	{ &kr_greater_fid, &keyrange_class, "greater", "D" },

	{ &lock_cptr_fid, &lock_class, "swigCPtr", "J" },

	{ &lockreq_op_fid, &lockreq_class, "op", "I" },
	{ &lockreq_modeflag_fid, &lockreq_class, "modeFlag", "I" },
	{ &lockreq_timeout_fid, &lockreq_class, "timeout", "I" },
	{ &lockreq_obj_fid, &lockreq_class, "obj",
	    "L" DB_PKG "DatabaseEntry;" },
	{ &lockreq_lock_fid, &lockreq_class, "lock",
	    "L" DB_PKG "internal/DbLock;" },

/* BEGIN-STAT-FIELDS */
	{ &bt_stat_bt_magic_fid, &bt_stat_class, "bt_magic", "I" },
	{ &bt_stat_bt_version_fid, &bt_stat_class, "bt_version", "I" },
	{ &bt_stat_bt_metaflags_fid, &bt_stat_class, "bt_metaflags", "I" },
	{ &bt_stat_bt_nkeys_fid, &bt_stat_class, "bt_nkeys", "I" },
	{ &bt_stat_bt_ndata_fid, &bt_stat_class, "bt_ndata", "I" },
	{ &bt_stat_bt_pagecnt_fid, &bt_stat_class, "bt_pagecnt", "I" },
	{ &bt_stat_bt_pagesize_fid, &bt_stat_class, "bt_pagesize", "I" },
	{ &bt_stat_bt_minkey_fid, &bt_stat_class, "bt_minkey", "I" },
	{ &bt_stat_bt_re_len_fid, &bt_stat_class, "bt_re_len", "I" },
	{ &bt_stat_bt_re_pad_fid, &bt_stat_class, "bt_re_pad", "I" },
	{ &bt_stat_bt_levels_fid, &bt_stat_class, "bt_levels", "I" },
	{ &bt_stat_bt_int_pg_fid, &bt_stat_class, "bt_int_pg", "I" },
	{ &bt_stat_bt_leaf_pg_fid, &bt_stat_class, "bt_leaf_pg", "I" },
	{ &bt_stat_bt_dup_pg_fid, &bt_stat_class, "bt_dup_pg", "I" },
	{ &bt_stat_bt_over_pg_fid, &bt_stat_class, "bt_over_pg", "I" },
	{ &bt_stat_bt_empty_pg_fid, &bt_stat_class, "bt_empty_pg", "I" },
	{ &bt_stat_bt_free_fid, &bt_stat_class, "bt_free", "I" },
	{ &bt_stat_bt_int_pgfree_fid, &bt_stat_class, "bt_int_pgfree", "J" },
	{ &bt_stat_bt_leaf_pgfree_fid, &bt_stat_class, "bt_leaf_pgfree", "J" },
	{ &bt_stat_bt_dup_pgfree_fid, &bt_stat_class, "bt_dup_pgfree", "J" },
	{ &bt_stat_bt_over_pgfree_fid, &bt_stat_class, "bt_over_pgfree", "J" },
	{ &compact_compact_fillpercent_fid, &compact_class, "compact_fillpercent", "I" },
	{ &compact_compact_timeout_fid, &compact_class, "compact_timeout", "I" },
	{ &compact_compact_pages_fid, &compact_class, "compact_pages", "I" },
	{ &compact_compact_pages_free_fid, &compact_class, "compact_pages_free", "I" },
	{ &compact_compact_pages_examine_fid, &compact_class, "compact_pages_examine", "I" },
	{ &compact_compact_levels_fid, &compact_class, "compact_levels", "I" },
	{ &compact_compact_deadlock_fid, &compact_class, "compact_deadlock", "I" },
	{ &compact_compact_pages_truncated_fid, &compact_class, "compact_pages_truncated", "I" },
	{ &compact_compact_truncate_fid, &compact_class, "compact_truncate", "I" },
	{ &h_stat_hash_magic_fid, &h_stat_class, "hash_magic", "I" },
	{ &h_stat_hash_version_fid, &h_stat_class, "hash_version", "I" },
	{ &h_stat_hash_metaflags_fid, &h_stat_class, "hash_metaflags", "I" },
	{ &h_stat_hash_nkeys_fid, &h_stat_class, "hash_nkeys", "I" },
	{ &h_stat_hash_ndata_fid, &h_stat_class, "hash_ndata", "I" },
	{ &h_stat_hash_pagecnt_fid, &h_stat_class, "hash_pagecnt", "I" },
	{ &h_stat_hash_pagesize_fid, &h_stat_class, "hash_pagesize", "I" },
	{ &h_stat_hash_ffactor_fid, &h_stat_class, "hash_ffactor", "I" },
	{ &h_stat_hash_buckets_fid, &h_stat_class, "hash_buckets", "I" },
	{ &h_stat_hash_free_fid, &h_stat_class, "hash_free", "I" },
	{ &h_stat_hash_bfree_fid, &h_stat_class, "hash_bfree", "J" },
	{ &h_stat_hash_bigpages_fid, &h_stat_class, "hash_bigpages", "I" },
	{ &h_stat_hash_big_bfree_fid, &h_stat_class, "hash_big_bfree", "J" },
	{ &h_stat_hash_overflows_fid, &h_stat_class, "hash_overflows", "I" },
	{ &h_stat_hash_ovfl_free_fid, &h_stat_class, "hash_ovfl_free", "J" },
	{ &h_stat_hash_dup_fid, &h_stat_class, "hash_dup", "I" },
	{ &h_stat_hash_dup_free_fid, &h_stat_class, "hash_dup_free", "J" },
	{ &lock_stat_st_id_fid, &lock_stat_class, "st_id", "I" },
	{ &lock_stat_st_cur_maxid_fid, &lock_stat_class, "st_cur_maxid", "I" },
	{ &lock_stat_st_maxlocks_fid, &lock_stat_class, "st_maxlocks", "I" },
	{ &lock_stat_st_maxlockers_fid, &lock_stat_class, "st_maxlockers", "I" },
	{ &lock_stat_st_maxobjects_fid, &lock_stat_class, "st_maxobjects", "I" },
	{ &lock_stat_st_partitions_fid, &lock_stat_class, "st_partitions", "I" },
	{ &lock_stat_st_nmodes_fid, &lock_stat_class, "st_nmodes", "I" },
	{ &lock_stat_st_nlockers_fid, &lock_stat_class, "st_nlockers", "I" },
	{ &lock_stat_st_nlocks_fid, &lock_stat_class, "st_nlocks", "I" },
	{ &lock_stat_st_maxnlocks_fid, &lock_stat_class, "st_maxnlocks", "I" },
	{ &lock_stat_st_maxhlocks_fid, &lock_stat_class, "st_maxhlocks", "I" },
	{ &lock_stat_st_locksteals_fid, &lock_stat_class, "st_locksteals", "J" },
	{ &lock_stat_st_maxlsteals_fid, &lock_stat_class, "st_maxlsteals", "J" },
	{ &lock_stat_st_maxnlockers_fid, &lock_stat_class, "st_maxnlockers", "I" },
	{ &lock_stat_st_nobjects_fid, &lock_stat_class, "st_nobjects", "I" },
	{ &lock_stat_st_maxnobjects_fid, &lock_stat_class, "st_maxnobjects", "I" },
	{ &lock_stat_st_maxhobjects_fid, &lock_stat_class, "st_maxhobjects", "I" },
	{ &lock_stat_st_objectsteals_fid, &lock_stat_class, "st_objectsteals", "J" },
	{ &lock_stat_st_maxosteals_fid, &lock_stat_class, "st_maxosteals", "J" },
	{ &lock_stat_st_nrequests_fid, &lock_stat_class, "st_nrequests", "J" },
	{ &lock_stat_st_nreleases_fid, &lock_stat_class, "st_nreleases", "J" },
	{ &lock_stat_st_nupgrade_fid, &lock_stat_class, "st_nupgrade", "J" },
	{ &lock_stat_st_ndowngrade_fid, &lock_stat_class, "st_ndowngrade", "J" },
	{ &lock_stat_st_lock_wait_fid, &lock_stat_class, "st_lock_wait", "J" },
	{ &lock_stat_st_lock_nowait_fid, &lock_stat_class, "st_lock_nowait", "J" },
	{ &lock_stat_st_ndeadlocks_fid, &lock_stat_class, "st_ndeadlocks", "J" },
	{ &lock_stat_st_locktimeout_fid, &lock_stat_class, "st_locktimeout", "I" },
	{ &lock_stat_st_nlocktimeouts_fid, &lock_stat_class, "st_nlocktimeouts", "J" },
	{ &lock_stat_st_txntimeout_fid, &lock_stat_class, "st_txntimeout", "I" },
	{ &lock_stat_st_ntxntimeouts_fid, &lock_stat_class, "st_ntxntimeouts", "J" },
	{ &lock_stat_st_part_wait_fid, &lock_stat_class, "st_part_wait", "J" },
	{ &lock_stat_st_part_nowait_fid, &lock_stat_class, "st_part_nowait", "J" },
	{ &lock_stat_st_part_max_wait_fid, &lock_stat_class, "st_part_max_wait", "J" },
	{ &lock_stat_st_part_max_nowait_fid, &lock_stat_class, "st_part_max_nowait", "J" },
	{ &lock_stat_st_objs_wait_fid, &lock_stat_class, "st_objs_wait", "J" },
	{ &lock_stat_st_objs_nowait_fid, &lock_stat_class, "st_objs_nowait", "J" },
	{ &lock_stat_st_lockers_wait_fid, &lock_stat_class, "st_lockers_wait", "J" },
	{ &lock_stat_st_lockers_nowait_fid, &lock_stat_class, "st_lockers_nowait", "J" },
	{ &lock_stat_st_region_wait_fid, &lock_stat_class, "st_region_wait", "J" },
	{ &lock_stat_st_region_nowait_fid, &lock_stat_class, "st_region_nowait", "J" },
	{ &lock_stat_st_hash_len_fid, &lock_stat_class, "st_hash_len", "I" },
	{ &lock_stat_st_regsize_fid, &lock_stat_class, "st_regsize", "I" },
	{ &log_stat_st_magic_fid, &log_stat_class, "st_magic", "I" },
	{ &log_stat_st_version_fid, &log_stat_class, "st_version", "I" },
	{ &log_stat_st_mode_fid, &log_stat_class, "st_mode", "I" },
	{ &log_stat_st_lg_bsize_fid, &log_stat_class, "st_lg_bsize", "I" },
	{ &log_stat_st_lg_size_fid, &log_stat_class, "st_lg_size", "I" },
	{ &log_stat_st_wc_bytes_fid, &log_stat_class, "st_wc_bytes", "I" },
	{ &log_stat_st_wc_mbytes_fid, &log_stat_class, "st_wc_mbytes", "I" },
	{ &log_stat_st_record_fid, &log_stat_class, "st_record", "J" },
	{ &log_stat_st_w_bytes_fid, &log_stat_class, "st_w_bytes", "I" },
	{ &log_stat_st_w_mbytes_fid, &log_stat_class, "st_w_mbytes", "I" },
	{ &log_stat_st_wcount_fid, &log_stat_class, "st_wcount", "J" },
	{ &log_stat_st_wcount_fill_fid, &log_stat_class, "st_wcount_fill", "J" },
	{ &log_stat_st_rcount_fid, &log_stat_class, "st_rcount", "J" },
	{ &log_stat_st_scount_fid, &log_stat_class, "st_scount", "J" },
	{ &log_stat_st_region_wait_fid, &log_stat_class, "st_region_wait", "J" },
	{ &log_stat_st_region_nowait_fid, &log_stat_class, "st_region_nowait", "J" },
	{ &log_stat_st_cur_file_fid, &log_stat_class, "st_cur_file", "I" },
	{ &log_stat_st_cur_offset_fid, &log_stat_class, "st_cur_offset", "I" },
	{ &log_stat_st_disk_file_fid, &log_stat_class, "st_disk_file", "I" },
	{ &log_stat_st_disk_offset_fid, &log_stat_class, "st_disk_offset", "I" },
	{ &log_stat_st_maxcommitperflush_fid, &log_stat_class, "st_maxcommitperflush", "I" },
	{ &log_stat_st_mincommitperflush_fid, &log_stat_class, "st_mincommitperflush", "I" },
	{ &log_stat_st_regsize_fid, &log_stat_class, "st_regsize", "I" },
	{ &mpool_fstat_file_name_fid, &mpool_fstat_class, "file_name", "Ljava/lang/String;" },
	{ &mpool_fstat_st_pagesize_fid, &mpool_fstat_class, "st_pagesize", "I" },
	{ &mpool_fstat_st_map_fid, &mpool_fstat_class, "st_map", "I" },
	{ &mpool_fstat_st_cache_hit_fid, &mpool_fstat_class, "st_cache_hit", "J" },
	{ &mpool_fstat_st_cache_miss_fid, &mpool_fstat_class, "st_cache_miss", "J" },
	{ &mpool_fstat_st_page_create_fid, &mpool_fstat_class, "st_page_create", "J" },
	{ &mpool_fstat_st_page_in_fid, &mpool_fstat_class, "st_page_in", "J" },
	{ &mpool_fstat_st_page_out_fid, &mpool_fstat_class, "st_page_out", "J" },
	{ &mpool_stat_st_gbytes_fid, &mpool_stat_class, "st_gbytes", "I" },
	{ &mpool_stat_st_bytes_fid, &mpool_stat_class, "st_bytes", "I" },
	{ &mpool_stat_st_ncache_fid, &mpool_stat_class, "st_ncache", "I" },
	{ &mpool_stat_st_max_ncache_fid, &mpool_stat_class, "st_max_ncache", "I" },
	{ &mpool_stat_st_mmapsize_fid, &mpool_stat_class, "st_mmapsize", "I" },
	{ &mpool_stat_st_maxopenfd_fid, &mpool_stat_class, "st_maxopenfd", "I" },
	{ &mpool_stat_st_maxwrite_fid, &mpool_stat_class, "st_maxwrite", "I" },
	{ &mpool_stat_st_maxwrite_sleep_fid, &mpool_stat_class, "st_maxwrite_sleep", "I" },
	{ &mpool_stat_st_pages_fid, &mpool_stat_class, "st_pages", "I" },
	{ &mpool_stat_st_map_fid, &mpool_stat_class, "st_map", "I" },
	{ &mpool_stat_st_cache_hit_fid, &mpool_stat_class, "st_cache_hit", "J" },
	{ &mpool_stat_st_cache_miss_fid, &mpool_stat_class, "st_cache_miss", "J" },
	{ &mpool_stat_st_page_create_fid, &mpool_stat_class, "st_page_create", "J" },
	{ &mpool_stat_st_page_in_fid, &mpool_stat_class, "st_page_in", "J" },
	{ &mpool_stat_st_page_out_fid, &mpool_stat_class, "st_page_out", "J" },
	{ &mpool_stat_st_ro_evict_fid, &mpool_stat_class, "st_ro_evict", "J" },
	{ &mpool_stat_st_rw_evict_fid, &mpool_stat_class, "st_rw_evict", "J" },
	{ &mpool_stat_st_page_trickle_fid, &mpool_stat_class, "st_page_trickle", "J" },
	{ &mpool_stat_st_page_clean_fid, &mpool_stat_class, "st_page_clean", "I" },
	{ &mpool_stat_st_page_dirty_fid, &mpool_stat_class, "st_page_dirty", "I" },
	{ &mpool_stat_st_hash_buckets_fid, &mpool_stat_class, "st_hash_buckets", "I" },
	{ &mpool_stat_st_pagesize_fid, &mpool_stat_class, "st_pagesize", "I" },
	{ &mpool_stat_st_hash_searches_fid, &mpool_stat_class, "st_hash_searches", "I" },
	{ &mpool_stat_st_hash_longest_fid, &mpool_stat_class, "st_hash_longest", "I" },
	{ &mpool_stat_st_hash_examined_fid, &mpool_stat_class, "st_hash_examined", "J" },
	{ &mpool_stat_st_hash_nowait_fid, &mpool_stat_class, "st_hash_nowait", "J" },
	{ &mpool_stat_st_hash_wait_fid, &mpool_stat_class, "st_hash_wait", "J" },
	{ &mpool_stat_st_hash_max_nowait_fid, &mpool_stat_class, "st_hash_max_nowait", "J" },
	{ &mpool_stat_st_hash_max_wait_fid, &mpool_stat_class, "st_hash_max_wait", "J" },
	{ &mpool_stat_st_region_nowait_fid, &mpool_stat_class, "st_region_nowait", "J" },
	{ &mpool_stat_st_region_wait_fid, &mpool_stat_class, "st_region_wait", "J" },
	{ &mpool_stat_st_mvcc_frozen_fid, &mpool_stat_class, "st_mvcc_frozen", "J" },
	{ &mpool_stat_st_mvcc_thawed_fid, &mpool_stat_class, "st_mvcc_thawed", "J" },
	{ &mpool_stat_st_mvcc_freed_fid, &mpool_stat_class, "st_mvcc_freed", "J" },
	{ &mpool_stat_st_alloc_fid, &mpool_stat_class, "st_alloc", "J" },
	{ &mpool_stat_st_alloc_buckets_fid, &mpool_stat_class, "st_alloc_buckets", "J" },
	{ &mpool_stat_st_alloc_max_buckets_fid, &mpool_stat_class, "st_alloc_max_buckets", "J" },
	{ &mpool_stat_st_alloc_pages_fid, &mpool_stat_class, "st_alloc_pages", "J" },
	{ &mpool_stat_st_alloc_max_pages_fid, &mpool_stat_class, "st_alloc_max_pages", "J" },
	{ &mpool_stat_st_io_wait_fid, &mpool_stat_class, "st_io_wait", "J" },
	{ &mpool_stat_st_sync_interrupted_fid, &mpool_stat_class, "st_sync_interrupted", "J" },
	{ &mpool_stat_st_regsize_fid, &mpool_stat_class, "st_regsize", "I" },
	{ &mutex_stat_st_mutex_align_fid, &mutex_stat_class, "st_mutex_align", "I" },
	{ &mutex_stat_st_mutex_tas_spins_fid, &mutex_stat_class, "st_mutex_tas_spins", "I" },
	{ &mutex_stat_st_mutex_cnt_fid, &mutex_stat_class, "st_mutex_cnt", "I" },
	{ &mutex_stat_st_mutex_free_fid, &mutex_stat_class, "st_mutex_free", "I" },
	{ &mutex_stat_st_mutex_inuse_fid, &mutex_stat_class, "st_mutex_inuse", "I" },
	{ &mutex_stat_st_mutex_inuse_max_fid, &mutex_stat_class, "st_mutex_inuse_max", "I" },
	{ &mutex_stat_st_region_wait_fid, &mutex_stat_class, "st_region_wait", "J" },
	{ &mutex_stat_st_region_nowait_fid, &mutex_stat_class, "st_region_nowait", "J" },
	{ &mutex_stat_st_regsize_fid, &mutex_stat_class, "st_regsize", "I" },
	{ &qam_stat_qs_magic_fid, &qam_stat_class, "qs_magic", "I" },
	{ &qam_stat_qs_version_fid, &qam_stat_class, "qs_version", "I" },
	{ &qam_stat_qs_metaflags_fid, &qam_stat_class, "qs_metaflags", "I" },
	{ &qam_stat_qs_nkeys_fid, &qam_stat_class, "qs_nkeys", "I" },
	{ &qam_stat_qs_ndata_fid, &qam_stat_class, "qs_ndata", "I" },
	{ &qam_stat_qs_pagesize_fid, &qam_stat_class, "qs_pagesize", "I" },
	{ &qam_stat_qs_extentsize_fid, &qam_stat_class, "qs_extentsize", "I" },
	{ &qam_stat_qs_pages_fid, &qam_stat_class, "qs_pages", "I" },
	{ &qam_stat_qs_re_len_fid, &qam_stat_class, "qs_re_len", "I" },
	{ &qam_stat_qs_re_pad_fid, &qam_stat_class, "qs_re_pad", "I" },
	{ &qam_stat_qs_pgfree_fid, &qam_stat_class, "qs_pgfree", "I" },
	{ &qam_stat_qs_first_recno_fid, &qam_stat_class, "qs_first_recno", "I" },
	{ &qam_stat_qs_cur_recno_fid, &qam_stat_class, "qs_cur_recno", "I" },
	{ &rep_stat_st_log_queued_fid, &rep_stat_class, "st_log_queued", "J" },
	{ &rep_stat_st_startup_complete_fid, &rep_stat_class, "st_startup_complete", "I" },
	{ &rep_stat_st_status_fid, &rep_stat_class, "st_status", "I" },
	{ &rep_stat_st_next_lsn_fid, &rep_stat_class, "st_next_lsn", "L" DB_PKG "LogSequenceNumber;" },
	{ &rep_stat_st_waiting_lsn_fid, &rep_stat_class, "st_waiting_lsn", "L" DB_PKG "LogSequenceNumber;" },
	{ &rep_stat_st_max_perm_lsn_fid, &rep_stat_class, "st_max_perm_lsn", "L" DB_PKG "LogSequenceNumber;" },
	{ &rep_stat_st_next_pg_fid, &rep_stat_class, "st_next_pg", "I" },
	{ &rep_stat_st_waiting_pg_fid, &rep_stat_class, "st_waiting_pg", "I" },
	{ &rep_stat_st_dupmasters_fid, &rep_stat_class, "st_dupmasters", "I" },
	{ &rep_stat_st_env_id_fid, &rep_stat_class, "st_env_id", "I" },
	{ &rep_stat_st_env_priority_fid, &rep_stat_class, "st_env_priority", "I" },
	{ &rep_stat_st_bulk_fills_fid, &rep_stat_class, "st_bulk_fills", "J" },
	{ &rep_stat_st_bulk_overflows_fid, &rep_stat_class, "st_bulk_overflows", "J" },
	{ &rep_stat_st_bulk_records_fid, &rep_stat_class, "st_bulk_records", "J" },
	{ &rep_stat_st_bulk_transfers_fid, &rep_stat_class, "st_bulk_transfers", "J" },
	{ &rep_stat_st_client_rerequests_fid, &rep_stat_class, "st_client_rerequests", "J" },
	{ &rep_stat_st_client_svc_req_fid, &rep_stat_class, "st_client_svc_req", "J" },
	{ &rep_stat_st_client_svc_miss_fid, &rep_stat_class, "st_client_svc_miss", "J" },
	{ &rep_stat_st_gen_fid, &rep_stat_class, "st_gen", "I" },
	{ &rep_stat_st_egen_fid, &rep_stat_class, "st_egen", "I" },
	{ &rep_stat_st_log_duplicated_fid, &rep_stat_class, "st_log_duplicated", "J" },
	{ &rep_stat_st_log_queued_max_fid, &rep_stat_class, "st_log_queued_max", "J" },
	{ &rep_stat_st_log_queued_total_fid, &rep_stat_class, "st_log_queued_total", "J" },
	{ &rep_stat_st_log_records_fid, &rep_stat_class, "st_log_records", "J" },
	{ &rep_stat_st_log_requested_fid, &rep_stat_class, "st_log_requested", "J" },
	{ &rep_stat_st_master_fid, &rep_stat_class, "st_master", "I" },
	{ &rep_stat_st_master_changes_fid, &rep_stat_class, "st_master_changes", "J" },
	{ &rep_stat_st_msgs_badgen_fid, &rep_stat_class, "st_msgs_badgen", "J" },
	{ &rep_stat_st_msgs_processed_fid, &rep_stat_class, "st_msgs_processed", "J" },
	{ &rep_stat_st_msgs_recover_fid, &rep_stat_class, "st_msgs_recover", "J" },
	{ &rep_stat_st_msgs_send_failures_fid, &rep_stat_class, "st_msgs_send_failures", "J" },
	{ &rep_stat_st_msgs_sent_fid, &rep_stat_class, "st_msgs_sent", "J" },
	{ &rep_stat_st_newsites_fid, &rep_stat_class, "st_newsites", "J" },
	{ &rep_stat_st_nsites_fid, &rep_stat_class, "st_nsites", "I" },
	{ &rep_stat_st_nthrottles_fid, &rep_stat_class, "st_nthrottles", "J" },
	{ &rep_stat_st_outdated_fid, &rep_stat_class, "st_outdated", "J" },
	{ &rep_stat_st_pg_duplicated_fid, &rep_stat_class, "st_pg_duplicated", "J" },
	{ &rep_stat_st_pg_records_fid, &rep_stat_class, "st_pg_records", "J" },
	{ &rep_stat_st_pg_requested_fid, &rep_stat_class, "st_pg_requested", "J" },
	{ &rep_stat_st_txns_applied_fid, &rep_stat_class, "st_txns_applied", "J" },
	{ &rep_stat_st_startsync_delayed_fid, &rep_stat_class, "st_startsync_delayed", "J" },
	{ &rep_stat_st_elections_fid, &rep_stat_class, "st_elections", "J" },
	{ &rep_stat_st_elections_won_fid, &rep_stat_class, "st_elections_won", "J" },
	{ &rep_stat_st_election_cur_winner_fid, &rep_stat_class, "st_election_cur_winner", "I" },
	{ &rep_stat_st_election_gen_fid, &rep_stat_class, "st_election_gen", "I" },
	{ &rep_stat_st_election_lsn_fid, &rep_stat_class, "st_election_lsn", "L" DB_PKG "LogSequenceNumber;" },
	{ &rep_stat_st_election_nsites_fid, &rep_stat_class, "st_election_nsites", "I" },
	{ &rep_stat_st_election_nvotes_fid, &rep_stat_class, "st_election_nvotes", "I" },
	{ &rep_stat_st_election_priority_fid, &rep_stat_class, "st_election_priority", "I" },
	{ &rep_stat_st_election_status_fid, &rep_stat_class, "st_election_status", "I" },
	{ &rep_stat_st_election_tiebreaker_fid, &rep_stat_class, "st_election_tiebreaker", "I" },
	{ &rep_stat_st_election_votes_fid, &rep_stat_class, "st_election_votes", "I" },
	{ &rep_stat_st_election_sec_fid, &rep_stat_class, "st_election_sec", "I" },
	{ &rep_stat_st_election_usec_fid, &rep_stat_class, "st_election_usec", "I" },
	{ &rep_stat_st_max_lease_sec_fid, &rep_stat_class, "st_max_lease_sec", "I" },
	{ &rep_stat_st_max_lease_usec_fid, &rep_stat_class, "st_max_lease_usec", "I" },
	{ &repmgr_stat_st_perm_failed_fid, &repmgr_stat_class, "st_perm_failed", "J" },
	{ &repmgr_stat_st_msgs_queued_fid, &repmgr_stat_class, "st_msgs_queued", "J" },
	{ &repmgr_stat_st_msgs_dropped_fid, &repmgr_stat_class, "st_msgs_dropped", "J" },
	{ &repmgr_stat_st_connection_drop_fid, &repmgr_stat_class, "st_connection_drop", "J" },
	{ &repmgr_stat_st_connect_fail_fid, &repmgr_stat_class, "st_connect_fail", "J" },
	{ &seq_stat_st_wait_fid, &seq_stat_class, "st_wait", "J" },
	{ &seq_stat_st_nowait_fid, &seq_stat_class, "st_nowait", "J" },
	{ &seq_stat_st_current_fid, &seq_stat_class, "st_current", "J" },
	{ &seq_stat_st_value_fid, &seq_stat_class, "st_value", "J" },
	{ &seq_stat_st_last_value_fid, &seq_stat_class, "st_last_value", "J" },
	{ &seq_stat_st_min_fid, &seq_stat_class, "st_min", "J" },
	{ &seq_stat_st_max_fid, &seq_stat_class, "st_max", "J" },
	{ &seq_stat_st_cache_size_fid, &seq_stat_class, "st_cache_size", "I" },
	{ &seq_stat_st_flags_fid, &seq_stat_class, "st_flags", "I" },
	{ &txn_stat_st_nrestores_fid, &txn_stat_class, "st_nrestores", "I" },
	{ &txn_stat_st_last_ckp_fid, &txn_stat_class, "st_last_ckp", "L" DB_PKG "LogSequenceNumber;" },
	{ &txn_stat_st_time_ckp_fid, &txn_stat_class, "st_time_ckp", "J" },
	{ &txn_stat_st_last_txnid_fid, &txn_stat_class, "st_last_txnid", "I" },
	{ &txn_stat_st_maxtxns_fid, &txn_stat_class, "st_maxtxns", "I" },
	{ &txn_stat_st_naborts_fid, &txn_stat_class, "st_naborts", "J" },
	{ &txn_stat_st_nbegins_fid, &txn_stat_class, "st_nbegins", "J" },
	{ &txn_stat_st_ncommits_fid, &txn_stat_class, "st_ncommits", "J" },
	{ &txn_stat_st_nactive_fid, &txn_stat_class, "st_nactive", "I" },
	{ &txn_stat_st_nsnapshot_fid, &txn_stat_class, "st_nsnapshot", "I" },
	{ &txn_stat_st_maxnactive_fid, &txn_stat_class, "st_maxnactive", "I" },
	{ &txn_stat_st_maxnsnapshot_fid, &txn_stat_class, "st_maxnsnapshot", "I" },
	{ &txn_stat_st_txnarray_fid, &txn_stat_class, "st_txnarray", "[L" DB_PKG "TransactionStats$Active;" },
	{ &txn_stat_st_region_wait_fid, &txn_stat_class, "st_region_wait", "J" },
	{ &txn_stat_st_region_nowait_fid, &txn_stat_class, "st_region_nowait", "J" },
	{ &txn_stat_st_regsize_fid, &txn_stat_class, "st_regsize", "I" },
	{ &txn_active_txnid_fid, &txn_active_class, "txnid", "I" },
	{ &txn_active_parentid_fid, &txn_active_class, "parentid", "I" },
	{ &txn_active_pid_fid, &txn_active_class, "pid", "I" },
	{ &txn_active_lsn_fid, &txn_active_class, "lsn", "L" DB_PKG "LogSequenceNumber;" },
	{ &txn_active_read_lsn_fid, &txn_active_class, "read_lsn", "L" DB_PKG "LogSequenceNumber;" },
	{ &txn_active_mvcc_ref_fid, &txn_active_class, "mvcc_ref", "I" },
	{ &txn_active_status_fid, &txn_active_class, "status", "I" },
	{ &txn_active_gid_fid, &txn_active_class, "gid", "[B" },
	{ &txn_active_name_fid, &txn_active_class, "name", "Ljava/lang/String;" },
/* END-STAT-FIELDS */

	{ &repmgr_siteinfo_status_fid, &repmgr_siteinfo_class, "status", "I" }
};

const struct {
	jmethodID *mid;
	jclass *cl;
	const char *name;
	const char *sig;
} all_methods[] = {
	{ &dbenv_construct, &dbenv_class, "<init>", "(JZ)V" },
	{ &dbt_construct, &dbt_class, "<init>", "()V" },
	{ &dblsn_construct, &dblsn_class, "<init>", "(II)V" },
	{ &dbpreplist_construct, &dbpreplist_class, "<init>",
	    "(L" DB_PKG "internal/DbTxn;[B)V" },
	{ &dbtxn_construct, &dbtxn_class, "<init>", "(JZ)V" },

	{ &bt_stat_construct, &bt_stat_class, "<init>", "()V" },
	{ &get_err_msg_method, &dbenv_class, "get_err_msg",
	    "(Ljava/lang/String;)Ljava/lang/String;" },
	{ &h_stat_construct, &h_stat_class, "<init>", "()V" },
	{ &lock_stat_construct, &lock_stat_class, "<init>", "()V" },
	{ &log_stat_construct, &log_stat_class, "<init>", "()V" },
	{ &mpool_stat_construct, &mpool_stat_class, "<init>", "()V" },
	{ &mpool_fstat_construct, &mpool_fstat_class, "<init>", "()V" },
	{ &mutex_stat_construct, &mutex_stat_class, "<init>", "()V" },
	{ &qam_stat_construct, &qam_stat_class, "<init>", "()V" },
	{ &rep_stat_construct, &rep_stat_class, "<init>", "()V" },
	{ &repmgr_stat_construct, &repmgr_stat_class, "<init>", "()V" },
	{ &seq_stat_construct, &seq_stat_class, "<init>", "()V" },
	{ &txn_stat_construct, &txn_stat_class, "<init>", "()V" },
	{ &txn_active_construct, &txn_active_class, "<init>", "()V" },
	{ &rephost_construct, &rephost_class, "<init>", "(Ljava/lang/String;I)V" },
        { &repmgr_siteinfo_construct, &repmgr_siteinfo_class, "<init>", 
	    "(L" DB_PKG "ReplicationHostAddress;I)V" },

	{ &dbex_construct, &dbex_class, "<init>",
	    "(Ljava/lang/String;IL" DB_PKG "internal/DbEnv;)V" },
	{ &deadex_construct, &deadex_class, "<init>",
	    "(Ljava/lang/String;IL" DB_PKG "internal/DbEnv;)V" },
	{ &lockex_construct, &lockex_class, "<init>",
	    "(Ljava/lang/String;IIL" DB_PKG "DatabaseEntry;L"
	    DB_PKG "internal/DbLock;IL" DB_PKG "internal/DbEnv;)V" },
	{ &memex_construct, &memex_class, "<init>",
	    "(Ljava/lang/String;L" DB_PKG "DatabaseEntry;IL"
	    DB_PKG "internal/DbEnv;)V" },
	{ &memex_update_method, &memex_class, "updateDatabaseEntry",
	    "(L" DB_PKG "DatabaseEntry;)V" },
	{ &repdupmasterex_construct, &repdupmasterex_class, "<init>",
	    "(Ljava/lang/String;IL" DB_PKG "internal/DbEnv;)V" },
	{ &rephandledeadex_construct, &rephandledeadex_class, "<init>",
	    "(Ljava/lang/String;IL" DB_PKG "internal/DbEnv;)V" },
	{ &repholdelectionex_construct, &repholdelectionex_class, "<init>",
	    "(Ljava/lang/String;IL" DB_PKG "internal/DbEnv;)V" },
	{ &repjoinfailex_construct, &repjoinfailex_class, "<init>",
	    "(Ljava/lang/String;IL" DB_PKG "internal/DbEnv;)V" },
	{ &repleaseexpiredex_construct, &repleaseexpiredex_class, "<init>",
	    "(Ljava/lang/String;IL" DB_PKG "internal/DbEnv;)V" },
	{ &repleasetimeoutex_construct, &repleasetimeoutex_class, "<init>",
	    "(Ljava/lang/String;IL" DB_PKG "internal/DbEnv;)V" },
	{ &replockoutex_construct, &replockoutex_class, "<init>",
	    "(Ljava/lang/String;IL" DB_PKG "internal/DbEnv;)V" },
	{ &repunavailex_construct, &repunavailex_class, "<init>",
	    "(Ljava/lang/String;IL" DB_PKG "internal/DbEnv;)V" },
	{ &runrecex_construct, &runrecex_class, "<init>",
	    "(Ljava/lang/String;IL" DB_PKG "internal/DbEnv;)V" },
	{ &versionex_construct, &versionex_class, "<init>",
	    "(Ljava/lang/String;IL" DB_PKG "internal/DbEnv;)V" },
	{ &filenotfoundex_construct, &filenotfoundex_class, "<init>",
	    "(Ljava/lang/String;)V" },
	{ &illegalargex_construct, &illegalargex_class, "<init>",
	    "(Ljava/lang/String;)V" },
	{ &outofmemerr_construct, &outofmemerr_class, "<init>",
	    "(Ljava/lang/String;)V" },

	{ &lock_construct, &lock_class, "<init>", "(JZ)V" },

	{ &app_dispatch_method, &dbenv_class, "handle_app_dispatch",
	    "(L" DB_PKG "DatabaseEntry;L" DB_PKG "LogSequenceNumber;I)I" },
	{ &panic_event_notify_method, &dbenv_class, "handle_panic_event_notify",
	    "()V" },
	{ &rep_client_event_notify_method, &dbenv_class, 
	    "handle_rep_client_event_notify", "()V" },
	{ &rep_elected_event_notify_method, &dbenv_class, 
	    "handle_rep_elected_event_notify" ,"()V" },
	{ &rep_master_event_notify_method, &dbenv_class, 
	    "handle_rep_master_event_notify", "()V" },
	{ &rep_new_master_event_notify_method, &dbenv_class, 
	    "handle_rep_new_master_event_notify", "(I)V" },
	{ &rep_perm_failed_event_notify_method, &dbenv_class, 
	    "handle_rep_perm_failed_event_notify", "()V" },
	{ &rep_startup_done_event_notify_method, &dbenv_class, 
	    "handle_rep_startup_done_event_notify", "()V" },
	{ &write_failed_event_notify_method, &dbenv_class, 
	    "handle_write_failed_event_notify", "(I)V" },
	{ &env_feedback_method, &dbenv_class, "handle_env_feedback", "(II)V" },
	{ &errcall_method, &dbenv_class, "handle_error",
	    "(Ljava/lang/String;)V" },
	{ &msgcall_method, &dbenv_class, "handle_message",
	    "(Ljava/lang/String;)V" },
	{ &paniccall_method, &dbenv_class, "handle_panic",
	    "(L" DB_PKG "DatabaseException;)V" },
	{ &rep_transport_method, &dbenv_class, "handle_rep_transport",
	    "(L" DB_PKG "DatabaseEntry;L" DB_PKG "DatabaseEntry;L"
	    DB_PKG "LogSequenceNumber;II)I" },

	{ &append_recno_method, &db_class, "handle_append_recno",
	    "(L" DB_PKG "DatabaseEntry;I)V" },
	{ &bt_compare_method, &db_class, "handle_bt_compare",
	    "([B[B)I" },
	{ &bt_compress_method, &db_class, "handle_bt_compress",
	    "(L" DB_PKG "DatabaseEntry;L" DB_PKG "DatabaseEntry;L" DB_PKG
	    "DatabaseEntry;L" DB_PKG "DatabaseEntry;L" DB_PKG
	    "DatabaseEntry;)I" },
	{ &bt_decompress_method, &db_class, "handle_bt_decompress",
	    "(L" DB_PKG "DatabaseEntry;L" DB_PKG "DatabaseEntry;L" DB_PKG
	    "DatabaseEntry;L" DB_PKG "DatabaseEntry;L" DB_PKG
	    "DatabaseEntry;)I" },
	{ &bt_prefix_method, &db_class, "handle_bt_prefix",
	    "(L" DB_PKG "DatabaseEntry;L" DB_PKG "DatabaseEntry;)I" },
	{ &db_feedback_method, &db_class, "handle_db_feedback", "(II)V" },
	{ &dup_compare_method, &db_class, "handle_dup_compare",
	    "([B[B)I" },
	{ &foreignkey_nullify_method, &db_class, "handle_foreignkey_nullify",
	    "(L" DB_PKG "DatabaseEntry;L" DB_PKG "DatabaseEntry;L" DB_PKG
	    "DatabaseEntry;)Z" },
	{ &h_compare_method, &db_class, "handle_h_compare",
	    "([B[B)I" },
	{ &h_hash_method, &db_class, "handle_h_hash", "([BI)I" },
	{ &partition_method, &db_class, "handle_partition", 
            "(L" DB_PKG "DatabaseEntry;)I" },
	{ &seckey_create_method, &db_class, "handle_seckey_create",
	    "(L" DB_PKG "DatabaseEntry;L" DB_PKG "DatabaseEntry;)[L"
	    DB_PKG "DatabaseEntry;" },

	{ &outputstream_write_method, &outputstream_class, "write", "([BII)V" }
};

#define	NELEM(x) (sizeof (x) / sizeof (x[0]))

SWIGEXPORT void JNICALL Java_com_sleepycat_db_internal_db_1javaJNI_initialize(
    JNIEnv *jenv, jclass clazz)
{
	jclass cl;
	unsigned int i, j;

	COMPQUIET(clazz, NULL);

	if ((*jenv)->GetJavaVM(jenv, &javavm) != 0) {
		__db_errx(NULL, "Cannot get Java VM");
		return;
	}

	for (i = 0; i < NELEM(all_classes); i++) {
		cl = (*jenv)->FindClass(jenv, all_classes[i].name);
		if (cl == NULL) {
			fprintf(stderr,
			    "Failed to load class %s - check CLASSPATH\n",
			    all_classes[i].name);
			return;
		}

		/*
		 * Wrap classes in GlobalRefs so we keep the reference between
		 * calls.
		 */
		*all_classes[i].cl = (jclass)(*jenv)->NewGlobalRef(jenv, cl);

		if (*all_classes[i].cl == NULL) {
			fprintf(stderr,
			    "Failed to create a global reference for %s\n",
			    all_classes[i].name);
			return;
		}
	}

	/* Get field IDs */
	for (i = 0; i < NELEM(all_fields); i++) {
		*all_fields[i].fid = (*jenv)->GetFieldID(jenv,
		    *all_fields[i].cl, all_fields[i].name, all_fields[i].sig);

		if (*all_fields[i].fid == NULL) {
			fprintf(stderr,
			    "Failed to look up field %s with sig %s\n",
			    all_fields[i].name, all_fields[i].sig);
			return;
		}
	}

	/* Get method IDs */
	for (i = 0; i < NELEM(all_methods); i++) {
		*all_methods[i].mid = (*jenv)->GetMethodID(jenv,
		    *all_methods[i].cl, all_methods[i].name,
		    all_methods[i].sig);

		if (*all_methods[i].mid == NULL) {
			for (j = 0; j < NELEM(all_classes); j++)
				if (all_methods[i].cl == all_classes[j].cl)
					break;
			fprintf(stderr,
			    "Failed to look up method %s.%s with sig %s\n",
			    all_classes[j].name, all_methods[i].name,
			    all_methods[i].sig);
			return;
		}
	}
}

static JNIEnv *__dbj_get_jnienv(int *needDetach)
{
	/*
	 * Note: Different versions of the JNI disagree on the signature for
	 * AttachCurrentThreadAsDaemon.  The most recent documentation seems to
	 * say that (JNIEnv **) is correct, but newer JNIs seem to use
	 * (void **), oddly enough.
	 */
#ifdef JNI_VERSION_1_2
	void *jenv = 0;
#else
	JNIEnv *jenv = 0;
#endif

	*needDetach = 0;
	if ((*javavm)->GetEnv(javavm, &jenv, JNI_VERSION_1_2) == JNI_OK)
		return ((JNIEnv *)jenv);

	/*
	 * This should always succeed, as we are called via some Java activity.
	 * I think therefore I am (a thread).
	 */
	if ((*javavm)->AttachCurrentThread(javavm, &jenv, 0) != 0)
		return (0);

        *needDetach = 1;
	return ((JNIEnv *)jenv);
}

static void __dbj_detach()
{
	(void)(*javavm)->DetachCurrentThread(javavm);
}

static jobject __dbj_wrap_DB_LSN(JNIEnv *jenv, DB_LSN *lsn)
{
	return (*jenv)->NewObject(jenv, dblsn_class, dblsn_construct,
	    lsn->file, lsn->offset);
}
%}
