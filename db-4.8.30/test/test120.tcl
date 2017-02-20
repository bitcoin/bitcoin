# See the file LICENSE for redistribution information.
#
# Copyright (c) 2006-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test120
# TEST	Test of multi-version concurrency control.
# TEST
# TEST	Test basic functionality: a snapshot transaction started
# TEST	before a regular transaction's put can't see the modification.
# TEST	A snapshot transaction started after the put can see it.

proc test120 { method {tnum "120"} args } {
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

	puts "\tTest$tnum ($method): MVCC and blocking."

	set args [convert_args $method $args]
	set omethod [convert_method $method]
	set encargs ""
	set args [split_encargs $args encargs]
	set pageargs ""
	split_pageargs $args pageargs
	set filename "test.db"

	# Create transactional env.  Specifying -multiversion makes
	# all databases opened within the env -multiversion.
	env_cleanup $testdir
	puts "\tTest$tnum.a: Creating txn env."
	set env [eval {berkdb_env}\
	    -create -txn -multiversion $pageargs $encargs -home $testdir]
	error_check_good env_open [is_valid_env $env] TRUE

	# Open database.
	puts "\tTest$tnum.b: Creating -multiversion db."
	set db [eval {berkdb_open} \
	    -create -auto_commit -env $env $omethod $args $filename]
	error_check_good db_open [is_valid_db $db] TRUE

	puts "\tTest$tnum.c: Start transactions."
	# Start two transactions.  T1 is the writer, so it's a regular
	# transaction.  T2 is the reader and uses -snapshot.
	set t1 [$env txn]
	set txn1 "-txn $t1"
	set t2 [$env txn -snapshot]
	set txn2 "-txn $t2"

	# Enter some data using txn1.
	set key 1
	set data DATA
	error_check_good \
	    t1_put [eval {$db put} $txn1 $key [chop_data $method $data]] 0

	# Txn2 cannot see txn1's put, but it does not block.
	puts "\tTest$tnum.d: Txn2 can't see txn1's put."
	set ret [eval {$db get} $txn2 $key]
	error_check_good txn2_get [llength $ret] 0

	# Commit txn1.  Txn2 get still can't see txn1's put.
	error_check_good t1_commit [$t1 commit] 0
	set ret [eval {$db get} $txn2 $key]
	error_check_good txn2_get [llength $ret] 0
	error_check_good db_sync [$db sync] 0
	set ret [eval {$db get} $txn2 $key]
	error_check_good txn2_get [llength $ret] 0

	# Start a new txn with -snapshot.  It can see the put.
	puts "\tTest$tnum.e: A new txn can see txn1's put."
	set t3 [$env txn -snapshot]
	set txn3 "-txn $t3"
	set ret [eval {$db get} $txn3 $key]
	error_check_good \
	    t3_get $ret [list [list $key [pad_data $method $data]]]

	# Commit txns.
	error_check_good t2_commit [$t2 commit] 0
	error_check_good t3_commit [$t3 commit] 0

	# Clean up.
	error_check_good db_close [$db close] 0
	error_check_good env_close [$env close] 0
}
