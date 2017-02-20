# See the file LICENSE for redistribution information.
#
# Copyright (c) 2006-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test121
# TEST	Tests of multi-version concurrency control.
# TEST
# TEST	MVCC and cursor adjustment.
# TEST	Set up a -snapshot cursor and position it in the middle
# TEST 	of a database.
# TEST  Write to the database, both before and after the cursor,
# TEST	and verify that it stays on the same position.

proc test121 { method {tnum "121"} args } {
	source ./include.tcl

	# This test needs its own env.
	set eindex [lsearch -exact $args "-env"]
	if { $eindex != -1 } {
		incr eindex
		set env [lindex $args $eindex]
		puts "Test$tnum skipping for env $env"
		return
	}

	# MVCC is not allowed with queue methods.
	if { [is_queue $method] == 1 } {
		puts "Test$tnum skipping for method $method"
		return
	}

	puts "\tTest$tnum ($method): MVCC and cursor adjustment."

	set args [convert_args $method $args]
	set omethod [convert_method $method]
	set encargs ""
	set args [split_encargs $args encargs]
	set filename "test.db"
	set pageargs ""
	set args [split_pageargs $args pageargs]

	# Create transactional env.  Specifying -multiversion makes
	# all databases opened within the env -multiversion.

	env_cleanup $testdir
	puts "\tTest$tnum.a: Creating txn env."

	# Raise cachesize so this test focuses on cursor adjustment
	# and not on small cache issues.
	set cachesize [expr 2 * 1024 * 1024]
	set max_locks 2000
	set max_objects 2000
	set env [eval {berkdb_env -create -cachesize "0 $cachesize 1"}\
	    -lock_max_locks $max_locks -lock_max_objects $max_objects\
	    -txn -multiversion $encargs $pageargs -home $testdir]
	error_check_good env_open [is_valid_env $env] TRUE

	# Open database.
	puts "\tTest$tnum.b: Creating -multiversion db."
	set db [eval {berkdb_open} \
	    -create -auto_commit -env $env $omethod $args $pageargs $filename]
	error_check_good db_open [is_valid_db $db] TRUE

	# Start transactions.
	puts "\tTest$tnum.c: Start txns with -snapshot."
	set t1 [$env txn -snapshot]
	set txn1 "-txn $t1"

	# Enter some data using txn1.  Leave holes, by using keys
	# 2, 4, 6 ....
	set niter 10000
	set data DATA
	for { set i 1 } { $i <= $niter } { incr i } {
		set key [expr $i * 2]
		error_check_good t1_put [eval {$db put} $txn1 $key $data.$key] 0
	}
	error_check_good t1_commit [$t1 commit] 0

	# Open a read-only cursor.
	set t2 [$env txn -snapshot]
	set txn2 "-txn $t2"
	set cursor [eval {$db cursor} $txn2]
	error_check_good db_cursor [is_valid_cursor $cursor $db] TRUE

	# Walk the cursor halfway through the database.
	set i 1
	set halfway [expr $niter / 2]
	for { set ret [$cursor get -first] } \
	    { $i <= $halfway } \
	    { set ret [$cursor get -next] } {
		incr i
	}

	set currentkey [lindex [lindex $ret 0] 0]
	set currentdata [lindex [lindex $ret 0] 1]

	# Start a new transaction and use it to enter more data.
	# Verify that the cursor is not changed.
	puts "\tTest$tnum.c: Enter more data."
	set t1 [$env txn -snapshot]
	set txn1 "-txn $t1"

	# Enter more data, filling in the holes from the first
	# time around by using keys 1, 3, 5 ....  Cursor should
	# stay on the same item.
	for { set i 1 } { $i <= $niter } { incr i } {
		set key [expr [expr $i * 2] - 1]
		error_check_good t1_put [eval {$db put} $txn1 $key $data.$key] 0
		set ret [$cursor get -current]
		set k [lindex [lindex $ret 0] 0]
		set d [lindex [lindex $ret 0] 1]
		error_check_good current_key $k $currentkey
		error_check_good current_data $d $currentdata
	}

	error_check_good t1_commit [$t1 commit] 0
	error_check_good cursor_close [$cursor close] 0
	error_check_good t2_commit [$t2 commit] 0

	# Clean up.
	error_check_good db_close [$db close] 0
	error_check_good env_close [$env close] 0
}
