# See the file LICENSE for redistribution information.
#
# Copyright (c)-2009 Oracle.  All rights reserved.
#
# TEST	rep082
# TEST  Sending replication requests to correct master site.
# TEST
# TEST  Regression test for a bug [#16592] where a client could send an
# TEST  UPDATE_REQ to another client instead of the master.
#
proc rep082 { method { niter 200 } { tnum "082" } args } {
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

	puts "Rep$tnum: ($method) Test that\
	    client doesn't send UPDATE_REQ to another client $msg2."

	rep082_sub $method $niter $tnum $args
}

proc rep082_sub { method niter tnum  largs } {
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
	    -home $dirC -rep_transport \[list 3 replsend\]"
	set envs(C) [eval $env_C_cmd -rep_client]

	# Bring the clients online by processing the startup messages.
	set envlist "{$envs(A) 1} {$envs(B) 2} {$envs(C) 3}"
	process_msgs $envlist

	# Run rep_test in the master (and update clients).
	puts "\tRep$tnum.a: populate initial portion of log."
	eval rep_test $method $envs(A) NULL $niter 0 0 0 $largs
	process_msgs $envlist

	$envs(A) close
	$envs(B) close
	$envs(C) close

	# At this point, we have a first section of the log history that was
	# produced at master site A, and is replicated to both other sites.  Now
	# let's produce a second section of history, also produced at master
	# site A, but only replicated to site C; make sure this second section
	# spans a log file boundary.  Archive log files at site C, so that we
	# make sure that site C has only a fraction of this second section.
	#
	set res [eval exec $util_path/db_archive -l -h $dirC]
	set last_client_log [lindex [lsort $res] end]

	set envs(A) [eval $env_A_cmd -recover -rep_master]
	set envs(C) [eval $env_C_cmd -recover -rep_client]
	replclear 2
	process_msgs "{$envs(A) 1} {$envs(C) 3}"

	set stop 0
	set start 0
	set count 0
	while { $stop == 0 } {
		incr count
		# Run rep_test in the master (don't update client).
		puts "\tRep$tnum.b: Fill log until next log file."
		incr start $niter
		eval rep_test $method $envs(A) NULL $niter $start $start 0 $largs

		replclear 2
		process_msgs "{$envs(A) 1} {$envs(C) 3}"

		puts "\tRep$tnum.c: Run db_archive on client C."
		exec $util_path/db_archive -d -h $dirC
		set res [eval exec $util_path/db_archive -l -h $dirC]
		if { [lsearch -exact $res $last_client_log] == -1} {
			set stop 1
		}
	}

	# Now make site B become the master.  Since site B was not running
	# during the last phase, it does not have any of the "second section of
	# log history" that we produced in that phase.  So site A will have to
	# throw away those transactions in order to sync up with B.  HOWEVER,
	# site B will now generate yet another new section of log history, which
	# is identical to the set of transactions generated a moment ago at site
	# A.  In other words, although this is the third section of history to
	# be generated, we have arranged to have it completely replace the
	# second section, and to have it exactly match!  Note that we leave site
	# C out of the picture during this phase.
	# 
	$envs(A) close
	$envs(C) close
	set envs(B) [eval $env_B_cmd -recover -rep_master]
	set envs(A) [eval $env_A_cmd -recover -rep_client]

	set start 0
	while {$count > 0} {
		puts "\tRep$tnum.d: Running rep_test in replicated env."
		incr start $niter
		eval rep_test $method $envs(B) NULL $niter $start $start 0 $largs

		replclear 3
		process_msgs "{$envs(A) 1} {$envs(B) 2}"

		incr count -1
	}

	# Now start up site C again, but configure it to rely on site A for
	# client-to-client synchronization.  Recall the known contents of site
	# C's transaction log: it has a partial copy of the "second section" of
	# log history (it has the end of that section, but not the beginning).
	# The transactions in this log will have the same LSN's as are currently
	# in place at sites A and B (which, remember, were produced by the
	# identical "third section" of history), but the commit record contents
	# won't exactly match, because the third section was produced by master
	# site B.
	#
	# During the verify dance, client C will continue to walk back the log,
	# finding commit records which find matching LSNs at A/B, but no
	# matching contents.  When it hits the archived log file boundary it
	# will have to give up without having found a match.  Thus we have
	# produced a situation where an incoming VERIFY message from another
	# client (site A) results in client C sending an UPDATE_REQ.  We want to
	# make sure that client C sends the UPDATE_REQ to the master, rather
	# than blindly sending to the same site that produced the VERIFY
	# message.
	# 
	puts "\tRep$tnum.e: start client C, with A as peer."
	set env_C_cmd "berkdb_env_noerr -create -txn $verbargs \
	    -log_buffer $log_buf -log_max $log_max -errpfx SITE_C \
	    -home $dirC -rep_transport \[list 3 rep082_send\]"
	set envs(C) [eval $env_C_cmd -recover -rep_client]
	process_msgs "{$envs(A) 1} {$envs(B) 2} {$envs(C) 3}"

	$envs(C) close
	$envs(A) close
	$envs(B) close

	replclose $testdir/MSGQUEUEDIR
}

# We use this special-purpose wrapper send function only in the very last phase
# of the test, and only at site C.  Before that we just use the normal send
# function as usual.  Since we know exactly what sites are in what roles, we can
# simply hard-code the EID numbers: site B (2) is the master, and site A (1) is
# the desired target site for c2c synchronization.
# 
proc rep082_send { control rec fromid toid flags lsn } {
	if {$toid == 2 && [lsearch $flags "rerequest"] == -1 \
	    && [lsearch $flags "any"] != -1} {
		set toid 1
	}
	replsend $control $rec $fromid $toid $flags $lsn
}
