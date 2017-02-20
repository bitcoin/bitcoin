/*-
 * Automatically built by dist/s_java_csharp.
 *
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 */

using System;
using System.Runtime.InteropServices;

namespace BerkeleyDB.Internal {
    [StructLayout(LayoutKind.Sequential)]
    internal struct BTreeStatStruct {
    internal uint bt_magic;     
    internal uint bt_version;       
    internal uint bt_metaflags;     
    internal uint bt_nkeys;     
    internal uint bt_ndata;     
    internal uint bt_pagecnt;       
    internal uint bt_pagesize;      
    internal uint bt_minkey;        
    internal uint bt_re_len;        
    internal uint bt_re_pad;        
    internal uint bt_levels;        
    internal uint bt_int_pg;        
    internal uint bt_leaf_pg;       
    internal uint bt_dup_pg;        
    internal uint bt_over_pg;       
    internal uint bt_empty_pg;      
    internal uint bt_free;      
    internal ulong bt_int_pgfree;   
    internal ulong bt_leaf_pgfree;  
    internal ulong bt_dup_pgfree;   
    internal ulong bt_over_pgfree;  
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct HashStatStruct {
    internal uint hash_magic;       
    internal uint hash_version;     
    internal uint hash_metaflags;   
    internal uint hash_nkeys;       
    internal uint hash_ndata;       
    internal uint hash_pagecnt;     
    internal uint hash_pagesize;    
    internal uint hash_ffactor;     
    internal uint hash_buckets;     
    internal uint hash_free;        
    internal ulong hash_bfree;      
    internal uint hash_bigpages;    
    internal ulong hash_big_bfree;  
    internal uint hash_overflows;   
    internal ulong hash_ovfl_free;  
    internal uint hash_dup;     
    internal ulong hash_dup_free;   
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct LockStatStruct {
    internal uint st_id;        
    internal uint st_cur_maxid;     
    internal uint st_maxlocks;      
    internal uint st_maxlockers;    
    internal uint st_maxobjects;    
    internal uint st_partitions;    
    internal int   st_nmodes;       
    internal uint st_nlockers;      
    internal uint st_nlocks;        
    internal uint st_maxnlocks;     
    internal uint st_maxhlocks;     
    internal ulong st_locksteals;   
    internal ulong st_maxlsteals;   
    internal uint st_maxnlockers;   
    internal uint st_nobjects;      
    internal uint st_maxnobjects;   
    internal uint st_maxhobjects;   
    internal ulong st_objectsteals; 
    internal ulong st_maxosteals;   
    internal ulong st_nrequests;        
    internal ulong st_nreleases;        
    internal ulong st_nupgrade;     
    internal ulong st_ndowngrade;   
    internal ulong st_lock_wait;        
    internal ulong st_lock_nowait;  
    internal ulong st_ndeadlocks;   
    internal uint st_locktimeout;   
    internal ulong st_nlocktimeouts;    
    internal uint st_txntimeout;    
    internal ulong st_ntxntimeouts; 
    internal ulong st_part_wait;        
    internal ulong st_part_nowait;  
    internal ulong st_part_max_wait;    
    internal ulong st_part_max_nowait;  
    internal ulong st_objs_wait;    
    internal ulong st_objs_nowait;  
    internal ulong st_lockers_wait; 
    internal ulong st_lockers_nowait;   
    internal ulong st_region_wait;  
    internal ulong st_region_nowait;    
    internal uint st_hash_len;      
    internal IntPtr   st_regsize;       
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct LogStatStruct {
    internal uint st_magic;     
    internal uint st_version;       
    internal int   st_mode;     
    internal uint st_lg_bsize;      
    internal uint st_lg_size;       
    internal uint st_wc_bytes;      
    internal uint st_wc_mbytes;     
    internal ulong st_record;       
    internal uint st_w_bytes;       
    internal uint st_w_mbytes;      
    internal ulong st_wcount;       
    internal ulong st_wcount_fill;  
    internal ulong st_rcount;       
    internal ulong st_scount;       
    internal ulong st_region_wait;  
    internal ulong st_region_nowait;    
    internal uint st_cur_file;      
    internal uint st_cur_offset;    
    internal uint st_disk_file;     
    internal uint st_disk_offset;   
    internal uint st_maxcommitperflush; 
    internal uint st_mincommitperflush; 
    internal IntPtr   st_regsize;       
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct MPoolFileStatStruct {
    internal string file_name;      
    internal uint st_pagesize;      
    internal uint st_map;       
    internal ulong st_cache_hit;    
    internal ulong st_cache_miss;   
    internal ulong st_page_create;  
    internal ulong st_page_in;      
    internal ulong st_page_out;     
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct MPoolStatStruct {
    internal uint st_gbytes;        
    internal uint st_bytes;     
    internal uint st_ncache;        
    internal uint st_max_ncache;    
    internal IntPtr   st_mmapsize;      
    internal int   st_maxopenfd;        
    internal int   st_maxwrite;     
    internal uint st_maxwrite_sleep;    
    internal uint st_pages;     
    internal uint st_map;       
    internal ulong st_cache_hit;    
    internal ulong st_cache_miss;   
    internal ulong st_page_create;  
    internal ulong st_page_in;      
    internal ulong st_page_out;     
    internal ulong st_ro_evict;     
    internal ulong st_rw_evict;     
    internal ulong st_page_trickle; 
    internal uint st_page_clean;    
    internal uint st_page_dirty;    
    internal uint st_hash_buckets;  
    internal uint st_pagesize;      
    internal uint st_hash_searches; 
    internal uint st_hash_longest;  
    internal ulong st_hash_examined;    
    internal ulong st_hash_nowait;  
    internal ulong st_hash_wait;        
    internal ulong st_hash_max_nowait;  
    internal ulong st_hash_max_wait;    
    internal ulong st_region_nowait;    
    internal ulong st_region_wait;  
    internal ulong st_mvcc_frozen;  
    internal ulong st_mvcc_thawed;  
    internal ulong st_mvcc_freed;   
    internal ulong st_alloc;        
    internal ulong st_alloc_buckets;    
    internal ulong st_alloc_max_buckets;
    internal ulong st_alloc_pages;  
    internal ulong st_alloc_max_pages;  
    internal ulong st_io_wait;      
    internal ulong st_sync_interrupted; 
    internal IntPtr   st_regsize;       
    }

    internal struct MempStatStruct {
        internal MPoolStatStruct st;
        internal MPoolFileStatStruct[] files;
}

    [StructLayout(LayoutKind.Sequential)]
    internal struct MutexStatStruct {
    
    internal uint st_mutex_align;   
    internal uint st_mutex_tas_spins;   
    internal uint st_mutex_cnt;     
    internal uint st_mutex_free;    
    internal uint st_mutex_inuse;   
    internal uint st_mutex_inuse_max;   

    
    internal ulong st_region_wait;  
    internal ulong st_region_nowait;    
    internal IntPtr   st_regsize;       
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct QueueStatStruct {
    internal uint qs_magic;     
    internal uint qs_version;       
    internal uint qs_metaflags;     
    internal uint qs_nkeys;     
    internal uint qs_ndata;     
    internal uint qs_pagesize;      
    internal uint qs_extentsize;    
    internal uint qs_pages;     
    internal uint qs_re_len;        
    internal uint qs_re_pad;        
    internal uint qs_pgfree;        
    internal uint qs_first_recno;   
    internal uint qs_cur_recno;     
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct RecnoStatStruct {
    internal uint bt_magic;     
    internal uint bt_version;       
    internal uint bt_metaflags;     
    internal uint bt_nkeys;     
    internal uint bt_ndata;     
    internal uint bt_pagecnt;       
    internal uint bt_pagesize;      
    internal uint bt_minkey;        
    internal uint bt_re_len;        
    internal uint bt_re_pad;        
    internal uint bt_levels;        
    internal uint bt_int_pg;        
    internal uint bt_leaf_pg;       
    internal uint bt_dup_pg;        
    internal uint bt_over_pg;       
    internal uint bt_empty_pg;      
    internal uint bt_free;      
    internal ulong bt_int_pgfree;   
    internal ulong bt_leaf_pgfree;  
    internal ulong bt_dup_pgfree;   
    internal ulong bt_over_pgfree;  
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct RepMgrStatStruct {
    internal ulong st_perm_failed;  
    internal ulong st_msgs_queued;  
    internal ulong st_msgs_dropped; 
    internal ulong st_connection_drop;  
    internal ulong st_connect_fail; 
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct ReplicationStatStruct {
    /* !!!
     * Many replication statistics fields cannot be protected by a mutex
     * without an unacceptable performance penalty, since most message
     * processing is done without the need to hold a region-wide lock.
     * Fields whose comments end with a '+' may be updated without holding
     * the replication or log mutexes (as appropriate), and thus may be
     * off somewhat (or, on unreasonable architectures under unlucky
     * circumstances, garbaged).
     */
    internal ulong st_log_queued;   
    internal uint st_startup_complete;  
    internal uint st_status;        
    internal DB_LSN_STRUCT st_next_lsn;     
    internal DB_LSN_STRUCT st_waiting_lsn;      
    internal DB_LSN_STRUCT st_max_perm_lsn;     
    internal uint st_next_pg;       
    internal uint st_waiting_pg;    

    internal uint st_dupmasters;    
    internal int st_env_id;         
    internal uint st_env_priority;  
    internal ulong st_bulk_fills;   
    internal ulong st_bulk_overflows;   
    internal ulong st_bulk_records; 
    internal ulong st_bulk_transfers;   
    internal ulong st_client_rerequests;
    internal ulong st_client_svc_req;   
    internal ulong st_client_svc_miss;  
    internal uint st_gen;       
    internal uint st_egen;      
    internal ulong st_log_duplicated;   
    internal ulong st_log_queued_max;   
    internal ulong st_log_queued_total; 
    internal ulong st_log_records;  
    internal ulong st_log_requested;    
    internal int st_master;         
    internal ulong st_master_changes;   
    internal ulong st_msgs_badgen;  
    internal ulong st_msgs_processed;   
    internal ulong st_msgs_recover; 
    internal ulong st_msgs_send_failures;
    internal ulong st_msgs_sent;    
    internal ulong st_newsites;     
    internal uint st_nsites;        
    internal ulong st_nthrottles;   
    internal ulong st_outdated;     
    internal ulong st_pg_duplicated;    
    internal ulong st_pg_records;   
    internal ulong st_pg_requested; 
    internal ulong st_txns_applied; 
    internal ulong st_startsync_delayed;

    
    internal ulong st_elections;    
    internal ulong st_elections_won;    

    
    internal int st_election_cur_winner;    
    internal uint st_election_gen;  
    internal DB_LSN_STRUCT st_election_lsn;     
    internal uint st_election_nsites;   
    internal uint st_election_nvotes;   
    internal uint st_election_priority; 
    internal int st_election_status;        
    internal uint st_election_tiebreaker;
    internal uint st_election_votes;    
    internal uint st_election_sec;  
    internal uint st_election_usec; 
    internal uint st_max_lease_sec; 
    internal uint st_max_lease_usec;    

    
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct SequenceStatStruct {
    internal ulong st_wait;     
    internal ulong st_nowait;       
    internal long  st_current;      
    internal long  st_value;        
    internal long  st_last_value;   
    internal long  st_min;      
    internal long  st_max;      
    internal int   st_cache_size;   
    internal uint st_flags;     
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct TransactionStatStruct {
    internal uint st_nrestores;     
    internal DB_LSN_STRUCT    st_last_ckp;      
    internal long     st_time_ckp;      
    internal uint st_last_txnid;    
    internal uint st_maxtxns;       
    internal ulong st_naborts;      
    internal ulong st_nbegins;      
    internal ulong st_ncommits;     
    internal uint st_nactive;       
    internal uint st_nsnapshot;     
    internal uint st_maxnactive;    
    internal uint st_maxnsnapshot;  
    internal IntPtr st_txnarray;    
    internal ulong st_region_wait;  
    internal ulong st_region_nowait;    
    internal IntPtr   st_regsize;       
    }

    internal struct DB_LSN_STRUCT {
        internal uint file;
        internal uint offset;
    }

internal enum DB_TXN_ACTIVE_STATUS {
        TXN_ABORTED = 1,
        TXN_COMMITTED = 2,
        TXN_PREPARED = 3,
        TXN_RUNNING = 4,
}

    [StructLayout(LayoutKind.Sequential)]
    internal struct DB_TXN_ACTIVE {
    internal uint txnid;        
    internal uint parentid;     
    internal int     pid;           
    internal uint tid;      

    internal DB_LSN_STRUCT    lsn;          

    internal DB_LSN_STRUCT    read_lsn;     
    internal uint mvcc_ref;     

    internal DB_TXN_ACTIVE_STATUS status;       

    }

    internal struct TxnStatStruct {
        internal TransactionStatStruct st;
        internal DB_TXN_ACTIVE[] st_txnarray;
        internal byte[][] st_txngids;
        internal string[] st_txnnames;
    }
}

