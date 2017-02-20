# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test059
# TEST	Cursor ops work with a partial length of 0.
# TEST	Make sure that we handle retrieves of zero-length data items correctly.
# TEST	The following ops, should allow a partial data retrieve of 0-length.
# TEST	db_get
# TEST	db_cget FIRST, NEXT, LAST, PREV, CURRENT, SET, SET_RANGE
proc test059 { method args } {
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	puts "Test059: $method 0-length partial data retrieval"

	# Create the database and open the dictionary
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test059.db
		set env NULL
	} else {
		set testfile test059.db
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set testdir [get_home $env]
	}
	cleanup $testdir $env

	set pflags ""
	set gflags ""
	set txn ""
	set count 0

	if { [is_record_based $method] == 1 } {
		append gflags " -recno"
	}

	puts "\tTest059.a: Populate a database"
	set oflags "-create -mode 0644 $omethod $args $testfile"
	set db [eval {berkdb_open} $oflags]
	error_check_good db_create [is_substr $db db] 1

	# Put ten keys in the database
	for { set key 1 } { $key <= 10 } {incr key} {
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set r [eval {$db put} $txn $pflags {$key datum$key}]
		error_check_good put $r 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
	}

	# Retrieve keys sequentially so we can figure out their order
	set i 1
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set curs [eval {$db cursor} $txn]
	error_check_good db_curs [is_valid_cursor $curs $db] TRUE

	for {set d [$curs get -first] } { [llength $d] != 0 } {
	    set d [$curs get -next] } {
		set key_set($i) [lindex [lindex $d 0] 0]
		incr i
	}

	puts "\tTest059.a: db get with 0 partial length retrieve"

	# Now set the cursor on the middle one.
	set ret [eval {$db get -partial {0 0}} $txn $gflags {$key_set(5)}]
	error_check_bad db_get_0 [llength $ret] 0

	puts "\tTest059.a: db cget FIRST with 0 partial length retrieve"
	set ret [$curs get -first -partial {0 0}]
	set data [lindex [lindex $ret 0] 1]
	set key [lindex [lindex $ret 0] 0]
	error_check_good key_check_first $key $key_set(1)
	error_check_good db_cget_first [string length $data] 0

	puts "\tTest059.b: db cget NEXT with 0 partial length retrieve"
	set ret [$curs get -next -partial {0 0}]
	set data [lindex [lindex $ret 0] 1]
	set key [lindex [lindex $ret 0] 0]
	error_check_good key_check_next $key $key_set(2)
	error_check_good db_cget_next [string length $data] 0

	puts "\tTest059.c: db cget LAST with 0 partial length retrieve"
	set ret [$curs get -last -partial {0 0}]
	set data [lindex [lindex $ret 0] 1]
	set key [lindex [lindex $ret 0] 0]
	error_check_good key_check_last $key $key_set(10)
	error_check_good db_cget_last [string length $data] 0

	puts "\tTest059.d: db cget PREV with 0 partial length retrieve"
	set ret [$curs get -prev -partial {0 0}]
	set data [lindex [lindex $ret 0] 1]
	set key [lindex [lindex $ret 0] 0]
	error_check_good key_check_prev $key $key_set(9)
	error_check_good db_cget_prev [string length $data] 0

	puts "\tTest059.e: db cget CURRENT with 0 partial length retrieve"
	set ret [$curs get -current -partial {0 0}]
	set data [lindex [lindex $ret 0] 1]
	set key [lindex [lindex $ret 0] 0]
	error_check_good key_check_current $key $key_set(9)
	error_check_good db_cget_current [string length $data] 0

	puts "\tTest059.f: db cget SET with 0 partial length retrieve"
	set ret [$curs get -set -partial {0 0} $key_set(7)]
	set data [lindex [lindex $ret 0] 1]
	set key [lindex [lindex $ret 0] 0]
	error_check_good key_check_set $key $key_set(7)
	error_check_good db_cget_set [string length $data] 0

	if {[is_btree $method] == 1} {
		puts "\tTest059.g:\
		    db cget SET_RANGE with 0 partial length retrieve"
		set ret [$curs get -set_range -partial {0 0} $key_set(5)]
		set data [lindex [lindex $ret 0] 1]
		set key [lindex [lindex $ret 0] 0]
		error_check_good key_check_set $key $key_set(5)
		error_check_good db_cget_set [string length $data] 0
	}

	error_check_good curs_close [$curs close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0
}
