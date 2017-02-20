# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test056
# TEST	Cursor maintenance during deletes.
# TEST	Check if deleting a key when a cursor is on a duplicate of that
# TEST	key works.
proc test056 { method args } {
	global errorInfo
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	append args " -create -mode 0644 -dup "
	if { [is_record_based $method] == 1 || [is_rbtree $method] } {
		puts "Test056: skipping for method $method"
		return
	}
	# Btree with compression does not support unsorted duplicates.
	if { [is_compressed $args] == 1 } {
		puts "Test056 skipping for btree with compression."
		return
	}

	puts "Test056: $method delete of key in presence of cursor"

	# Create the database and open the dictionary
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test056.db
		set env NULL
	} else {
		set testfile test056.db
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
	set txn  ""

	set db [eval {berkdb_open} $args {$omethod $testfile}]
	error_check_good db_open:dup [is_valid_db $db] TRUE

	puts "\tTest056.a: Key delete with cursor on duplicate."
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
	error_check_good curs_open:dup [is_valid_cursor $curs $db] TRUE

	for {set d [$curs get -first] } { [llength $d] != 0 } {
	    set d [$curs get -next] } {
		set key_set($i) [lindex [lindex $d 0] 0]
		incr i
	}

	# Now put in a bunch of duplicates for key 2
	for { set d 1 } { $d <= 5 } {incr d} {
		set r [eval {$db put} $txn $flags {$key_set(2) dup_$d}]
		error_check_good dup:put $r 0
	}

	# Now put the cursor on a duplicate of key 2

	# Now set the cursor on the first of the duplicate set.
	set r [$curs get -set $key_set(2)]
	error_check_bad cursor_get:DB_SET [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_SET:key $k $key_set(2)
	error_check_good curs_get:DB_SET:data $d datum$key_set(2)

	# Now do two nexts
	set r [$curs get -next]
	error_check_bad cursor_get:DB_NEXT [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_NEXT:key $k $key_set(2)
	error_check_good curs_get:DB_NEXT:data $d dup_1

	set r [$curs get -next]
	error_check_bad cursor_get:DB_NEXT [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_NEXT:key $k $key_set(2)
	error_check_good curs_get:DB_NEXT:data $d dup_2

	# Now do the delete
	set r [eval {$db del} $txn $flags {$key_set(2)}]
	error_check_good delete $r 0

	# Now check the get current on the cursor.
	set ret [$curs get -current]
	error_check_good curs_after_del $ret ""

	# Now check that the rest of the database looks intact.  There
	# should be only two keys, 1 and 3.

	set r [$curs get -first]
	error_check_bad cursor_get:DB_FIRST [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_FIRST:key $k $key_set(1)
	error_check_good curs_get:DB_FIRST:data $d datum$key_set(1)

	set r [$curs get -next]
	error_check_bad cursor_get:DB_NEXT [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_NEXT:key $k $key_set(3)
	error_check_good curs_get:DB_NEXT:data $d datum$key_set(3)

	set r [$curs get -next]
	error_check_good cursor_get:DB_NEXT [llength $r] 0

	puts "\tTest056.b:\
	    Cursor delete of first item, followed by cursor FIRST"
	# Set to beginning
	set r [$curs get -first]
	error_check_bad cursor_get:DB_FIRST [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_FIRST:key $k $key_set(1)
	error_check_good curs_get:DB_FIRST:data $d datum$key_set(1)

	# Now do delete
	error_check_good curs_del [$curs del] 0

	# Now do DB_FIRST
	set r [$curs get -first]
	error_check_bad cursor_get:DB_FIRST [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_FIRST:key $k $key_set(3)
	error_check_good curs_get:DB_FIRST:data $d datum$key_set(3)

	error_check_good curs_close [$curs close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0
}
