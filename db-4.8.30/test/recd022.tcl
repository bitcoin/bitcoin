# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	recd022
# TEST	Test that pages allocated by an aborted subtransaction
# TEST	within an aborted prepared parent transaction are returned
# TEST 	to the free list after recovery.  This exercises
# TEST	__db_pg_prepare in systems without FTRUNCATE.  [#7403]

proc recd022 { method args} {
	global log_log_record_types
	global fixed_len
	global is_hp_test
	source ./include.tcl

	# Skip test for specified page sizes -- we want to
	# specify our own page size.
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Recd022: Skipping for specific pagesizes"
		return
	}

	# Skip the test for HP-UX, where we can't open an env twice.
	if { $is_hp_test == 1 } {
		puts "Recd022: Skipping for HP-UX."
		return
	}


	# Increase size of fixed-length records to match other methods.
	set orig_fixed_len $fixed_len
	set fixed_len 53
	set opts [convert_args $method $args]
	set omethod [convert_method $method]

	puts "Recd022: ($method) Page allocation and recovery"
	puts "Recd022: with aborted prepared txns and child txns."

	# Create the database and environment.
	env_cleanup $testdir
	set testfile recd022.db

	puts "\tRecd022.a: creating environment"
	# We open the env and database with _noerr so we don't
	# get error messages when cleaning up at the end of the test.
	set env_cmd "berkdb_env_noerr -create -txn -home $testdir"
	set dbenv [eval $env_cmd]
	error_check_good dbenv [is_valid_env $dbenv] TRUE

	# Open database with small pages.
	puts "\tRecd022.b: creating database with small pages"
	set pagesize 512
	set oflags "-create $omethod -mode 0644 -pagesize $pagesize \
	    -env $dbenv -auto_commit $opts $testfile"
	set db [eval {berkdb_open_noerr} $oflags]
	error_check_good db_open [is_valid_db $db] TRUE

	puts "\tRecd022.c: start transaction, put some data"
	set iter 10
	set datasize 53
	set data [repeat "a" $datasize]
	set iter2 [expr $iter * 2]

        # Start parent and child txns.
	puts "\tRecd022.d: start child txn, put some data"
	set parent [$dbenv txn]
	set child1 [$dbenv txn -parent $parent]

	# Child puts some new data.
	for { set i 1 } {$i <= $iter } { incr i } {
		eval {$db put} -txn $child1 $i $data
	}

	# Abort the child txn.
	puts "\tRecd022.e: abort child txn"
	error_check_good child1_abort [$child1 abort] 0

	# Start a second child.  Put some data, enough to allocate
	# a new page, then delete it.
	puts "\tRecd022.f: start second child txn, put some data"
	set child2 [$dbenv txn -parent $parent]
	for { set i 1 } { $i <= $iter2 } { incr i } {
		eval {$db put} -txn $child2 $i $data
	}
	for { set i 1 } { $i <= $iter2 } { incr i } {
		eval {$db del} -txn $child2 $i
	}

	# Put back half the data.
	for { set i 1 } { $i <= $iter } { incr i } {
		eval {$db put} -txn $child2 $i $data
	}

	# Commit second child
	puts "\tRecd022.g: commit second child txn, prepare parent"
	error_check_good child2_commit [$child2 commit] 0

	# Prepare parent
	error_check_good prepare [$parent prepare "ABC"] 0

	# Recover, then abort the recovered parent txn
	puts "\tRecd022.h: recover, then abort parent"
	set env1 [berkdb_env -create -recover -home $testdir -txn]
	set txnlist [$env1 txn_recover]
	set aborttxn [lindex [lindex $txnlist 0] 0]
	error_check_good parent_abort [$aborttxn abort] 0

	# Verify database and then clean up.  We still need to get
	# rid of the handles created before recovery.
	puts "\tRecd022.i: verify and clean up"
	if  { [is_partition_callback $args] == 1 } {
		set nodump 1
	} else {
		set nodump 0
	}
	verify_dir $testdir "" 1 0 $nodump
	set stat [catch {$db close} res]
	error_check_good db_close [is_substr $res "run recovery"] 1
	error_check_good env1_close [$env1 close] 0
	set stat [catch {$dbenv close} res]
	error_check_good dbenv_close [is_substr $res "run recovery"] 1

	# Track the log types we've seen
	if { $log_log_record_types == 1} {
		logtrack_read $testdir
	}

	# Set fixed_len back to the global value so we don't
	# mess up other tests.
	set fixed_len $orig_fixed_len
	return
}
