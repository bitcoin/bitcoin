# See the file LICENSE for redistribution information.
#
# Copyright (c) 2005-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	log008
# TEST	Test what happens if a txn_ckp record falls into a
# TEST 	different log file than the DBREG_CKP records generated
# TEST	by the same checkpoint.

proc log008 { { nhandles 100 } args } {
	source ./include.tcl
	set tnum "008"

	puts "Log$tnum: Checkpoint test with records spanning log files."
	env_cleanup $testdir

	# Set up env command for use later.
	set envcmd "berkdb_env -create -txn -home $testdir"

	# Start up a child process which will open a bunch of handles
	# on a database and write to it, running until it creates a
	# checkpoint with records spanning two log files.
	puts "\tLog$tnum.a: Spawning child tclsh."
	set pid [exec $tclsh_path $test_path/wrap.tcl \
	    log008script.tcl $testdir/log008script.log $nhandles &]

	watch_procs $pid 3

	puts "\tLog$tnum.b: Child is done."

	# Join the env with recovery.  This ought to work.
	puts "\tLog$tnum.c: Join abandoned child env with recovery."
	set env [eval $envcmd -recover]

	# Clean up.
	error_check_good env_close [$env close] 0

	# Check log file for failures.
	set errstrings [eval findfail $testdir/log008script.log]
	foreach str $errstrings {
		puts "FAIL: error message in log008 log file: $str"
	}
}

