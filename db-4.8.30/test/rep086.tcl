# See the file LICENSE for redistribution information.
#
# Copyright (c) 2009 Oracle.  All rights reserved.
#
# TEST	rep086
# TEST  Interrupted abbreviated internal init.
# TEST
# TEST  Make sure we cleanly remove partially loaded named in-memory
# TEST	databases (NIMDBs).

proc rep086 { method { tnum "086" } args } {

	source ./include.tcl 

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
	rep086_sub $method $tnum $args
}

proc rep086_sub { method tnum largs } {

	global testdir
	global util_path
	global rep_verbose
	global verbose_type
	
	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}
	set omethod [convert_method $method]

	env_cleanup $testdir
	replsetup $testdir/MSGQUEUEDIR

	file mkdir [set dirs(A) $testdir/SITE_A]
	file mkdir [set dirs(B) $testdir/SITE_B]

	set pagesize 4096
	append largs " -pagesize $pagesize "
	set log_buf [expr $pagesize * 2]
	set log_max [expr $log_buf * 4]

	puts "Rep$tnum ($method): Test of interrupted abbreviated internal init."
	puts "\tRep$tnum.a: Create master and client."
	repladd 1
	set env_A_cmd "berkdb_env_noerr -create -txn \
	    $verbargs \
	    -log_buffer $log_buf -log_max $log_max -errpfx SITE_A \
	    -home $dirs(A) -rep_transport \[list 1 replsend\]"
	set envs(A) [eval $env_A_cmd -rep_master]

	# Open a client
	repladd 2
	set env_B_cmd "berkdb_env_noerr -create -txn \
	    $verbargs \
	    -log_buffer $log_buf -log_max $log_max -errpfx SITE_B \
	    -home $dirs(B) -rep_transport \[list 2 replsend\]"
	set envs(B) [eval $env_B_cmd -rep_client]

	set envlist "{$envs(A) 1} {$envs(B) 2}"
	process_msgs $envlist

	puts "\tRep$tnum.b: Create a regular DB and a few NIMDBs."
	set niter 200
	set start 0
	eval rep_test $method $envs(A) NULL $niter 0 0 0 $largs
	for { set i 1 } { $i <= 3 } { incr i } {
		set nimdb [eval {berkdb_open} -env $envs(A) -auto_commit \
		    -create $largs $omethod {""} "mynimdb$i"]
		eval rep_test $method $envs(A) \
		    $nimdb $niter $start $start 0 $largs
		$nimdb close
	}
	process_msgs $envlist

	puts "\tRep$tnum.c: Bounce client so it has to re-materialize the NIMDBs."
	$envs(B) close
	set envs(B) [eval $env_B_cmd -rep_client -recover]
	set envlist "{$envs(A) 1} {$envs(B) 2}"

	# Here's a summary reminder of the messaging that is taking place in
	# each of the proc_msgs_once message cycles.
	# 
	# 1. NEWCLIENT -> NEWMASTER -> VERIFY_REQ (the checkpoint written by
	#                        regular recovery)
	# 2.   -> VERIFY -> (no match) VERIFY_REQ (last txn commit in common)
	# 3.   -> VERIFY -> (match, but need NIMDBS) UPDATE_REQ
	# 4.   -> UPDATE -> PAGE_REQ
	# 5.   -> PAGE -> (limited to partial NIMDB content by rep_limit)

	proc_msgs_once $envlist
	proc_msgs_once $envlist
	proc_msgs_once $envlist
	proc_msgs_once $envlist

	# Before doing cycle # 5, set a ridiculously low limit, so that only the
	# first page of the database will be received on this next cycle.
	# 
	$envs(A) rep_limit 0 4
	proc_msgs_once $envlist

	# Just to make sure our test is working the way we think it should,
	# verify that we are indeed in REP_F_RECOVER_PAGE state.
	# 
	assert_rep_flag $dirs(B) REP_F_RECOVER_PAGE 1

	# Now, with only a partial materialization of the NIMDB, downgrade the
	# master, which should cause client to realize its internal init is
	# interrupted.
	# 
	$envs(A) rep_limit 0 0
	$envs(A) rep_start -client
	proc_msgs_once $envlist

	puts "\tRep$tnum.d: Try to open NIMDBs."
	for { set i 0 } { $i <= 3 } { incr i } {
		set cmd [list berkdb_open -env $envs(B) -auto_commit "" "mynimdb$i"]
		error_check_bad "open partially loaded NIMDB" [catch $cmd] 0
	}

	$envs(A) close
	$envs(B) close
	replclose $testdir/MSGQUEUEDIR
}
