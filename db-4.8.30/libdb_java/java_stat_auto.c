/*-
 * Automatically built by dist/s_java_stat.
 * Only the javadoc comments can be edited.
 *
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 */
static int __dbj_fill_bt_stat(JNIEnv *jnienv,
    jobject jobj, struct __db_bt_stat *statp) {
	JAVADB_STAT_INT(jnienv, jobj, bt_stat_bt_magic_fid, statp, bt_magic);
	JAVADB_STAT_INT(jnienv, jobj, bt_stat_bt_version_fid, statp, bt_version);
	JAVADB_STAT_INT(jnienv, jobj, bt_stat_bt_metaflags_fid, statp, bt_metaflags);
	JAVADB_STAT_INT(jnienv, jobj, bt_stat_bt_nkeys_fid, statp, bt_nkeys);
	JAVADB_STAT_INT(jnienv, jobj, bt_stat_bt_ndata_fid, statp, bt_ndata);
	JAVADB_STAT_INT(jnienv, jobj, bt_stat_bt_pagecnt_fid, statp, bt_pagecnt);
	JAVADB_STAT_INT(jnienv, jobj, bt_stat_bt_pagesize_fid, statp, bt_pagesize);
	JAVADB_STAT_INT(jnienv, jobj, bt_stat_bt_minkey_fid, statp, bt_minkey);
	JAVADB_STAT_INT(jnienv, jobj, bt_stat_bt_re_len_fid, statp, bt_re_len);
	JAVADB_STAT_INT(jnienv, jobj, bt_stat_bt_re_pad_fid, statp, bt_re_pad);
	JAVADB_STAT_INT(jnienv, jobj, bt_stat_bt_levels_fid, statp, bt_levels);
	JAVADB_STAT_INT(jnienv, jobj, bt_stat_bt_int_pg_fid, statp, bt_int_pg);
	JAVADB_STAT_INT(jnienv, jobj, bt_stat_bt_leaf_pg_fid, statp, bt_leaf_pg);
	JAVADB_STAT_INT(jnienv, jobj, bt_stat_bt_dup_pg_fid, statp, bt_dup_pg);
	JAVADB_STAT_INT(jnienv, jobj, bt_stat_bt_over_pg_fid, statp, bt_over_pg);
	JAVADB_STAT_INT(jnienv, jobj, bt_stat_bt_empty_pg_fid, statp, bt_empty_pg);
	JAVADB_STAT_INT(jnienv, jobj, bt_stat_bt_free_fid, statp, bt_free);
	JAVADB_STAT_LONG(jnienv, jobj, bt_stat_bt_int_pgfree_fid, statp, bt_int_pgfree);
	JAVADB_STAT_LONG(jnienv, jobj, bt_stat_bt_leaf_pgfree_fid, statp, bt_leaf_pgfree);
	JAVADB_STAT_LONG(jnienv, jobj, bt_stat_bt_dup_pgfree_fid, statp, bt_dup_pgfree);
	JAVADB_STAT_LONG(jnienv, jobj, bt_stat_bt_over_pgfree_fid, statp, bt_over_pgfree);
	return (0);
}
static int __dbj_fill_compact(JNIEnv *jnienv,
    jobject jobj, struct __db_compact *statp) {
	JAVADB_STAT_INT(jnienv, jobj, compact_compact_fillpercent_fid, statp, compact_fillpercent);
	JAVADB_STAT_INT(jnienv, jobj, compact_compact_timeout_fid, statp, compact_timeout);
	JAVADB_STAT_INT(jnienv, jobj, compact_compact_pages_fid, statp, compact_pages);
	JAVADB_STAT_INT(jnienv, jobj, compact_compact_pages_free_fid, statp, compact_pages_free);
	JAVADB_STAT_INT(jnienv, jobj, compact_compact_pages_examine_fid, statp, compact_pages_examine);
	JAVADB_STAT_INT(jnienv, jobj, compact_compact_levels_fid, statp, compact_levels);
	JAVADB_STAT_INT(jnienv, jobj, compact_compact_deadlock_fid, statp, compact_deadlock);
	JAVADB_STAT_INT(jnienv, jobj, compact_compact_pages_truncated_fid, statp, compact_pages_truncated);
	JAVADB_STAT_INT(jnienv, jobj, compact_compact_truncate_fid, statp, compact_truncate);
	return (0);
}
static int __dbj_fill_h_stat(JNIEnv *jnienv,
    jobject jobj, struct __db_h_stat *statp) {
	JAVADB_STAT_INT(jnienv, jobj, h_stat_hash_magic_fid, statp, hash_magic);
	JAVADB_STAT_INT(jnienv, jobj, h_stat_hash_version_fid, statp, hash_version);
	JAVADB_STAT_INT(jnienv, jobj, h_stat_hash_metaflags_fid, statp, hash_metaflags);
	JAVADB_STAT_INT(jnienv, jobj, h_stat_hash_nkeys_fid, statp, hash_nkeys);
	JAVADB_STAT_INT(jnienv, jobj, h_stat_hash_ndata_fid, statp, hash_ndata);
	JAVADB_STAT_INT(jnienv, jobj, h_stat_hash_pagecnt_fid, statp, hash_pagecnt);
	JAVADB_STAT_INT(jnienv, jobj, h_stat_hash_pagesize_fid, statp, hash_pagesize);
	JAVADB_STAT_INT(jnienv, jobj, h_stat_hash_ffactor_fid, statp, hash_ffactor);
	JAVADB_STAT_INT(jnienv, jobj, h_stat_hash_buckets_fid, statp, hash_buckets);
	JAVADB_STAT_INT(jnienv, jobj, h_stat_hash_free_fid, statp, hash_free);
	JAVADB_STAT_LONG(jnienv, jobj, h_stat_hash_bfree_fid, statp, hash_bfree);
	JAVADB_STAT_INT(jnienv, jobj, h_stat_hash_bigpages_fid, statp, hash_bigpages);
	JAVADB_STAT_LONG(jnienv, jobj, h_stat_hash_big_bfree_fid, statp, hash_big_bfree);
	JAVADB_STAT_INT(jnienv, jobj, h_stat_hash_overflows_fid, statp, hash_overflows);
	JAVADB_STAT_LONG(jnienv, jobj, h_stat_hash_ovfl_free_fid, statp, hash_ovfl_free);
	JAVADB_STAT_INT(jnienv, jobj, h_stat_hash_dup_fid, statp, hash_dup);
	JAVADB_STAT_LONG(jnienv, jobj, h_stat_hash_dup_free_fid, statp, hash_dup_free);
	return (0);
}
static int __dbj_fill_lock_stat(JNIEnv *jnienv,
    jobject jobj, struct __db_lock_stat *statp) {
	JAVADB_STAT_INT(jnienv, jobj, lock_stat_st_id_fid, statp, st_id);
	JAVADB_STAT_INT(jnienv, jobj, lock_stat_st_cur_maxid_fid, statp, st_cur_maxid);
	JAVADB_STAT_INT(jnienv, jobj, lock_stat_st_maxlocks_fid, statp, st_maxlocks);
	JAVADB_STAT_INT(jnienv, jobj, lock_stat_st_maxlockers_fid, statp, st_maxlockers);
	JAVADB_STAT_INT(jnienv, jobj, lock_stat_st_maxobjects_fid, statp, st_maxobjects);
	JAVADB_STAT_INT(jnienv, jobj, lock_stat_st_partitions_fid, statp, st_partitions);
	JAVADB_STAT_INT(jnienv, jobj, lock_stat_st_nmodes_fid, statp, st_nmodes);
	JAVADB_STAT_INT(jnienv, jobj, lock_stat_st_nlockers_fid, statp, st_nlockers);
	JAVADB_STAT_INT(jnienv, jobj, lock_stat_st_nlocks_fid, statp, st_nlocks);
	JAVADB_STAT_INT(jnienv, jobj, lock_stat_st_maxnlocks_fid, statp, st_maxnlocks);
	JAVADB_STAT_INT(jnienv, jobj, lock_stat_st_maxhlocks_fid, statp, st_maxhlocks);
	JAVADB_STAT_LONG(jnienv, jobj, lock_stat_st_locksteals_fid, statp, st_locksteals);
	JAVADB_STAT_LONG(jnienv, jobj, lock_stat_st_maxlsteals_fid, statp, st_maxlsteals);
	JAVADB_STAT_INT(jnienv, jobj, lock_stat_st_maxnlockers_fid, statp, st_maxnlockers);
	JAVADB_STAT_INT(jnienv, jobj, lock_stat_st_nobjects_fid, statp, st_nobjects);
	JAVADB_STAT_INT(jnienv, jobj, lock_stat_st_maxnobjects_fid, statp, st_maxnobjects);
	JAVADB_STAT_INT(jnienv, jobj, lock_stat_st_maxhobjects_fid, statp, st_maxhobjects);
	JAVADB_STAT_LONG(jnienv, jobj, lock_stat_st_objectsteals_fid, statp, st_objectsteals);
	JAVADB_STAT_LONG(jnienv, jobj, lock_stat_st_maxosteals_fid, statp, st_maxosteals);
	JAVADB_STAT_LONG(jnienv, jobj, lock_stat_st_nrequests_fid, statp, st_nrequests);
	JAVADB_STAT_LONG(jnienv, jobj, lock_stat_st_nreleases_fid, statp, st_nreleases);
	JAVADB_STAT_LONG(jnienv, jobj, lock_stat_st_nupgrade_fid, statp, st_nupgrade);
	JAVADB_STAT_LONG(jnienv, jobj, lock_stat_st_ndowngrade_fid, statp, st_ndowngrade);
	JAVADB_STAT_LONG(jnienv, jobj, lock_stat_st_lock_wait_fid, statp, st_lock_wait);
	JAVADB_STAT_LONG(jnienv, jobj, lock_stat_st_lock_nowait_fid, statp, st_lock_nowait);
	JAVADB_STAT_LONG(jnienv, jobj, lock_stat_st_ndeadlocks_fid, statp, st_ndeadlocks);
	JAVADB_STAT_INT(jnienv, jobj, lock_stat_st_locktimeout_fid, statp, st_locktimeout);
	JAVADB_STAT_LONG(jnienv, jobj, lock_stat_st_nlocktimeouts_fid, statp, st_nlocktimeouts);
	JAVADB_STAT_INT(jnienv, jobj, lock_stat_st_txntimeout_fid, statp, st_txntimeout);
	JAVADB_STAT_LONG(jnienv, jobj, lock_stat_st_ntxntimeouts_fid, statp, st_ntxntimeouts);
	JAVADB_STAT_LONG(jnienv, jobj, lock_stat_st_part_wait_fid, statp, st_part_wait);
	JAVADB_STAT_LONG(jnienv, jobj, lock_stat_st_part_nowait_fid, statp, st_part_nowait);
	JAVADB_STAT_LONG(jnienv, jobj, lock_stat_st_part_max_wait_fid, statp, st_part_max_wait);
	JAVADB_STAT_LONG(jnienv, jobj, lock_stat_st_part_max_nowait_fid, statp, st_part_max_nowait);
	JAVADB_STAT_LONG(jnienv, jobj, lock_stat_st_objs_wait_fid, statp, st_objs_wait);
	JAVADB_STAT_LONG(jnienv, jobj, lock_stat_st_objs_nowait_fid, statp, st_objs_nowait);
	JAVADB_STAT_LONG(jnienv, jobj, lock_stat_st_lockers_wait_fid, statp, st_lockers_wait);
	JAVADB_STAT_LONG(jnienv, jobj, lock_stat_st_lockers_nowait_fid, statp, st_lockers_nowait);
	JAVADB_STAT_LONG(jnienv, jobj, lock_stat_st_region_wait_fid, statp, st_region_wait);
	JAVADB_STAT_LONG(jnienv, jobj, lock_stat_st_region_nowait_fid, statp, st_region_nowait);
	JAVADB_STAT_INT(jnienv, jobj, lock_stat_st_hash_len_fid, statp, st_hash_len);
	JAVADB_STAT_INT(jnienv, jobj, lock_stat_st_regsize_fid, statp, st_regsize);
	return (0);
}
static int __dbj_fill_log_stat(JNIEnv *jnienv,
    jobject jobj, struct __db_log_stat *statp) {
	JAVADB_STAT_INT(jnienv, jobj, log_stat_st_magic_fid, statp, st_magic);
	JAVADB_STAT_INT(jnienv, jobj, log_stat_st_version_fid, statp, st_version);
	JAVADB_STAT_INT(jnienv, jobj, log_stat_st_mode_fid, statp, st_mode);
	JAVADB_STAT_INT(jnienv, jobj, log_stat_st_lg_bsize_fid, statp, st_lg_bsize);
	JAVADB_STAT_INT(jnienv, jobj, log_stat_st_lg_size_fid, statp, st_lg_size);
	JAVADB_STAT_INT(jnienv, jobj, log_stat_st_wc_bytes_fid, statp, st_wc_bytes);
	JAVADB_STAT_INT(jnienv, jobj, log_stat_st_wc_mbytes_fid, statp, st_wc_mbytes);
	JAVADB_STAT_LONG(jnienv, jobj, log_stat_st_record_fid, statp, st_record);
	JAVADB_STAT_INT(jnienv, jobj, log_stat_st_w_bytes_fid, statp, st_w_bytes);
	JAVADB_STAT_INT(jnienv, jobj, log_stat_st_w_mbytes_fid, statp, st_w_mbytes);
	JAVADB_STAT_LONG(jnienv, jobj, log_stat_st_wcount_fid, statp, st_wcount);
	JAVADB_STAT_LONG(jnienv, jobj, log_stat_st_wcount_fill_fid, statp, st_wcount_fill);
	JAVADB_STAT_LONG(jnienv, jobj, log_stat_st_rcount_fid, statp, st_rcount);
	JAVADB_STAT_LONG(jnienv, jobj, log_stat_st_scount_fid, statp, st_scount);
	JAVADB_STAT_LONG(jnienv, jobj, log_stat_st_region_wait_fid, statp, st_region_wait);
	JAVADB_STAT_LONG(jnienv, jobj, log_stat_st_region_nowait_fid, statp, st_region_nowait);
	JAVADB_STAT_INT(jnienv, jobj, log_stat_st_cur_file_fid, statp, st_cur_file);
	JAVADB_STAT_INT(jnienv, jobj, log_stat_st_cur_offset_fid, statp, st_cur_offset);
	JAVADB_STAT_INT(jnienv, jobj, log_stat_st_disk_file_fid, statp, st_disk_file);
	JAVADB_STAT_INT(jnienv, jobj, log_stat_st_disk_offset_fid, statp, st_disk_offset);
	JAVADB_STAT_INT(jnienv, jobj, log_stat_st_maxcommitperflush_fid, statp, st_maxcommitperflush);
	JAVADB_STAT_INT(jnienv, jobj, log_stat_st_mincommitperflush_fid, statp, st_mincommitperflush);
	JAVADB_STAT_INT(jnienv, jobj, log_stat_st_regsize_fid, statp, st_regsize);
	return (0);
}
static int __dbj_fill_mpool_fstat(JNIEnv *jnienv,
    jobject jobj, struct __db_mpool_fstat *statp) {
	JAVADB_STAT_STRING(jnienv, jobj, mpool_fstat_file_name_fid, statp, file_name);
	JAVADB_STAT_INT(jnienv, jobj, mpool_fstat_st_pagesize_fid, statp, st_pagesize);
	JAVADB_STAT_INT(jnienv, jobj, mpool_fstat_st_map_fid, statp, st_map);
	JAVADB_STAT_LONG(jnienv, jobj, mpool_fstat_st_cache_hit_fid, statp, st_cache_hit);
	JAVADB_STAT_LONG(jnienv, jobj, mpool_fstat_st_cache_miss_fid, statp, st_cache_miss);
	JAVADB_STAT_LONG(jnienv, jobj, mpool_fstat_st_page_create_fid, statp, st_page_create);
	JAVADB_STAT_LONG(jnienv, jobj, mpool_fstat_st_page_in_fid, statp, st_page_in);
	JAVADB_STAT_LONG(jnienv, jobj, mpool_fstat_st_page_out_fid, statp, st_page_out);
	return (0);
}
static int __dbj_fill_mpool_stat(JNIEnv *jnienv,
    jobject jobj, struct __db_mpool_stat *statp) {
	JAVADB_STAT_INT(jnienv, jobj, mpool_stat_st_gbytes_fid, statp, st_gbytes);
	JAVADB_STAT_INT(jnienv, jobj, mpool_stat_st_bytes_fid, statp, st_bytes);
	JAVADB_STAT_INT(jnienv, jobj, mpool_stat_st_ncache_fid, statp, st_ncache);
	JAVADB_STAT_INT(jnienv, jobj, mpool_stat_st_max_ncache_fid, statp, st_max_ncache);
	JAVADB_STAT_INT(jnienv, jobj, mpool_stat_st_mmapsize_fid, statp, st_mmapsize);
	JAVADB_STAT_INT(jnienv, jobj, mpool_stat_st_maxopenfd_fid, statp, st_maxopenfd);
	JAVADB_STAT_INT(jnienv, jobj, mpool_stat_st_maxwrite_fid, statp, st_maxwrite);
	JAVADB_STAT_INT(jnienv, jobj, mpool_stat_st_maxwrite_sleep_fid, statp, st_maxwrite_sleep);
	JAVADB_STAT_INT(jnienv, jobj, mpool_stat_st_pages_fid, statp, st_pages);
	JAVADB_STAT_INT(jnienv, jobj, mpool_stat_st_map_fid, statp, st_map);
	JAVADB_STAT_LONG(jnienv, jobj, mpool_stat_st_cache_hit_fid, statp, st_cache_hit);
	JAVADB_STAT_LONG(jnienv, jobj, mpool_stat_st_cache_miss_fid, statp, st_cache_miss);
	JAVADB_STAT_LONG(jnienv, jobj, mpool_stat_st_page_create_fid, statp, st_page_create);
	JAVADB_STAT_LONG(jnienv, jobj, mpool_stat_st_page_in_fid, statp, st_page_in);
	JAVADB_STAT_LONG(jnienv, jobj, mpool_stat_st_page_out_fid, statp, st_page_out);
	JAVADB_STAT_LONG(jnienv, jobj, mpool_stat_st_ro_evict_fid, statp, st_ro_evict);
	JAVADB_STAT_LONG(jnienv, jobj, mpool_stat_st_rw_evict_fid, statp, st_rw_evict);
	JAVADB_STAT_LONG(jnienv, jobj, mpool_stat_st_page_trickle_fid, statp, st_page_trickle);
	JAVADB_STAT_INT(jnienv, jobj, mpool_stat_st_page_clean_fid, statp, st_page_clean);
	JAVADB_STAT_INT(jnienv, jobj, mpool_stat_st_page_dirty_fid, statp, st_page_dirty);
	JAVADB_STAT_INT(jnienv, jobj, mpool_stat_st_hash_buckets_fid, statp, st_hash_buckets);
	JAVADB_STAT_INT(jnienv, jobj, mpool_stat_st_pagesize_fid, statp, st_pagesize);
	JAVADB_STAT_INT(jnienv, jobj, mpool_stat_st_hash_searches_fid, statp, st_hash_searches);
	JAVADB_STAT_INT(jnienv, jobj, mpool_stat_st_hash_longest_fid, statp, st_hash_longest);
	JAVADB_STAT_LONG(jnienv, jobj, mpool_stat_st_hash_examined_fid, statp, st_hash_examined);
	JAVADB_STAT_LONG(jnienv, jobj, mpool_stat_st_hash_nowait_fid, statp, st_hash_nowait);
	JAVADB_STAT_LONG(jnienv, jobj, mpool_stat_st_hash_wait_fid, statp, st_hash_wait);
	JAVADB_STAT_LONG(jnienv, jobj, mpool_stat_st_hash_max_nowait_fid, statp, st_hash_max_nowait);
	JAVADB_STAT_LONG(jnienv, jobj, mpool_stat_st_hash_max_wait_fid, statp, st_hash_max_wait);
	JAVADB_STAT_LONG(jnienv, jobj, mpool_stat_st_region_nowait_fid, statp, st_region_nowait);
	JAVADB_STAT_LONG(jnienv, jobj, mpool_stat_st_region_wait_fid, statp, st_region_wait);
	JAVADB_STAT_LONG(jnienv, jobj, mpool_stat_st_mvcc_frozen_fid, statp, st_mvcc_frozen);
	JAVADB_STAT_LONG(jnienv, jobj, mpool_stat_st_mvcc_thawed_fid, statp, st_mvcc_thawed);
	JAVADB_STAT_LONG(jnienv, jobj, mpool_stat_st_mvcc_freed_fid, statp, st_mvcc_freed);
	JAVADB_STAT_LONG(jnienv, jobj, mpool_stat_st_alloc_fid, statp, st_alloc);
	JAVADB_STAT_LONG(jnienv, jobj, mpool_stat_st_alloc_buckets_fid, statp, st_alloc_buckets);
	JAVADB_STAT_LONG(jnienv, jobj, mpool_stat_st_alloc_max_buckets_fid, statp, st_alloc_max_buckets);
	JAVADB_STAT_LONG(jnienv, jobj, mpool_stat_st_alloc_pages_fid, statp, st_alloc_pages);
	JAVADB_STAT_LONG(jnienv, jobj, mpool_stat_st_alloc_max_pages_fid, statp, st_alloc_max_pages);
	JAVADB_STAT_LONG(jnienv, jobj, mpool_stat_st_io_wait_fid, statp, st_io_wait);
	JAVADB_STAT_LONG(jnienv, jobj, mpool_stat_st_sync_interrupted_fid, statp, st_sync_interrupted);
	JAVADB_STAT_INT(jnienv, jobj, mpool_stat_st_regsize_fid, statp, st_regsize);
	return (0);
}
static int __dbj_fill_mutex_stat(JNIEnv *jnienv,
    jobject jobj, struct __db_mutex_stat *statp) {
	JAVADB_STAT_INT(jnienv, jobj, mutex_stat_st_mutex_align_fid, statp, st_mutex_align);
	JAVADB_STAT_INT(jnienv, jobj, mutex_stat_st_mutex_tas_spins_fid, statp, st_mutex_tas_spins);
	JAVADB_STAT_INT(jnienv, jobj, mutex_stat_st_mutex_cnt_fid, statp, st_mutex_cnt);
	JAVADB_STAT_INT(jnienv, jobj, mutex_stat_st_mutex_free_fid, statp, st_mutex_free);
	JAVADB_STAT_INT(jnienv, jobj, mutex_stat_st_mutex_inuse_fid, statp, st_mutex_inuse);
	JAVADB_STAT_INT(jnienv, jobj, mutex_stat_st_mutex_inuse_max_fid, statp, st_mutex_inuse_max);
	JAVADB_STAT_LONG(jnienv, jobj, mutex_stat_st_region_wait_fid, statp, st_region_wait);
	JAVADB_STAT_LONG(jnienv, jobj, mutex_stat_st_region_nowait_fid, statp, st_region_nowait);
	JAVADB_STAT_INT(jnienv, jobj, mutex_stat_st_regsize_fid, statp, st_regsize);
	return (0);
}
static int __dbj_fill_qam_stat(JNIEnv *jnienv,
    jobject jobj, struct __db_qam_stat *statp) {
	JAVADB_STAT_INT(jnienv, jobj, qam_stat_qs_magic_fid, statp, qs_magic);
	JAVADB_STAT_INT(jnienv, jobj, qam_stat_qs_version_fid, statp, qs_version);
	JAVADB_STAT_INT(jnienv, jobj, qam_stat_qs_metaflags_fid, statp, qs_metaflags);
	JAVADB_STAT_INT(jnienv, jobj, qam_stat_qs_nkeys_fid, statp, qs_nkeys);
	JAVADB_STAT_INT(jnienv, jobj, qam_stat_qs_ndata_fid, statp, qs_ndata);
	JAVADB_STAT_INT(jnienv, jobj, qam_stat_qs_pagesize_fid, statp, qs_pagesize);
	JAVADB_STAT_INT(jnienv, jobj, qam_stat_qs_extentsize_fid, statp, qs_extentsize);
	JAVADB_STAT_INT(jnienv, jobj, qam_stat_qs_pages_fid, statp, qs_pages);
	JAVADB_STAT_INT(jnienv, jobj, qam_stat_qs_re_len_fid, statp, qs_re_len);
	JAVADB_STAT_INT(jnienv, jobj, qam_stat_qs_re_pad_fid, statp, qs_re_pad);
	JAVADB_STAT_INT(jnienv, jobj, qam_stat_qs_pgfree_fid, statp, qs_pgfree);
	JAVADB_STAT_INT(jnienv, jobj, qam_stat_qs_first_recno_fid, statp, qs_first_recno);
	JAVADB_STAT_INT(jnienv, jobj, qam_stat_qs_cur_recno_fid, statp, qs_cur_recno);
	return (0);
}
static int __dbj_fill_rep_stat(JNIEnv *jnienv,
    jobject jobj, struct __db_rep_stat *statp) {
	JAVADB_STAT_LONG(jnienv, jobj, rep_stat_st_log_queued_fid, statp, st_log_queued);
	JAVADB_STAT_INT(jnienv, jobj, rep_stat_st_startup_complete_fid, statp, st_startup_complete);
	JAVADB_STAT_INT(jnienv, jobj, rep_stat_st_status_fid, statp, st_status);
	JAVADB_STAT_LSN(jnienv, jobj, rep_stat_st_next_lsn_fid, statp, st_next_lsn);
	JAVADB_STAT_LSN(jnienv, jobj, rep_stat_st_waiting_lsn_fid, statp, st_waiting_lsn);
	JAVADB_STAT_LSN(jnienv, jobj, rep_stat_st_max_perm_lsn_fid, statp, st_max_perm_lsn);
	JAVADB_STAT_INT(jnienv, jobj, rep_stat_st_next_pg_fid, statp, st_next_pg);
	JAVADB_STAT_INT(jnienv, jobj, rep_stat_st_waiting_pg_fid, statp, st_waiting_pg);
	JAVADB_STAT_INT(jnienv, jobj, rep_stat_st_dupmasters_fid, statp, st_dupmasters);
	JAVADB_STAT_INT(jnienv, jobj, rep_stat_st_env_id_fid, statp, st_env_id);
	JAVADB_STAT_INT(jnienv, jobj, rep_stat_st_env_priority_fid, statp, st_env_priority);
	JAVADB_STAT_LONG(jnienv, jobj, rep_stat_st_bulk_fills_fid, statp, st_bulk_fills);
	JAVADB_STAT_LONG(jnienv, jobj, rep_stat_st_bulk_overflows_fid, statp, st_bulk_overflows);
	JAVADB_STAT_LONG(jnienv, jobj, rep_stat_st_bulk_records_fid, statp, st_bulk_records);
	JAVADB_STAT_LONG(jnienv, jobj, rep_stat_st_bulk_transfers_fid, statp, st_bulk_transfers);
	JAVADB_STAT_LONG(jnienv, jobj, rep_stat_st_client_rerequests_fid, statp, st_client_rerequests);
	JAVADB_STAT_LONG(jnienv, jobj, rep_stat_st_client_svc_req_fid, statp, st_client_svc_req);
	JAVADB_STAT_LONG(jnienv, jobj, rep_stat_st_client_svc_miss_fid, statp, st_client_svc_miss);
	JAVADB_STAT_INT(jnienv, jobj, rep_stat_st_gen_fid, statp, st_gen);
	JAVADB_STAT_INT(jnienv, jobj, rep_stat_st_egen_fid, statp, st_egen);
	JAVADB_STAT_LONG(jnienv, jobj, rep_stat_st_log_duplicated_fid, statp, st_log_duplicated);
	JAVADB_STAT_LONG(jnienv, jobj, rep_stat_st_log_queued_max_fid, statp, st_log_queued_max);
	JAVADB_STAT_LONG(jnienv, jobj, rep_stat_st_log_queued_total_fid, statp, st_log_queued_total);
	JAVADB_STAT_LONG(jnienv, jobj, rep_stat_st_log_records_fid, statp, st_log_records);
	JAVADB_STAT_LONG(jnienv, jobj, rep_stat_st_log_requested_fid, statp, st_log_requested);
	JAVADB_STAT_INT(jnienv, jobj, rep_stat_st_master_fid, statp, st_master);
	JAVADB_STAT_LONG(jnienv, jobj, rep_stat_st_master_changes_fid, statp, st_master_changes);
	JAVADB_STAT_LONG(jnienv, jobj, rep_stat_st_msgs_badgen_fid, statp, st_msgs_badgen);
	JAVADB_STAT_LONG(jnienv, jobj, rep_stat_st_msgs_processed_fid, statp, st_msgs_processed);
	JAVADB_STAT_LONG(jnienv, jobj, rep_stat_st_msgs_recover_fid, statp, st_msgs_recover);
	JAVADB_STAT_LONG(jnienv, jobj, rep_stat_st_msgs_send_failures_fid, statp, st_msgs_send_failures);
	JAVADB_STAT_LONG(jnienv, jobj, rep_stat_st_msgs_sent_fid, statp, st_msgs_sent);
	JAVADB_STAT_LONG(jnienv, jobj, rep_stat_st_newsites_fid, statp, st_newsites);
	JAVADB_STAT_INT(jnienv, jobj, rep_stat_st_nsites_fid, statp, st_nsites);
	JAVADB_STAT_LONG(jnienv, jobj, rep_stat_st_nthrottles_fid, statp, st_nthrottles);
	JAVADB_STAT_LONG(jnienv, jobj, rep_stat_st_outdated_fid, statp, st_outdated);
	JAVADB_STAT_LONG(jnienv, jobj, rep_stat_st_pg_duplicated_fid, statp, st_pg_duplicated);
	JAVADB_STAT_LONG(jnienv, jobj, rep_stat_st_pg_records_fid, statp, st_pg_records);
	JAVADB_STAT_LONG(jnienv, jobj, rep_stat_st_pg_requested_fid, statp, st_pg_requested);
	JAVADB_STAT_LONG(jnienv, jobj, rep_stat_st_txns_applied_fid, statp, st_txns_applied);
	JAVADB_STAT_LONG(jnienv, jobj, rep_stat_st_startsync_delayed_fid, statp, st_startsync_delayed);
	JAVADB_STAT_LONG(jnienv, jobj, rep_stat_st_elections_fid, statp, st_elections);
	JAVADB_STAT_LONG(jnienv, jobj, rep_stat_st_elections_won_fid, statp, st_elections_won);
	JAVADB_STAT_INT(jnienv, jobj, rep_stat_st_election_cur_winner_fid, statp, st_election_cur_winner);
	JAVADB_STAT_INT(jnienv, jobj, rep_stat_st_election_gen_fid, statp, st_election_gen);
	JAVADB_STAT_LSN(jnienv, jobj, rep_stat_st_election_lsn_fid, statp, st_election_lsn);
	JAVADB_STAT_INT(jnienv, jobj, rep_stat_st_election_nsites_fid, statp, st_election_nsites);
	JAVADB_STAT_INT(jnienv, jobj, rep_stat_st_election_nvotes_fid, statp, st_election_nvotes);
	JAVADB_STAT_INT(jnienv, jobj, rep_stat_st_election_priority_fid, statp, st_election_priority);
	JAVADB_STAT_INT(jnienv, jobj, rep_stat_st_election_status_fid, statp, st_election_status);
	JAVADB_STAT_INT(jnienv, jobj, rep_stat_st_election_tiebreaker_fid, statp, st_election_tiebreaker);
	JAVADB_STAT_INT(jnienv, jobj, rep_stat_st_election_votes_fid, statp, st_election_votes);
	JAVADB_STAT_INT(jnienv, jobj, rep_stat_st_election_sec_fid, statp, st_election_sec);
	JAVADB_STAT_INT(jnienv, jobj, rep_stat_st_election_usec_fid, statp, st_election_usec);
	JAVADB_STAT_INT(jnienv, jobj, rep_stat_st_max_lease_sec_fid, statp, st_max_lease_sec);
	JAVADB_STAT_INT(jnienv, jobj, rep_stat_st_max_lease_usec_fid, statp, st_max_lease_usec);
	return (0);
}
static int __dbj_fill_repmgr_stat(JNIEnv *jnienv,
    jobject jobj, struct __db_repmgr_stat *statp) {
	JAVADB_STAT_LONG(jnienv, jobj, repmgr_stat_st_perm_failed_fid, statp, st_perm_failed);
	JAVADB_STAT_LONG(jnienv, jobj, repmgr_stat_st_msgs_queued_fid, statp, st_msgs_queued);
	JAVADB_STAT_LONG(jnienv, jobj, repmgr_stat_st_msgs_dropped_fid, statp, st_msgs_dropped);
	JAVADB_STAT_LONG(jnienv, jobj, repmgr_stat_st_connection_drop_fid, statp, st_connection_drop);
	JAVADB_STAT_LONG(jnienv, jobj, repmgr_stat_st_connect_fail_fid, statp, st_connect_fail);
	return (0);
}
static int __dbj_fill_seq_stat(JNIEnv *jnienv,
    jobject jobj, struct __db_seq_stat *statp) {
	JAVADB_STAT_LONG(jnienv, jobj, seq_stat_st_wait_fid, statp, st_wait);
	JAVADB_STAT_LONG(jnienv, jobj, seq_stat_st_nowait_fid, statp, st_nowait);
	JAVADB_STAT_LONG(jnienv, jobj, seq_stat_st_current_fid, statp, st_current);
	JAVADB_STAT_LONG(jnienv, jobj, seq_stat_st_value_fid, statp, st_value);
	JAVADB_STAT_LONG(jnienv, jobj, seq_stat_st_last_value_fid, statp, st_last_value);
	JAVADB_STAT_LONG(jnienv, jobj, seq_stat_st_min_fid, statp, st_min);
	JAVADB_STAT_LONG(jnienv, jobj, seq_stat_st_max_fid, statp, st_max);
	JAVADB_STAT_INT(jnienv, jobj, seq_stat_st_cache_size_fid, statp, st_cache_size);
	JAVADB_STAT_INT(jnienv, jobj, seq_stat_st_flags_fid, statp, st_flags);
	return (0);
}
static int __dbj_fill_txn_stat(JNIEnv *jnienv,
    jobject jobj, struct __db_txn_stat *statp) {
	JAVADB_STAT_INT(jnienv, jobj, txn_stat_st_nrestores_fid, statp, st_nrestores);
	JAVADB_STAT_LSN(jnienv, jobj, txn_stat_st_last_ckp_fid, statp, st_last_ckp);
	JAVADB_STAT_LONG(jnienv, jobj, txn_stat_st_time_ckp_fid, statp, st_time_ckp);
	JAVADB_STAT_INT(jnienv, jobj, txn_stat_st_last_txnid_fid, statp, st_last_txnid);
	JAVADB_STAT_INT(jnienv, jobj, txn_stat_st_maxtxns_fid, statp, st_maxtxns);
	JAVADB_STAT_LONG(jnienv, jobj, txn_stat_st_naborts_fid, statp, st_naborts);
	JAVADB_STAT_LONG(jnienv, jobj, txn_stat_st_nbegins_fid, statp, st_nbegins);
	JAVADB_STAT_LONG(jnienv, jobj, txn_stat_st_ncommits_fid, statp, st_ncommits);
	JAVADB_STAT_INT(jnienv, jobj, txn_stat_st_nactive_fid, statp, st_nactive);
	JAVADB_STAT_INT(jnienv, jobj, txn_stat_st_nsnapshot_fid, statp, st_nsnapshot);
	JAVADB_STAT_INT(jnienv, jobj, txn_stat_st_maxnactive_fid, statp, st_maxnactive);
	JAVADB_STAT_INT(jnienv, jobj, txn_stat_st_maxnsnapshot_fid, statp, st_maxnsnapshot);
	JAVADB_STAT_ACTIVE(jnienv, jobj, txn_stat_st_txnarray_fid, statp, st_txnarray);
	JAVADB_STAT_LONG(jnienv, jobj, txn_stat_st_region_wait_fid, statp, st_region_wait);
	JAVADB_STAT_LONG(jnienv, jobj, txn_stat_st_region_nowait_fid, statp, st_region_nowait);
	JAVADB_STAT_INT(jnienv, jobj, txn_stat_st_regsize_fid, statp, st_regsize);
	return (0);
}
static int __dbj_fill_txn_active(JNIEnv *jnienv,
    jobject jobj, struct __db_txn_active *statp) {
	JAVADB_STAT_INT(jnienv, jobj, txn_active_txnid_fid, statp, txnid);
	JAVADB_STAT_INT(jnienv, jobj, txn_active_parentid_fid, statp, parentid);
	JAVADB_STAT_INT(jnienv, jobj, txn_active_pid_fid, statp, pid);
	JAVADB_STAT_LSN(jnienv, jobj, txn_active_lsn_fid, statp, lsn);
	JAVADB_STAT_LSN(jnienv, jobj, txn_active_read_lsn_fid, statp, read_lsn);
	JAVADB_STAT_INT(jnienv, jobj, txn_active_mvcc_ref_fid, statp, mvcc_ref);
	JAVADB_STAT_INT(jnienv, jobj, txn_active_status_fid, statp, status);
	JAVADB_STAT_GID(jnienv, jobj, txn_active_gid_fid, statp, gid);
	JAVADB_STAT_STRING(jnienv, jobj, txn_active_name_fid, statp, name);
	return (0);
}
