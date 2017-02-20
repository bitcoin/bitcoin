# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test066
# TEST	Test of cursor overwrites of DB_CURRENT w/ duplicates.
# TEST
# TEST	Make sure a cursor put to DB_CURRENT acts as an overwrite in a
# TEST	database with duplicates.
proc test066 { method args } {
	set omethod [convert_method $method]
	set args [convert_args $method $args]

	set tnum "066"

	if { [is_record_based $method] || [is_rbtree $method] } {
	    puts "Test$tnum: Skipping for method $method."
	    return
	}

	# Btree with compression does not support unsorted duplicates.
	if { [is_compressed $args] == 1 } {
		puts "Test$tnum skipping for btree with compression."
		return
	}
	puts "Test$tnum: Test of cursor put to DB_CURRENT with duplicates."

	source ./include.tcl

	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test066.db
		set env NULL
	} else {
		set testfile test066.db
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set testdir [get_home $env]
	}
	cleanup $testdir $env

	set txn ""
	set key "test"
	set data "olddata"

	set db [eval {berkdb_open -create -mode 0644 -dup} $omethod $args \
	    $testfile]
	error_check_good db_open [is_valid_db $db] TRUE

	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set ret [eval {$db put} $txn {$key [chop_data $method $data]}]
	error_check_good db_put $ret 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}

	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good db_cursor [is_valid_cursor $dbc $db] TRUE

	set ret [$dbc get -first]
	error_check_good db_get $ret [list [list $key [pad_data $method $data]]]

	set newdata "newdata"
	set ret [$dbc put -current [chop_data $method $newdata]]
	error_check_good dbc_put $ret 0

	# There should be only one (key,data) pair in the database, and this
	# is it.
	set ret [$dbc get -first]
	error_check_good db_get_first $ret \
	    [list [list $key [pad_data $method $newdata]]]

	# and this one should come up empty.
	set ret [$dbc get -next]
	error_check_good db_get_next $ret ""

	error_check_good dbc_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0

	puts "\tTest$tnum: Test completed successfully."
}
