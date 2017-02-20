# See the file LICENSE for redistribution information.
#
# Copyright (c) 2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	mut002
# TEST	Two-process mutex test.
#	
# TEST	Allocate and lock a self-blocking mutex.  Start another process. 
# TEST	Try to lock the mutex again -- it will block.  
# TEST	Unlock the mutex from the other process, and the blocked 
# TEST 	lock should be obtained.  Clean up.
# TEST	Do another test with a "-process-only" mutex.  The second 
# TEST	process should not be able to unlock the mutex.

proc mut002 { } {
	source ./include.tcl

	puts "Mut002: Two process mutex test."

	# Open an env.
	set env [berkdb_env -create -home $testdir]
	
	puts "\tMut002.a: Allocate and lock a mutex."
	set mutex [$env mutex -self_block]
	error_check_good obtained_lock [$env mutex_lock $mutex] 0

	# Start a second process.
	puts "\tMut002.b: Start another process."
	set p2 [exec $tclsh_path $test_path/wrap.tcl mut002script.tcl\
	    $testdir/mut002.log $testdir $mutex &]

	# Try to lock the mutex again.  This will hang until the second
	# process unlocks it. 
	$env mutex_lock $mutex

	watch_procs $p2 1 20

	# Clean up, and check the log file from process 2. 
	error_check_good mutex_unlock [$env mutex_unlock $mutex] 0
	error_check_good env_close [$env close] 0 

	# We expect the log file to be empty.  If there are any 
	# messages, report them as failures.
	set fd [open $testdir/mut002.log r]
	while { [gets $fd line] >= 0 } {
		puts "FAIL: unexpected output in log file mut002: $line"
	}
	close $fd
}

