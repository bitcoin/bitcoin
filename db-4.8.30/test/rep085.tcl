# See the file LICENSE for redistribution information.
#
# Copyright (c) 2009 Oracle.  All rights reserved.
#
# TEST	rep085
# TEST  Skipping unnecessary abbreviated internal init.
# TEST
# TEST  Make sure that once we've materialized NIMDBs, we don't bother
# TEST  trying to do it again on subsequent sync without recovery.  Make
# TEST  sure we do probe for the need to materialize NIMDBs, but don't do
# TEST  any internal init at all if there are no NIMDBs.  Note that in order to
# TEST  do this test we don't even need any NIMDBs.

proc rep085 { method {niter 20} {tnum 085} args } {
	source ./include.tcl

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}

	# Run for btree and queue only.  Since this is a NIMDB test, 
	# skip queueext. 
	if { $checking_valid_methods } {
		set test_methods {}
		foreach method $valid_methods {
			if { [is_btree $method] == 1 || [is_queue $method] == 1 } {
				if { [is_queueext $method] == 0 } {
					lappend test_methods $method
				}
			}
		}
		return $test_methods
	}
	if { [is_btree $method] != 1 && [is_queue $method] != 1 } {
		puts "Skipping internal init test rep$tnum for method $method."
		return
	}
	if { [is_queueext $method] == 1 } {
		puts "Skipping in-memory database test rep$tnum for method $method."
		return
	}

	set args [convert_args $method $args]

	rep085_sub $method $niter $tnum $args
}

proc rep085_sub { method niter tnum largs } {
	global testdir
	global rep_verbose
	global verbose_type
	global rep085_page_msg_count rep085_update_req_count
	
	puts "Rep$tnum ($method): skipping unnecessary abbreviated internal init."

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	env_cleanup $testdir
	replsetup $testdir/MSGQUEUEDIR

	file mkdir [set dirs(A) $testdir/SITE_A]
	file mkdir [set dirs(B) $testdir/SITE_B]
	file mkdir [set dirs(C) $testdir/SITE_C]

	set rep085_page_msg_count 0
	set rep085_update_req_count 0

	puts "\tRep$tnum.a: Create master"
	repladd 1
	set env_A_cmd "berkdb_env_noerr -create -txn \
	    $verbargs \
	    -errpfx SITE_A -errfile /dev/stderr \
	    -home $dirs(A) -rep_transport \[list 1 rep085_send\]"
	set envs(A) [eval $env_A_cmd -rep_master]

	puts "\tRep$tnum.b: create (only) a regular DB"
	set start 0
	eval rep_test $method $envs(A) NULL $niter $start $start 0 $largs

	puts "\tRep$tnum.c: Create two clients"
	repladd 2
	set env_B_cmd "berkdb_env_noerr -create -txn \
	    $verbargs \
	    -errpfx SITE_B -errfile /dev/stderr \
	    -home $dirs(B) -rep_transport \[list 2 rep085_send\]"
	set envs(B) [eval $env_B_cmd -rep_client]

	repladd 3
	set env_C_cmd "berkdb_env_noerr -create -txn \
	    $verbargs \
	    -errpfx SITE_C -errfile /dev/stderr \
	    -home $dirs(C) -rep_transport \[list 3 rep085_send\]"
	set envs(C) [eval $env_C_cmd -rep_client]

	set envlist "{$envs(A) 1} {$envs(B) 2} {$envs(C) 3}"
	process_msgs $envlist

	# Note that the initial internal init that we've just done should have
	# the effect of setting this flag.  The flag indicates that any NIMDBs
	# have been loaded, and any full internal init of course accomplishes
	# that.  If there are no NIMDBs whatsoever (which is the case here),
	# then the condition "any NIMDBs are loaded" is trivially satisfied.
	# 
	assert_rep_flag $dirs(C) REP_F_NIMDBS_LOADED 1

	# Restart client C with recovery, which forces it to check for NIMDBs
	# even though a full internal init is not necessary.
	# 
	puts "\tRep$tnum.d: Bounce client C"
	$envs(C) close
	set envs(C) [eval $env_C_cmd -rep_client -recover]
	assert_rep_flag $dirs(C) REP_F_NIMDBS_LOADED 0
	set upd_before $rep085_update_req_count
	set pg_before $rep085_page_msg_count
	set envlist "{$envs(A) 1} {$envs(B) 2} {$envs(C) 3}"
	process_msgs $envlist
	error_check_good update.msg.sent \
	    $rep085_update_req_count [incr upd_before]
	error_check_good no.page.msg $rep085_page_msg_count $pg_before
	assert_rep_flag $dirs(C) REP_F_NIMDBS_LOADED 1

	# Switch masters, forcing client C to re-sync.  But this time it already
	# knows it has NIMDBs, so even an UPDATE_REQ shouldn't be necessary.
	# 
	puts "\tRep$tnum.e: Switch master to site B"
	$envs(A) rep_start -client
	$envs(B) rep_start -master
	set upd_before $rep085_update_req_count
	set pg_before $rep085_page_msg_count
	process_msgs $envlist
	error_check_good no.update.msg $rep085_update_req_count $upd_before
	error_check_good no.page.msg.2 $rep085_page_msg_count $pg_before

	$envs(A) close
	$envs(B) close
	$envs(C) close
	replclose $testdir/MSGQUEUEDIR
}

proc rep085_send { control rec fromid toid flags lsn } {
	global rep085_page_msg_count rep085_update_req_count

	if {[berkdb msgtype $control] eq "page"} {
		incr rep085_page_msg_count
	} elseif {[berkdb msgtype $control] eq "update_req"} {
		incr rep085_update_req_count
	}

	return [replsend $control $rec $fromid $toid $flags $lsn]
}
