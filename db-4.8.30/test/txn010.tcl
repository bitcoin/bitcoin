# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	txn010
# TEST	Test DB_ENV->txn_checkpoint arguments/flags
proc txn010 { } {
	source ./include.tcl

	puts "Txn010: test DB_ENV->txn_checkpoint arguments/flags."
	env_cleanup $testdir

	# Open an environment and database.
	puts "\tTxn010.a: open the environment and a database, checkpoint."
	set env [berkdb_env -create -home $testdir -txn]
	error_check_good envopen [is_valid_env $env] TRUE
	set db [berkdb_open \
	    -env $env -create -mode 0644 -btree -auto_commit a.db]
	error_check_good dbopen [is_valid_db $db] TRUE

	# Insert some data and do a checkpoint.
	for { set count 0 } { $count < 100 } { incr count } {
		set t [$env txn]
		error_check_good "init: put" \
		    [$db put -txn $t "key_a_$count" "data"] 0
		error_check_good "init: commit" [$t commit] 0
	}
	tclsleep 1
	error_check_good checkpoint [$env txn_checkpoint] 0

	# Test that checkpoint calls are ignored in quiescent systems.
	puts "\tTxn010.b: test for checkpoints when system is quiescent"
	set chkpt [txn010_stat $env "Time of last checkpoint"]
	for { set count 0 } { $count < 5 } {incr count } {
		tclsleep 1
		error_check_good checkpoint [$env txn_checkpoint] 0
		set test_chkpt [txn010_stat $env "Time of last checkpoint"]
		error_check_good "quiescent: checkpoint time changed" \
		    [expr $test_chkpt == $chkpt] 1
	}

	# Add a single record, and test that checkpoint does something.
	set chkpt [txn010_stat $env "Time of last checkpoint"]
	set t [$env txn]
	error_check_good \
	    "quiescent: put" [$db put -txn $t "key_b_$count" "data"] 0
	error_check_good "quiescent: commit" [$t commit] 0
	tclsleep 1
	error_check_good checkpoint [$env txn_checkpoint] 0
	set test_chkpt [txn010_stat $env "Time of last checkpoint"]
	error_check_good "quiescent: checkpoint time unchanged" \
	    [expr $test_chkpt > $chkpt] 1

	# Test that -force causes a checkpoint.
	puts "\tTxn010.c: test checkpoint -force"
	set chkpt [txn010_stat $env "Time of last checkpoint"]
	for { set count 0 } { $count < 5 } {incr count } {
		tclsleep 1
		error_check_good checkpoint [$env txn_checkpoint -force] 0
		set test_chkpt [txn010_stat $env "Time of last checkpoint"]
		error_check_good "force: checkpoint time unchanged" \
		    [expr $test_chkpt > $chkpt] 1
		set chkpt $test_chkpt
	}

	# Test that -kbyte doesn't cause a checkpoint unless there's
	# enough activity.
	puts "\tTxn010.d: test checkpoint -kbyte"

	# Put in lots of data, and verify that -kbyte causes a checkpoint
	for { set count 0 } { $count < 1000 } { incr count } {
		set t [$env txn]
		error_check_good "kbyte: put" \
		    [$db put -txn $t "key_c_$count" "data"] 0
		error_check_good "kbyte: commit" [$t commit] 0
	}

	set chkpt [txn010_stat $env "Time of last checkpoint"]
	tclsleep 1
	error_check_good checkpoint [$env txn_checkpoint -kbyte 2] 0
	set test_chkpt [txn010_stat $env "Time of last checkpoint"]
	error_check_good "kbytes: checkpoint time unchanged" \
	    [expr $test_chkpt > $chkpt] 1

	# Put in a little data and verify that -kbyte doesn't cause a
	# checkpoint
	set chkpt [txn010_stat $env "Time of last checkpoint"]
	for { set count 0 } { $count < 20 } { incr count } {
		set t [$env txn]
		error_check_good "kbyte: put" \
		    [$db put -txn $t "key_d_$count" "data"] 0
		error_check_good "kbyte: commit" [$t commit] 0
		tclsleep 1
		error_check_good checkpoint [$env txn_checkpoint -kbyte 20] 0
		set test_chkpt [txn010_stat $env "Time of last checkpoint"]
		error_check_good "kbytes: checkpoint time changed" \
		    [expr $test_chkpt == $chkpt] 1
	}

	# Test that -min doesn't cause a checkpoint unless enough time has
	# passed.
	puts "\tTxn010.e: test checkpoint -min"
	set t [$env txn]
	error_check_good "min: put" [$db put -txn $t "key_e_$count" "data"] 0
	error_check_good "min: commit" [$t commit] 0
	set chkpt [txn010_stat $env "Time of last checkpoint"]
	for { set count 0 } { $count < 5 } {incr count } {
		tclsleep 1
		error_check_good checkpoint [$env txn_checkpoint -min 2] 0
		set test_chkpt [txn010_stat $env "Time of last checkpoint"]
		error_check_good "min: checkpoint time changed" \
		    [expr $test_chkpt == $chkpt] 1
	}

	# Wait long enough, and then check to see if -min causes a checkpoint.
	set chkpt [txn010_stat $env "Time of last checkpoint"]
	tclsleep 120
	error_check_good checkpoint [$env txn_checkpoint -min 2] 0
	set test_chkpt [txn010_stat $env "Time of last checkpoint"]
	error_check_good "min: checkpoint time unchanged" \
	    [expr $test_chkpt > $chkpt] 1

	# Close down the database and the environment.
	error_check_good db_close [$db close] 0
	error_check_good env_close [$env close] 0
}

# txn010_stat --
#	Return the current log statistics.
proc txn010_stat { env s } {
	set stat [$env txn_stat]
	foreach statpair $stat {
		set statmsg [lindex $statpair 0]
		set statval [lindex $statpair 1]
		if {[is_substr $statmsg $s] != 0} {
			return $statval
		}
	}
	puts "FAIL: Txn010: stat string $s not found"
	return 0
}
