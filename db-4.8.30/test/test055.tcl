# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test055
# TEST	Basic cursor operations.
# TEST	This test checks basic cursor operations.
# TEST	There are N different scenarios to tests:
# TEST	1. (no dups) Set cursor, retrieve current.
# TEST	2. (no dups) Set cursor, retrieve next.
# TEST	3. (no dups) Set cursor, retrieve prev.
proc test055 { method args } {
	global errorInfo
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	puts "Test055: $method interspersed cursor and normal operations"

	# Create the database and open the dictionary
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test055.db
		set env NULL
	} else {
		set testfile test055.db
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set testdir [get_home $env]
	}
	cleanup $testdir $env

	set flags ""
	set txn ""

	puts "\tTest055.a: No duplicates"
	set db [eval {berkdb_open -create -mode 0644 $omethod } \
	    $args {$testfile}]
	error_check_good db_open:nodup [is_valid_db $db] TRUE

	# Put three keys in the database
	for { set key 1 } { $key <= 3 } {incr key} {
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set r [eval {$db put} $txn $flags {$key datum$key}]
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
	error_check_good curs_open:nodup [is_valid_cursor $curs $db] TRUE

	for {set d [$curs get -first] } { [llength $d] != 0 } {\
		set d [$curs get -next] } {
		set key_set($i) [lindex [lindex $d 0] 0]
		incr i
	}

	# Test case #1.
	puts "\tTest055.a1: Set cursor, retrieve current"

	# Now set the cursor on the middle on.
	set r [$curs get -set $key_set(2)]
	error_check_bad cursor_get:DB_SET [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_SET:key $k $key_set(2)
	error_check_good \
	    curs_get:DB_SET:data $d [pad_data $method datum$key_set(2)]

	# Now retrieve current
	set r [$curs get -current]
	error_check_bad cursor_get:DB_CURRENT [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_CURRENT:key $k $key_set(2)
	error_check_good \
	    curs_get:DB_CURRENT:data $d [pad_data $method datum$key_set(2)]

	# Test case #2.
	puts "\tTest055.a2: Set cursor, retrieve previous"
	set r [$curs get -prev]
	error_check_bad cursor_get:DB_PREV [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_PREV:key $k $key_set(1)
	error_check_good \
	    curs_get:DB_PREV:data $d [pad_data $method datum$key_set(1)]

	# Test case #3.
	puts "\tTest055.a2: Set cursor, retrieve next"

	# Now set the cursor on the middle one.
	set r [$curs get -set $key_set(2)]
	error_check_bad cursor_get:DB_SET [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_SET:key $k $key_set(2)
	error_check_good \
	    curs_get:DB_SET:data $d [pad_data $method datum$key_set(2)]

	# Now retrieve next
	set r [$curs get -next]
	error_check_bad cursor_get:DB_NEXT [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_NEXT:key $k $key_set(3)
	error_check_good \
	    curs_get:DB_NEXT:data $d [pad_data $method datum$key_set(3)]

	# Close cursor and database.
	error_check_good curs_close [$curs close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0
}
