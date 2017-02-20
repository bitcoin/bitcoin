# See the file LICENSE for redistribution information.
#
# Copyright (c) 2007-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep080
# TEST	NOAUTOINIT with empty client logs.
# TEST	
# TEST	Verify that a fresh client trying to join the group for
# TEST	the first time observes the setting of DELAY_SYNC and NOAUTOINIT
# TEST	properly.
# TEST	
proc rep080 { method { niter 200 } { tnum "080" } args } {

	source ./include.tcl
	global mixed_mode_logging
	global databases_in_memory
	global repfiles_in_memory

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win9x platform."
		return
	}

	# Skip for all methods except btree.
	if { $checking_valid_methods } {
		return btree
	}
	if { [is_btree $method] == 0 } {
		puts "Rep$tnum: skipping for non-btree method $method."
		return
	}

	if { $mixed_mode_logging != 0 } {
		puts "Rep$tnum: skipping for in-mem (or mixed) logging."
		return
	}

	set args [convert_args $method $args]

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

	# The {"" "-recover"} loop that is typical in replication tests can be
	# useful for tests which close existing environments at some point, and
	# then later reopen them.  (When we reopen, we do so either with
	# recovery, or without it.)  But this test never does that.
	# 
	puts "Rep$tnum ($method):\
	    Test of NOAUTOINIT with empty client logs $msg $msg2."
	rep080_sub $method $niter $tnum $args
}

proc rep080_sub { method niter tnum largs } {
	global testdir
	global databases_in_memory
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

	set masterdir $testdir/MASTERDIR
	set clientdir1 $testdir/CLIENTDIR1
	set clientdir2 $testdir/CLIENTDIR2
	set clientdir3 $testdir/CLIENTDIR3
	set clientdir4 $testdir/CLIENTDIR4

	file mkdir $masterdir
	file mkdir $clientdir1
	file mkdir $clientdir2
	file mkdir $clientdir3
	file mkdir $clientdir4

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $verbargs -errpfx MASTER \
	    $repmemargs \
	    -home $masterdir -txn -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd -rep_master]
	set envlist "{$masterenv 1}"

	# Run rep_test in the master.
	puts "\tRep$tnum.a: Running rep_test in replicated env."
	eval rep_test $method $masterenv NULL $niter 0 0 $largs
	process_msgs $envlist

	# Open a client
	puts "\tRep$tnum.b: Add a normal client."
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $verbargs -errpfx CLIENT \
	    $repmemargs \
	    -home $clientdir1 -txn -rep_transport \[list 2 replsend\]"
	set clientenv1 [eval $cl_envcmd -rep_client]
	lappend envlist [list $clientenv1 2]
	process_msgs $envlist

	rep_verify $masterdir $masterenv $clientdir1 $clientenv1 0 1 1

	# Open a client with NOAUTOINIT
	# 
	puts "\tRep$tnum.c: Add a client with NOAUTOINIT (should fail)."
	repladd 3
	set cl_envcmd "berkdb_env_noerr -create $verbargs -errpfx CLIENT \
	    $repmemargs \
	    -home $clientdir2 -txn -rep_transport \[list 3 replsend\]"
	set clientenv2 [eval $cl_envcmd -rep_client]
	$clientenv2 rep_config {noautoinit on}

	lappend envlist [list $clientenv2 3]
	process_msgs $envlist 0 NONE error
	error_check_good errchk [is_substr $error JOIN_FAILURE] 1

	# Open a client with DELAY_SYNC
	# 
	puts "\tRep$tnum.d: Add a client with DELAY_SYNC."
	repladd 4
	set cl_envcmd "berkdb_env_noerr -create $verbargs -errpfx CLIENT \
	    $repmemargs \
	    -home $clientdir3 -txn -rep_transport \[list 4 replsend\]"
	set clientenv3 [eval $cl_envcmd -rep_client]
	$clientenv3 rep_config {delayclient on}

	lappend envlist [list $clientenv3 4]
	process_msgs $envlist 0 NONE error
	error_check_good errchk2 $error 0

	error_check_bad expect_error [catch {rep_verify \
	    $masterdir $masterenv $clientdir3 $clientenv3 0 1 1}] 0

	error_check_good rep_sync [$clientenv3 rep_sync] 0
	process_msgs $envlist 0 NONE error
	error_check_good errchk3 $error 0
	rep_verify $masterdir $masterenv $clientdir3 $clientenv3

	# Open a client with both DELAY_SYNC and NOAUTOINIT
	# 
	puts "\tRep$tnum.f: Add a client with DELAY_SYNC and NOAUTOINIT."
	repladd 5
	set cl_envcmd "berkdb_env_noerr -create $verbargs -errpfx CLIENT \
	    $repmemargs \
	    -home $clientdir4 -txn -rep_transport \[list 5 replsend\]"
	set clientenv4 [eval $cl_envcmd -rep_client]
	$clientenv4 rep_config {delayclient on}
	$clientenv4 rep_config {noautoinit on}

	lappend envlist [list $clientenv4 5]
	process_msgs $envlist 0 NONE error
	error_check_good process_msgs $error 0

	error_check_bad expect_error2 [catch {rep_verify\
	    $masterdir $masterenv $clientdir4 $clientenv4 0 1 1}] 0

	error_check_bad rep_sync [catch {$clientenv4 rep_sync} result] 0
	error_check_good errchk5 [is_substr $result JOIN_FAILURE] 1
	
	$masterenv close
	$clientenv1 close
	$clientenv2 close
	$clientenv3 close
	$clientenv4 close
	replclose $testdir/MSGQUEUEDIR
}
