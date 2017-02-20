# See the file LICENSE for redistribution information.
#
# Copyright (c)-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST    env017
# TEST    Check documented "stat" fields against the fields
# TEST    returned by the "stat" functions.  Make sure they
# TEST    match, and that none are missing.	
# TEST    These are the stat functions we test:
# TEST        env log_stat
# TEST        env lock_stat
# TEST        env txn_stat
# TEST        env mutex_stat
# TEST        env rep_stat
# TEST        env repmgr_stat
# TEST        env mpool_stat
# TEST        db_stat
# TEST        seq_stat
  

proc env017 { } {
	puts "\nEnv017: Check the integrity of the various stat"
	env017_log_stat
	env017_lock_stat
	env017_txn_stat
	env017_mutex_stat
	env017_rep_stat
	env017_repmgr_stat
	env017_mpool_stat
	env017_db_stat
	env017_seq_stat
}

# Check the log stat field.
proc env017_log_stat { } {
	puts "\nEnv017: Check the Log stat field"
	set check_type log_stat_check
	set stat_method log_stat
	set envargs {-create -log}
	set map_list {
		{ "Magic"	    st_magic }
		{ "Log file Version"	    st_version }
		{ "Region size"	    st_regsize }
		{ "Log file mode"	    st_mode }
		{ "Log record cache size"	    st_lg_bsize }
		{ "Current log file size"	    st_lg_size }
		{ "Log file records written"	    st_record }
		{ "Mbytes written"	    st_w_mbytes }
		{ "Bytes written (over Mb)"	    st_w_bytes }
		{ "Mbytes written since checkpoint"	    st_wc_mbytes }
		{ "Bytes written (over Mb) since checkpoint"
		    st_wc_bytes }
		{ "Times log written"	    st_wcount }
		{ "Times log written because cache filled up"
		    st_wcount_fill }
		{ "Times log read from disk"	    st_rcount }
		{ "Times log flushed to disk"	    st_scount }
		{ "Current log file number"	    st_cur_file }
		{ "Current log file offset"	    st_cur_offset }
		{ "On-disk log file number"	    st_disk_file }
		{ "On-disk log file offset"	    st_disk_offset }
		{ "Max commits in a log flush"	    st_maxcommitperflush }
		{ "Min commits in a log flush"	    st_mincommitperflush }
		{ "Number of region lock waits"	    st_region_wait }
		{ "Number of region lock nowaits"	    st_region_nowait }
	}
	set doc_list [list st_magic st_version st_mode st_lg_bsize st_lg_size \
	    st_record st_w_mbytes st_w_bytes st_wc_mbytes st_wc_bytes \
	    st_wcount st_wcount_fill st_rcount st_scount st_cur_file \
	    st_cur_offset st_disk_file st_disk_offset st_maxcommitperflush \
	    st_mincommitperflush st_regsize st_region_wait st_region_nowait ]
	env017_stat_check \
	    $map_list $doc_list $check_type $stat_method $envargs
}

# Check the lock stat field.
proc env017_lock_stat { } {
	puts "\nEnv017: Check the lock stat field"
	set check_type lock_stat_check
	set stat_method lock_stat
	set envargs {-create -lock}
	set map_list {
		{ "Region size"	    st_regsize }
		{ "Last allocated locker ID"	    st_id }
		{ "Current maximum unused locker ID"	    st_cur_maxid }
		{ "Maximum locks"	    st_maxlocks }
		{ "Maximum lockers"	    st_maxlockers }
		{ "Maximum objects"	    st_maxobjects }
		{ "Lock modes"	    st_nmodes }
		{ "Number of lock table partitions"	    st_partitions }
		{ "Current number of locks"	    st_nlocks }
		{ "Maximum number of locks so far"	    st_maxnlocks }
		{ "Maximum number of locks in any hash bucket"
    		    st_maxhlocks }
		{ "Maximum number of lock steals for an empty partition"
		    st_locksteals }
		{ "Maximum number lock steals in any partition"
		    st_maxlsteals }
		{ "Current number of lockers"	    st_nlockers }
		{ "Maximum number of lockers so far"	    st_maxnlockers }
		{ "Current number of objects"	    st_nobjects }
		{ "Maximum number of objects so far"	    st_maxnobjects }
		{ "Maximum number of objects in any hash bucket"
		    st_maxhobjects }
		{ "Maximum number of object steals for an empty partition"
		    st_objectsteals }
		{ "Maximum number object steals in any partition"
		    st_maxosteals }
		{ "Lock requests"	    st_nrequests }
		{ "Lock releases"	    st_nreleases }
		{ "Lock upgrades"	    st_nupgrade }
		{ "Lock downgrades"	    st_ndowngrade }
		{ "Number of conflicted locks for which we waited"
		    st_lock_wait }
		{ "Number of conflicted locks for which we did not wait"
		    st_lock_nowait }
		{ "Deadlocks detected"	    st_ndeadlocks }
		{ "Number of region lock waits"	    st_region_wait }
		{ "Number of region lock nowaits"	    st_region_nowait }
		{ "Number of object allocation waits"	    st_objs_wait }
		{ "Number of object allocation nowaits"	    st_objs_nowait }
		{ "Number of locker allocation waits"	    st_lockers_wait }
		{ "Number of locker allocation nowaits"	    st_lockers_nowait }
		{ "Maximum hash bucket length"	    st_hash_len }
		{ "Lock timeout value"	    st_locktimeout }
		{ "Number of lock timeouts"	    st_nlocktimeouts }
		{ "Transaction timeout value"	    st_txntimeout }
		{ "Number of transaction timeouts"	    st_ntxntimeouts }
		{ "Number lock partition mutex waits"	    st_part_wait }
		{ "Number lock partition mutex nowaits"	    st_part_nowait }
		{ "Maximum number waits on any lock partition mutex"
		    st_part_max_wait }
		{ "Maximum number nowaits on any lock partition mutex"
		    st_part_max_nowait }
	}
	set doc_list [list st_id st_cur_maxid st_nmodes st_maxlocks \
	    st_maxlockers st_maxobjects st_partitions st_nlocks st_maxnlocks \
	    st_maxhlocks st_locksteals st_maxlsteals st_nlockers \
	    st_maxnlockers st_nobjects st_maxnobjects st_maxhobjects \
	    st_objectsteals st_maxosteals st_nrequests st_nreleases st_nupgrade\
	    st_ndowngrade st_lock_wait st_lock_nowait st_ndeadlocks \
	    st_locktimeout st_nlocktimeouts st_txntimeout st_ntxntimeouts \
	    st_objs_wait st_objs_nowait st_lockers_wait st_lockers_nowait \
	    st_hash_len st_regsize st_part_wait st_part_nowait st_part_max_wait\
	    st_part_max_nowait st_region_wait st_region_nowait]
	env017_stat_check \
	    $map_list $doc_list $check_type $stat_method $envargs
}

# Check the txn stat field.
proc env017_txn_stat { } {
	puts "\nEnv017: Check the transaction stat field"
	set check_type txn_stat_check
	set stat_method txn_stat
	set envargs {-create -txn}
	set map_list {
		{ "Region size"	    st_regsize }
		{ "LSN of last checkpoint"	    st_last_ckp }
		{ "Time of last checkpoint"	    st_time_ckp }
		{ "Last txn ID allocated"	    st_last_txnid }
		{ "Maximum txns"	    st_maxtxns }
		{ "Number aborted txns"	    st_naborts }
		{ "Number txns begun"	    st_nbegins }
		{ "Number committed txns"	    st_ncommits }
		{ "Number active txns"	    st_nactive }
		{ "Number of snapshot txns"	    st_nsnapshot }
		{ "Number restored txns"	    st_nrestores }
		{ "Maximum active txns"	    st_maxnactive }
		{ "Maximum snapshot txns"	    st_maxnsnapshot }
		{ "Number of region lock waits"	    st_region_wait }
		{ "Number of region lock nowaits"	    st_region_nowait }
	}
	set doc_list [list  st_last_ckp st_time_ckp st_last_txnid st_maxtxns \
	    st_nactive st_nsnapshot st_maxnactive st_maxnsnapshot st_nbegins \
	    st_naborts st_ncommits st_nrestores st_regsize st_region_wait \
	    st_region_nowait ]
	env017_stat_check \
	    $map_list $doc_list $check_type $stat_method $envargs
}

#Check the mutex stat field.
proc env017_mutex_stat { } {
	puts "\nEnv017: Check the mutex stat field"
	set check_type mutex_stat_check
	set stat_method mutex_stat
	set envargs {-create}
	set map_list {
		{ "Mutex align"	    st_mutex_align }
		{ "Mutex TAS spins"	    st_mutex_tas_spins }
		{ "Mutex count"	    st_mutex_cnt }
		{ "Free mutexes"	    st_mutex_free }
		{ "Mutexes in use"	    st_mutex_inuse }
		{ "Max in use"	    st_mutex_inuse_max }
		{ "Mutex region size"	    st_regsize }
		{ "Number of region waits"	    st_region_wait }
		{ "Number of region no waits"	    st_region_nowait }
	}
	set doc_list [list st_mutex_align st_mutex_tas_spins st_mutex_cnt \
	    st_mutex_free st_mutex_inuse st_mutex_inuse_max st_regsize \
	    st_region_wait st_region_nowait ]

	env017_stat_check \
	    $map_list $doc_list $check_type $stat_method $envargs
}

# Check the rep stat field.
proc env017_rep_stat { } {
	puts "\nEnv017: Check the replication stat field"
	set check_type rep_stat_check
	set stat_method rep_stat
	set envargs {-create -rep -log -txn}	
	set map_list {
		{ "Role"	    st_status}
		{ "Next LSN expected"	    st_next_lsn }
		{ "First missed LSN"	    st_waiting_lsn }
		{ "Maximum permanent LSN"	    st_max_perm_lsn }
		{ "Bulk buffer fills"	    st_bulk_fills }
		{ "Bulk buffer overflows"	    st_bulk_overflows }
		{ "Bulk records stored"	    st_bulk_records }
		{ "Bulk buffer transfers"	    st_bulk_transfers }
		{ "Client service requests"	    st_client_svc_req }
		{ "Client service req misses"	    st_client_svc_miss }
		{ "Client rerequests"	    st_client_rerequests }
		{ "Duplicate master conditions"	    st_dupmasters }
		{ "Environment ID"	    st_env_id }
		{ "Environment priority"	    st_env_priority }
		{ "Generation number"	    st_gen }
		{ "Election generation number"	    st_egen }
		{ "Startup complete"	    st_startup_complete }
		{ "Duplicate log records received"	    st_log_duplicated }
		{ "Current log records queued"	    st_log_queued }
		{ "Maximum log records queued"	    st_log_queued_max }
		{ "Total log records queued"	    st_log_queued_total }
		{ "Log records received"	    st_log_records }
		{ "Log records requested"	    st_log_requested }
		{ "Master environment ID"	    st_master }
		{ "Master changes"	    st_master_changes }
		{ "Messages with bad generation number"	    st_msgs_badgen }
		{ "Messages processed"	    st_msgs_processed }
		{ "Messages ignored for recovery"	    st_msgs_recover }
		{ "Message send failures"	    st_msgs_send_failures }
		{ "Messages sent"	    st_msgs_sent }
		{ "New site messages"	    st_newsites }
		{ "Number of sites in replication group"	    st_nsites }
		{ "Transmission limited"	    st_nthrottles }
		{ "Outdated conditions"	    st_outdated }
		{ "Transactions applied"	    st_txns_applied }
		{ "Next page expected"	    st_next_pg }
		{ "First missed page"	    st_waiting_pg }
		{ "Duplicate pages received"	    st_pg_duplicated }
		{ "Pages received"	    st_pg_records }
		{ "Pages requested"	    st_pg_requested }
		{ "Elections held"	    st_elections }
		{ "Elections won"	    st_elections_won }
		{ "Election phase"	    st_election_status }
		{ "Election winner"	    st_election_cur_winner }
		{ "Election generation number"	    st_election_gen }
		{ "Election max LSN"	    st_election_lsn }
		{ "Election sites"	    st_election_nsites }
		{ "Election nvotes"	    st_election_nvotes }
		{ "Election priority"	    st_election_priority }
		{ "Election tiebreaker"	    st_election_tiebreaker }
		{ "Election votes"	    st_election_votes }
		{ "Election seconds"	    st_election_sec }
		{ "Election usecs"	    st_election_usec }
		{ "Start-sync operations delayed"
		    st_startsync_delayed }
		{ "Maximum lease seconds"	    st_max_lease_sec }
		{ "Maximum lease usecs"	    st_max_lease_usec }
		{ "File fail cleanups done"	st_filefail_cleanups }
	}
	set doc_list [list st_bulk_fills st_bulk_overflows st_bulk_records \
	    st_bulk_transfers st_client_rerequests st_client_svc_miss \
	    st_client_svc_req st_dupmasters st_egen st_election_cur_winner \
	    st_election_gen st_election_lsn st_election_nsites \
	    st_election_nvotes st_election_priority st_election_sec \
	    st_election_status st_election_tiebreaker st_election_usec \
	    st_election_votes st_elections st_elections_won st_env_id \
	    st_env_priority st_filefail_cleanups st_gen st_log_duplicated \
	    st_log_queued st_log_queued_max st_log_queued_total st_log_records \
	    st_log_requested st_master st_master_changes st_max_lease_sec \
	    st_max_lease_usec st_max_perm_lsn st_msgs_badgen st_msgs_processed\
	    st_msgs_recover st_msgs_send_failures st_msgs_sent st_newsites \
	    st_next_lsn st_next_pg st_nsites st_nthrottles st_outdated \
	    st_pg_duplicated st_pg_records st_pg_requested \
	    st_startsync_delayed st_startup_complete st_status st_txns_applied\
	    st_waiting_lsn st_waiting_pg ]
	env017_stat_check \
	    $map_list $doc_list $check_type $stat_method $envargs
}

# Check the repmgr stat field.
proc env017_repmgr_stat { } {
	puts "\nEnv017: Check the repmgr stat field"
	set check_type repmgr_stat_check
	set stat_method repmgr_stat
	set envargs {-create -txn -rep}
	set map_list {
		{ "Acknowledgement failures"	    st_perm_failed }
		{ "Messages delayed"	    st_msgs_queued}
		{ "Messages discarded"	    st_msgs_dropped}
		{ "Connections dropped"	    st_connection_drop}
		{ "Failed re-connects"	    st_connect_fail}
	}
	set doc_list [list st_perm_failed st_msgs_queued st_msgs_dropped \
	    st_connection_drop st_connect_fail ]
	env017_stat_check \
	    $map_list $doc_list $check_type $stat_method $envargs
}

# Check the mpool stat field.
proc env017_mpool_stat { } {
	puts "\nEnv017: Check the mpool stat field"
	set check_type mpool_stat_check
	set stat_method mpool_stat
	set envargs {-create}
	set map_list {
		{ "Cache size (gbytes)"	    st_gbytes }
		{ "Cache size (bytes)"	    st_bytes }
		{ "Number of caches"	    st_ncache }
		{ "Maximum number of caches"	    st_max_ncache }
		{ "Region size"	    st_regsize }
		{ "Maximum memory-mapped file size"	    st_mmapsize }
		{ "Maximum open file descriptors"	    st_maxopenfd }
		{ "Maximum sequential buffer writes"	    st_maxwrite }
		{ "Sleep after writing maximum buffers"	    st_maxwrite_sleep }
		{ "Pages mapped into address space"	    st_map }
		{ "Cache hits"	    st_cache_hit }
		{ "Cache misses"	    st_cache_miss }
		{ "Pages created"	    st_page_create }
		{ "Pages read in"	    st_page_in }
		{ "Pages written"	    st_page_out }
		{ "Clean page evictions"	    st_ro_evict }
		{ "Dirty page evictions"	    st_rw_evict }
		{ "Dirty pages trickled"	    st_page_trickle }
		{ "Cached pages"	    st_pages }
		{ "Cached clean pages"	    st_page_clean }
		{ "Cached dirty pages"	    st_page_dirty }
		{ "Hash buckets"	    st_hash_buckets }
		{ "Default pagesize"	    st_pagesize }
		{ "Hash lookups"	    st_hash_searches }
		{ "Longest hash chain found"	    st_hash_longest }
		{ "Hash elements examined"	    st_hash_examined }
		{ "Number of hash bucket nowaits"	    st_hash_nowait }
		{ "Number of hash bucket waits"	    st_hash_wait }
		{ "Maximum number of hash bucket nowaits"
		    st_hash_max_nowait }
		{ "Maximum number of hash bucket waits"	    st_hash_max_wait }
		{ "Number of region lock nowaits"	    st_region_nowait }
		{ "Number of region lock waits"	    st_region_wait }
		{ "Buffers frozen"	    st_mvcc_frozen }
		{ "Buffers thawed"	    st_mvcc_thawed }
		{ "Frozen buffers freed"	    st_mvcc_freed }
		{ "Page allocations"	    st_alloc }
		{ "Buckets examined during allocation"	    st_alloc_buckets }
		{ "Maximum buckets examined during allocation"
		    st_alloc_max_buckets }
		{ "Pages examined during allocation"	    st_alloc_pages }
		{ "Maximum pages examined during allocation"
		    st_alloc_max_pages }
		{ "Threads waiting on buffer I/O"    st_io_wait}
		{ "Number of syncs interrupted"		st_sync_interrupted}
	}
	set doc_list [list st_gbytes st_bytes st_ncache st_max_ncache \
	    st_regsize st_mmapsize st_maxopenfd st_maxwrite st_maxwrite_sleep \
	    st_map st_cache_hit st_cache_miss st_page_create st_page_in \
	    st_page_out st_ro_evict st_rw_evict st_page_trickle st_pages \
	    st_page_clean st_page_dirty st_hash_buckets st_pagesize \
	    st_hash_searches \
	    st_hash_longest st_hash_examined st_hash_nowait st_hash_wait \
	    st_hash_max_nowait st_hash_max_wait st_region_wait \
	    st_region_nowait st_mvcc_frozen st_mvcc_thawed st_mvcc_freed \
	    st_alloc st_alloc_buckets st_alloc_max_buckets st_alloc_pages \
	    st_alloc_max_pages st_io_wait st_sync_interrupted ] 
	env017_stat_check \
	    $map_list $doc_list $check_type $stat_method $envargs
}

# Check the db stat field.
proc env017_db_stat { } {
	puts "\nEnv017: Check the db stat field"
	set hash_map_list {
		{ "Magic"	    hash_magic }
		{ "Version"	    hash_version }
		{ "Page size"	    hash_pagesize }
		{ "Page count"	    hash_pagecnt }
		{ "Number of keys"	    hash_nkeys }
		{ "Number of records"	    hash_ndata }
		{ "Fill factor"	    hash_ffactor }
		{ "Buckets"	    hash_buckets }
		{ "Free pages"	    hash_free }
		{ "Bytes free"	    hash_bfree }
		{ "Number of big pages"	    hash_bigpages }
		{ "Big pages bytes free"	    hash_big_bfree }
		{ "Overflow pages"	    hash_overflows }
		{ "Overflow bytes free"	    hash_ovfl_free }
		{ "Duplicate pages"	    hash_dup }
		{ "Duplicate pages bytes free"	    hash_dup_free }
		{ "Flags"	flags }
	}
	set queue_map_list {
		{ "Magic"	    qs_magic }
		{ "Version"	    qs_version }
		{ "Page size"	    qs_pagesize }
		{ "Extent size"	    qs_extentsize }
		{ "Number of keys"	qs_nkeys }
		{ "Number of records"	    qs_ndata }
		{ "Record length"	    qs_re_len }
		{ "Record pad"	    qs_re_pad }
		{ "First record number"	    qs_first_recno }
		{ "Last record number"	    qs_cur_recno }
		{ "Number of pages"	    qs_pages }
		{ "Bytes free"	    qs_pgfree}
		{ "Flags"	flags }
	}
	set btree_map_list {
		{ "Magic"	    bt_magic }
		{ "Version"	    bt_version }
		{ "Number of keys"	    bt_nkeys }
		{ "Number of records"	    bt_ndata }
		{ "Minimum keys per page"	    bt_minkey }
		{ "Fixed record length"	    bt_re_len }
		{ "Record pad"	    bt_re_pad }
		{ "Page size"	    bt_pagesize }
		{ "Page count"	    bt_pagecnt }
		{ "Levels"	    bt_levels }
		{ "Internal pages"	    bt_int_pg }
		{ "Leaf pages"	    bt_leaf_pg }
		{ "Duplicate pages"	    bt_dup_pg }
		{ "Overflow pages"	    bt_over_pg }
		{ "Empty pages"	    bt_empty_pg }
		{ "Pages on freelist"	    bt_free }
		{ "Internal pages bytes free"	    bt_int_pgfree }
		{ "Leaf pages bytes free"	    bt_leaf_pgfree }
		{ "Duplicate pages bytes free"	    bt_dup_pgfree }
		{ "Bytes free in overflow pages"	    bt_over_pgfree }
		{ "Flags"	flags }
	}
	set hash_doc_list [list hash_magic hash_version hash_nkeys hash_ndata \
	    hash_pagecnt hash_pagesize hash_ffactor hash_buckets hash_free \
	    hash_bfree hash_bigpages hash_big_bfree hash_overflows \
	    hash_ovfl_free hash_dup hash_dup_free flags]

	set btree_doc_list [list bt_magic bt_version bt_nkeys bt_ndata \
	    bt_pagecnt bt_pagesize bt_minkey bt_re_len bt_re_pad bt_levels \
	    bt_int_pg bt_leaf_pg bt_dup_pg bt_over_pg bt_empty_pg bt_free \
	    bt_int_pgfree bt_leaf_pgfree bt_dup_pgfree bt_over_pgfree flags ]

	set queue_doc_list [list qs_magic qs_version qs_nkeys qs_ndata \
	    qs_pagesize qs_extentsize qs_pages qs_re_len qs_re_pad qs_pgfree \
	    qs_first_recno qs_cur_recno flags ]

	# Check the hash db stat field.
	puts "\tEnv017: Check the hash db stat"
	env017_dbstat_check \
	    $hash_map_list $hash_doc_list {hash_db_stat_check} {-create -hash}

	# Check the queue db stat field.
	puts "\tEnv017: Check the queue db stat"
	env017_dbstat_check \
	    $queue_map_list $queue_doc_list {queue_db_stat_check} \
	    {-create -queue}

	# Check the btree/recno db stat field.
	puts "\tEnv017: Check the btree/recno db stat"
	env017_dbstat_check \
	    $btree_map_list $btree_doc_list {btree_db_stat_check} \
	    {-create -btree}
}


# Check the sequence stat field.
proc env017_seq_stat { } {
	puts "\nEnv017: Check the sequence stat field"
	source ./include.tcl
	env_cleanup $testdir	
	set file1 db1.db
	set db1 [berkdb open -create -btree $testdir/$file1]
	error_check_good is_valid_db [is_valid_db $db1] TRUE
	set seq [berkdb sequence -create -min 0 -max 1024768  $db1 seq_key1]
	error_check_good is_valid_seq [is_valid_seq $seq] TRUE	
	set stat_list [$seq stat]
	set map_list {
		{ "Wait"	    st_wait }
		{ "No wait"	    st_nowait }
		{ "Current"	    st_current }
		{ "Cached"	    st_value }
		{ "Max Cached"	    st_last_value }
		{ "Min"	    st_min }
		{ "Max"	    st_max }
		{ "Cache size"	    st_cache_size}
		{ "Flags"	    st_flags}
	}
	set doc_list [list st_wait st_nowait st_current st_value \
	    st_last_value st_min st_max st_cache_size st_flags]
	env017_do_check $map_list $stat_list $doc_list {seq_stat} 
	error_check_good "$seq close" [$seq close] 0
	error_check_good "$db1 close" [$db1 close] 0
}

# This is common proc for the stat method called by env handle.
proc env017_stat_check { map_list doc_list check_type stat_method \
     {envargs {}} } {
	source ./include.tcl
	env_cleanup $testdir
	set env [eval berkdb_env_noerr $envargs -home $testdir]	
	error_check_good is_valid_env [is_valid_env $env] TRUE
	set stat_list [$env $stat_method]
	env017_do_check $map_list $stat_list $doc_list $check_type 
	error_check_good "$env close" [$env close] 0
}

# This is common proc for db stat.
proc env017_dbstat_check { map_list doc_list check_type {dbargs {}} } {
	source ./include.tcl
	env_cleanup $testdir
	set filename "db1.db"
	set db [eval berkdb_open_noerr $dbargs $testdir/$filename]
	error_check_good is_valid_db [is_valid_db $db] TRUE
	set stat_list [$db stat]
	env017_do_check $map_list $stat_list $doc_list $check_type 
	error_check_good "$db close" [$db close] 0    
}

# This proc does the actual checking job.
proc env017_do_check { map_list stat_list doc_list check_type } {
	# Check if all the items in the stat_list have the corresponding 
	# item in doc_list.
	foreach l $map_list {
		set field_map([lindex $l 0]) [lindex $l 1]
	}
	puts "\tEnv017: Check from stat_list"
	set res_stat_list {}
	foreach item $stat_list {
		puts "\t\tEnv017: Checking item [lindex $item 0]"
		if {![info exists field_map([lindex $item 0])]} {
			lappend res_stat_list [lindex $item 0]
			continue
		}
		set cur_field $field_map([lindex $item 0])
		if {[lsearch -exact $doc_list $cur_field] == -1} {
			lappend res_stat_list [lindex $item 0]
		}
	}
	if {[llength $res_stat_list]>0} {
		puts -nonewline "FAIL: in stat_list of $check_type, "
		puts "Mismatch Items: $res_stat_list"
	}

	# Check if all the items in the doc_list have the corresponding
	# record in the stat_list.
	foreach l $map_list {
		set field_map([lindex $l 1]) [lindex $l 0]
	}
	
	set stat_field_list {}
	
	foreach item $stat_list {
		lappend stat_field_list [lindex $item 0]
	}

	set res_doc_list {}
	puts "\tEnv017: Check from doc_list"
	foreach item $doc_list {
		puts "\t\tEnv017: Checking item [lindex $item 0]"
		if {![info exists field_map([lindex $item 0])]} {
			lappend res_doc_list [lindex $item 0]
			continue
		}
		set cur_field $field_map([lindex $item 0])
		if {[lsearch -exact $stat_field_list $cur_field] == -1} {
			lappend res_doc_list [lindex $item 0]
		}
	}
	if {[llength $res_doc_list]>0} {
		puts -nonewline "FAIL: in doc_list of $check_type, "
		puts "Mismatch Items: $res_doc_list"
	}
}

