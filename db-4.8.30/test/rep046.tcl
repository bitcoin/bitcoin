# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST  rep046
# TEST	Replication and basic bulk transfer.
# TEST	Set bulk transfer replication option.
# TEST	Run long txns on master and then commit.  Process on client
# TEST	and verify contents.  Run a very long txn so that logging
# TEST	must send the log.  Process and verify on client.
#
proc rep046 { method { nentries 200 } { tnum "046" } args } {
	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win9x platform."
		return
	}

	if { $checking_valid_methods } {
		return "ALL"
	}

	set args [convert_args $method $args]
	set logsets [create_logsets 3]

	# Set up for on-disk or in-memory databases.
	set msg "using on-disk databases"
	if { $databases_in_memory } {
		set msg "using named in-memory databases"
		if { [is_queueext $method] } { 
			puts -nonewline "Skipping rep$tnum for method "
			puts "$method with named in-memory databases."
			return
		}
	}

	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	# Run the body of the test with and without recovery.
	set throttle { "throttle" "" }
	foreach r $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Skipping test with -recover for \
				    in-memory logs."
				continue
			}
			foreach t $throttle {
				puts "Rep$tnum ($method $r $t):\
				    Replication and bulk transfer $msg $msg2."
				puts "Rep$tnum: Master logs are [lindex $l 0]"
				puts "Rep$tnum: Client 0 logs are [lindex $l 1]"
				puts "Rep$tnum: Client 1 logs are [lindex $l 2]"
				rep046_sub $method $nentries $tnum $l $r \
				    $t $args
			}
		}
	}
}

proc rep046_sub { method niter tnum logset recargs throttle largs } {
	global overflowword1
	global overflowword2
	global databases_in_memory
	global repfiles_in_memory
	global testdir
	global util_path
	global rep_verbose
	global verbose_type

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	set repmemargs ""
	if { $repfiles_in_memory } {
		set repmemargs "-rep_inmem_files "
	}

	set orig_tdir $testdir
	env_cleanup $testdir

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	file mkdir $masterdir
	file mkdir $clientdir

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]
	set c2_logtype [lindex $logset 2]

	set in_memory_log \
	    [expr { $m_logtype == "in-memory" || $c_logtype == "in-memory" || \
	    $c2_logtype == "in-memory" }]

	# In-memory logs require a large log buffer, and can not
	# be used with -txn nosync.  Adjust the args for master
	# and client.
	# This test has a long transaction, allocate a larger log 
	# buffer for in-memory test.
	set m_logargs [adjust_logargs $m_logtype [expr 20 * 1024 * 1024]]
	set c_logargs [adjust_logargs $c_logtype [expr 20 * 1024 * 1024]]
	set c2_logargs [adjust_logargs $c2_logtype [expr 20 * 1024 * 1024]]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]
	set c2_txnargs [adjust_txnargs $c2_logtype]

	# If replication files are in-memory we'll need a bigger cache.
	set cacheargs ""
	if { $repfiles_in_memory } {
		set cachesize [expr 8 * (1024 * 1024)]
		set cacheargs "-cachesize {0 $cachesize 1} "
	}

	set bigniter [expr 10000 - [expr 2 * $niter]]
	set lkmax [expr $bigniter * 2]

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs $m_logargs \
	    $repmemargs $cacheargs \
	    $verbargs -lock_max_locks 10000 -lock_max_objects 10000 \
	    -errpfx MASTER -home $masterdir -rep_master -rep_transport \
	    \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs]
	error_check_good master_env [is_valid_env $masterenv] TRUE

	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs $c_logargs \
	    $repmemargs $cacheargs \
	    $verbargs -home $clientdir -errpfx CLIENT \
	    -lock_max_locks 10000 -lock_max_objects 10000 \
	    -rep_client -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd $recargs]

	if { $throttle == "throttle" } {
		set clientdir2 $testdir/CLIENTDIR2
		file mkdir $clientdir2
		repladd 3
		set cl2_envcmd "berkdb_env_noerr -create $c2_txnargs $verbargs \
		    $repmemargs $cacheargs \
		    $c2_logargs -home $clientdir2 -errpfx CLIENT2 \
	    	    -lock_max_locks 10000 -lock_max_objects 10000 \
		    -rep_client -rep_transport \[list 3 replsend\]"
		set cl2env [eval $cl2_envcmd $recargs]
		set envlist "{$masterenv 1} {$clientenv 2} {$cl2env 3}"
		#
		# Turn throttling on in master
		#
		error_check_good thr [$masterenv rep_limit 0 [expr 32 * 1024]] 0
	} else {
		set envlist "{$masterenv 1} {$clientenv 2}"
	}
	# Bring the client online by processing the startup messages.
	process_msgs $envlist

	#
	# Turn on bulk processing now on the master.
	#
	error_check_good set_bulk [$masterenv rep_config {bulk on}] 0

	puts "\tRep$tnum.a: Create and open master database"
	# Set up databases as in-memory or on-disk.
	if { $databases_in_memory } {
		set dbname { "" "test.db" }
	} else { 
		set dbname "test.db"
	} 

	set omethod [convert_method $method]
	set masterdb [eval {berkdb_open_noerr -env $masterenv -auto_commit \
	    -create -mode 0644} $largs $omethod $dbname]
	error_check_good dbopen [is_valid_db $masterdb] TRUE

	# Process database.
	process_msgs $envlist

	# Run a modified test001 in the master (and update clients).
	puts "\tRep$tnum.b: Basic long running txn"
	set bulkrec1 [stat_field $masterenv rep_stat "Bulk records stored"]
	set bulkxfer1 [stat_field $masterenv rep_stat "Bulk buffer transfers"]

	set overflowword1 "0"
	set overflowword2 "0"
	rep_test_bulk $method $masterenv $masterdb $niter 0 0
	process_msgs $envlist
	set bulkrec2 [stat_field $masterenv rep_stat "Bulk records stored"]
	set bulkxfer2 [stat_field $masterenv rep_stat "Bulk buffer transfers"]
	error_check_good recstat [expr $bulkrec2 > $bulkrec1] 1
	error_check_good xferstat [expr $bulkxfer2 > $bulkxfer1] 1
	rep_verify $masterdir $masterenv\
	    $clientdir $clientenv $in_memory_log 1 1

	puts "\tRep$tnum.c: Very long txn"
	# Determine whether this build is configured with --enable-debug_rop
	# or --enable-debug_wop.
	set conf [berkdb getconfig]
	set debug_rop_wop 0
	if { [is_substr $conf "debug_rop"] == 1 || \
	    [is_substr $conf "debug_wop"] == 1 } {
		set debug_rop_wop 1
	}

	# If debug_rop/wop is set test will write more info to log.
	# An in-memory log has a smaller "file" size than the large
	# items written in this part of the test, so skip this section
	# if any in-memory logs and debug_rop/wop is set.
	if { $in_memory_log == 1 && $debug_rop_wop == 1 } {
		puts "\t\tSkipping for in-memory log and debug_rop/wop"
	} else {
		set skip $niter
		set start $niter
		set orig $niter
		set bulkfill1 [stat_field $masterenv rep_stat \
		    "Bulk buffer fills"]
		rep_test_bulk $method $masterenv $masterdb $bigniter \
		    $start $skip
		set start [expr $niter + $bigniter]
		if { $throttle == "throttle" } {
			#
			# If we're throttling clear all messages from client 3
			# so that we force a huge gap that the client will have
			# to ask for to invoke a rerequest that throttles.
			#
			replclear 3
			set old_thr \
			    [stat_field $masterenv rep_stat \
			    "Transmission limited"]
		}
		process_msgs $envlist
		set bulkfill2 [stat_field $masterenv rep_stat \
		    "Bulk buffer fills"]
		error_check_good fillstat [expr $bulkfill2 > $bulkfill1] 1
		rep_verify $masterdir $masterenv $clientdir $clientenv \
		    $in_memory_log 1 1
        }

	puts "\tRep$tnum.d: Very large data"

	# If debug_rop/wop is set test will write entire item to log.
	# An in-memory log has a smaller "file" size than the large
	# items written in this part of the test, so skip this section
	# if any in-memory logs and debug_rop/wop is set.
	if { $in_memory_log == 1 && $debug_rop_wop == 1 } {
		puts "\t\tSkipping for in-memory log and debug_rop/wop"
	} else {
		set bulkovf1 [stat_field $masterenv rep_stat \
		    "Bulk buffer overflows"]
		set bulkfill1 [stat_field $masterenv rep_stat \
		    "Bulk buffer fills"]
		#
		# Send in '2' exactly because we're sending in the flag to use
		# the overflow entries.  We have 2 overflow entries.
		# If it's fixed length, we can't overflow.  Induce throttling
		# by putting in a bunch more entries.  Create a gap by
		# forcing a checkpoint record.
		#
		$masterenv txn_checkpoint -force
		process_msgs $envlist
		tclsleep 1
		if { [is_fixed_length $method] == 1 } {
			rep_test_bulk $method $masterenv $masterdb $niter \
			    $start $start 0
		} else {
			rep_test_bulk $method $masterenv $masterdb 2 0 0 1
		}
		process_msgs $envlist

		# Generally overflows cannot happen because large data gets
		# broken up into overflow pages, and none will be larger than
		# the buffer.  However, if we're configured for debug_rop/wop
		# then we record the data as is and will overflow.
		#
		set bulkovf2 [stat_field $masterenv rep_stat \
		    "Bulk buffer overflows"]
		set bulkfill2 [stat_field $masterenv rep_stat \
		    "Bulk buffer fills"]
		if { [is_fixed_length $method] == 0 } {
			error_check_good fillstat1 \
			    [expr $bulkfill2 > $bulkfill1] 1
			if { $debug_rop_wop == 1 } {
				error_check_good overflows \
				    [expr $bulkovf2 > $bulkovf1] 1
			} else {
				error_check_good no_overflows $bulkovf2 0
			}
		}
	}

	# !!!
	# Turn off bulk processing now on the master.  We need to do
	# this because some configurations (like debug_rop/wop) will
	# generate log records when verifying the logs and databases.
	# We want to control processing those messages.
	#
	error_check_good set_bulk [$masterenv rep_config {bulk off}] 0

	if { $in_memory_log == 1 && $debug_rop_wop == 1 } {
		puts "\t\tSkipping for in-memory log and debug_rop/wop"
	} else {
		rep_verify $masterdir $masterenv $clientdir $clientenv \
		    $in_memory_log

		if { $throttle == "throttle" } {
			puts "\tRep$tnum.e: Verify throttling."
			set new_thr \
			    [stat_field $masterenv rep_stat \
			    "Transmission limited"]
			error_check_bad nthrottles1 $new_thr -1
			error_check_bad nthrottles0 $new_thr 0
			error_check_good nthrottles \
			    [expr $old_thr < $new_thr] 1
			process_msgs $envlist
			rep_verify $masterdir $masterenv $clientdir2 $cl2env \
			    $in_memory_log
		}
	}

	if { $throttle == "throttle" } {
		error_check_good cclose [$cl2env close] 0
	}

	error_check_good dbclose [$masterdb close] 0
	error_check_good mclose [$masterenv close] 0
	error_check_good cclose [$clientenv close] 0
	replclose $testdir/MSGQUEUEDIR
}
