# See the file LICENSE for redistribution information.
#
# Copyright (c)-2009 Oracle.  All rights reserved.
#
# TEST	rep083
# TEST  Replication clients must never send VERIFY_FAIL to a c2c request.
# TEST
# TEST  Regression test for a bug [#16592] where a client could send a
# TEST  VERIFY_FAIL to another client, which is illegal.
#
proc rep083 { method { niter 200 } { tnum "083" } args } {
	source ./include.tcl
	global repfiles_in_memory

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win9x platform."
		return
	}

	if { $checking_valid_methods } {
		return "ALL"
	}

	set args [convert_args $method $args]

	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	puts "Rep$tnum: ($method)\
	    Test that client never sends VERIFY_FAIL $msg2."
	rep083_sub $method $niter $tnum $args
}

proc rep083_sub { method niter tnum largs } {
	global testdir
	global util_path
	global repfiles_in_memory
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

	env_cleanup $testdir
	replsetup $testdir/MSGQUEUEDIR

	file mkdir [set dirA $testdir/A]
	file mkdir [set dirB $testdir/B]
	file mkdir [set dirC $testdir/C]

	set pagesize 4096
	append largs " -pagesize $pagesize "
	set log_buf [expr $pagesize * 2]
	set log_max [expr $log_buf * 4]

	repladd 1
	set env_A_cmd "berkdb_env_noerr -create -txn $verbargs $repmemargs \
	    -log_buffer $log_buf -log_max $log_max -errpfx SITE_A \
	    -home $dirA -rep_transport \[list 1 replsend\]"
	set envs(A) [eval $env_A_cmd -rep_master]

	# Open a client
	repladd 2
	set env_B_cmd "berkdb_env_noerr -create -txn $verbargs $repmemargs \
	    -log_buffer $log_buf -log_max $log_max -errpfx SITE_B \
	    -home $dirB -rep_transport \[list 2 replsend\]"
	set envs(B) [eval $env_B_cmd -rep_client]

	# Open 2nd client
	repladd 3
	set env_C_cmd "berkdb_env_noerr -create -txn $verbargs $repmemargs \
	    -log_buffer $log_buf -log_max $log_max -errpfx SITE_C \
	    -home $dirC -rep_transport \[list 3 rep083_send\]"
	set envs(C) [eval $env_C_cmd -rep_client]

	# Bring the clients online by processing the startup messages.
	set envlist "{$envs(A) 1} {$envs(B) 2} {$envs(C) 3}"
	process_msgs $envlist

	# Run rep_test in the master (and update clients).
	puts "\tRep$tnum.a: populate initial portion of log."
	eval rep_test $method $envs(A) NULL $niter 0 0 0 $largs
	process_msgs $envlist

	# Take note of the initial value of "Pages received"
	set pages_rcvd0 [stat_field $envs(C) rep_stat "Pages received"]

	set res [eval exec $util_path/db_archive -l -h $dirB]
	set last_client_log [lindex [lsort $res] end]

	set stop 0
	set start 0
	while { $stop == 0 } {
		# Run rep_test in the master (don't update client).
		puts "\tRep$tnum.b: Fill log until next log file."
		incr start $niter
		eval rep_test $method $envs(A) NULL $niter $start $start 0 $largs

		replclear 3
		process_msgs "{$envs(A) 1} {$envs(B) 2}"

		puts "\tRep$tnum.c: Run db_archive on client B."
		exec $util_path/db_archive -d -h $dirB
		set res [eval exec $util_path/db_archive -l -h $dirB]
		if { [lsearch -exact $res $last_client_log] == -1} {
			set stop 1
		}
	}

	# At this point, client C is far behind (because we've been throwing
	# away messages destined to it).  And client B has minimal log, because
	# we've been aggressively archiving, but the master A has its entire log
	# history.  Therefore, upon resuming messaging to C, it should be able
	# to catch up without doing an internal init.

	puts "\tRep$tnum.d: Write one more txn, and resume msging to C."
	incr start $niter
	eval rep_test $method $envs(A) NULL 1 $start $start 0 $largs
	process_msgs $envlist

	# Pause and do it one more time, to provide time for client C's
	# time-based gap request trigger to work.
	# 
	tclsleep 1
	incr start 1
	eval rep_test $method $envs(A) NULL 1 $start $start 0 $largs
	process_msgs $envlist

	# Make sure C didn't do an internal init (which we detect by testing
	# whether it received any pages recently).
	# 
	error_check_good no_internal_init \
	    [stat_field $envs(C) rep_stat "Pages received"] $pages_rcvd0
	$envs(C) close
	$envs(A) close
	$envs(B) close

	replclose $testdir/MSGQUEUEDIR
}

# We use this special-purpose wrapper send function only at site C.  Since we
# know exactly what sites are in what roles, we can simply hard-code the EID
# numbers: site A (1) is the master, and site B (2) is the desired target site
# for c2c synchronization.
# 
proc rep083_send { control rec fromid toid flags lsn } {
	if {$toid == 1 && [lsearch $flags "rerequest"] == -1 \
	    && [lsearch $flags "any"] != -1} {
		set toid 2
	}
	replsend $control $rec $fromid $toid $flags $lsn
}
