# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test054
# TEST	Cursor maintenance during key/data deletion.
# TEST
# TEST	This test checks for cursor maintenance in the presence of deletes.
# TEST	There are N different scenarios to tests:
# TEST	 1. No duplicates.  Cursor A deletes a key, do a  GET for the key.
# TEST	 2. No duplicates.  Cursor is positioned right before key K, Delete K,
# TEST	    do a next on the cursor.
# TEST	 3. No duplicates.  Cursor is positioned on key K, do a regular delete
# TEST	    of K, do a current get on K.
# TEST	 4. Repeat 3 but do a next instead of current.
# TEST	 5. Duplicates. Cursor A is on the first item of a duplicate set, A
# TEST	    does a delete.  Then we do a non-cursor get.
# TEST	 6. Duplicates.  Cursor A is in a duplicate set and deletes the item.
# TEST	    do a delete of the entire Key. Test cursor current.
# TEST	 7. Continue last test and try cursor next.
# TEST	 8. Duplicates.  Cursor A is in a duplicate set and deletes the item.
# TEST	    Cursor B is in the same duplicate set and deletes a different item.
# TEST	    Verify that the cursor is in the right place.
# TEST	 9. Cursors A and B are in the place in the same duplicate set.  A
# TEST	    deletes its item.  Do current on B.
# TEST	10. Continue 8 and do a next on B.
proc test054 { method args } {
	global errorInfo
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	append args " -create -mode 0644"
	puts "Test054 ($method $args):\
	    interspersed cursor and normal operations"
	if { [is_record_based $method] == 1 } {
		puts "Test054 skipping for method $method"
		return
	}

	# Find the environment in the argument list, we'll need it
	# later.
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	if { $eindex != -1 } {
		incr eindex
	}

	# Create the database and open the dictionary
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test054-nodup.db
		set env NULL
	} else {
		set testfile test054-nodup.db
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

	puts "\tTest054.a: No Duplicate Tests"
	set db [eval {berkdb_open} $args {$omethod $testfile}]
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

	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set curs [eval {$db cursor} $txn]
	error_check_good curs_open:nodup [is_valid_cursor $curs $db] TRUE

	# Retrieve keys sequentially so we can figure out their order
	set i 1
	for {set d [$curs get -first] } \
	    {[llength $d] != 0 } \
		 {set d [$curs get -next] } {
		set key_set($i) [lindex [lindex $d 0] 0]
		incr i
	}

	# Test case #1.
	puts "\tTest054.a1: Delete w/cursor, regular get"

	# Now set the cursor on the middle on.
	set r [$curs get -set $key_set(2)]
	error_check_bad cursor_get:DB_SET [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_SET:key $k $key_set(2)
	error_check_good curs_get:DB_SET:data $d datum$key_set(2)

	# Now do the delete
	set r [$curs del]
	error_check_good curs_del $r 0

	# Now do the get
	set r [eval {$db get} $txn {$key_set(2)}]
	error_check_good get_after_del [llength $r] 0

	# Free up the cursor.
	error_check_good cursor_close [eval {$curs close}] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}

	# Test case #2.
	puts "\tTest054.a2: Cursor before K, delete K, cursor next"

	# Replace key 2
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set r [eval {$db put} $txn {$key_set(2) datum$key_set(2)}]
	error_check_good put $r 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}

	# Open and position cursor on first item.
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set curs [eval {$db cursor} $txn]
	error_check_good curs_open:nodup [is_valid_cursor $curs $db] TRUE

	# Retrieve keys sequentially so we can figure out their order
	set i 1
	for {set d [eval {$curs get} -first] } \
	    {[llength $d] != 0 } \
		 {set d [$curs get -nextdup] } {
		set key_set($i) [lindex [lindex $d 0] 0]
		incr i
	}

	set r [eval {$curs get} -set {$key_set(1)} ]
	error_check_bad cursor_get:DB_SET [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_SET:key $k $key_set(1)
	error_check_good curs_get:DB_SET:data $d datum$key_set(1)

	# Now delete (next item) $key_set(2)
	error_check_good \
	    db_del:$key_set(2) [eval {$db del} $txn {$key_set(2)}] 0

	# Now do next on cursor
	set r [$curs get -next]
	error_check_bad cursor_get:DB_NEXT [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_NEXT:key $k $key_set(3)
	error_check_good curs_get:DB_NEXT:data $d datum$key_set(3)

	# Test case #3.
	puts "\tTest054.a3: Cursor on K, delete K, cursor current"

	# delete item 3
	error_check_good \
	    db_del:$key_set(3) [eval {$db del} $txn {$key_set(3)}] 0
	# NEEDS TO COME BACK IN, BUG CHECK
	set ret [$curs get -current]
	error_check_good current_after_del $ret ""
	error_check_good cursor_close [$curs close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}

	puts "\tTest054.a4: Cursor on K, delete K, cursor next"

	# Restore keys 2 and 3
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set r [eval {$db put} $txn {$key_set(2) datum$key_set(2)}]
	error_check_good put $r 0
	set r [eval {$db put} $txn {$key_set(3) datum$key_set(3)}]
	error_check_good put $r 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}

	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	# Create the new cursor and put it on 1
	set curs [eval {$db cursor} $txn]
	error_check_good curs_open:nodup [is_valid_cursor $curs $db] TRUE
	set r [$curs get -set $key_set(1)]
	error_check_bad cursor_get:DB_SET [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_SET:key $k $key_set(1)
	error_check_good curs_get:DB_SET:data $d datum$key_set(1)

	# Delete 2
	error_check_good \
	    db_del:$key_set(2) [eval {$db del} $txn {$key_set(2)}] 0

	# Now do next on cursor
	set r [$curs get -next]
	error_check_bad cursor_get:DB_NEXT [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_NEXT:key $k $key_set(3)
	error_check_good curs_get:DB_NEXT:data $d datum$key_set(3)

	# Close cursor
	error_check_good curs_close [$curs close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0

	# Now get ready for duplicate tests

	if { [is_rbtree $method] == 1 || [is_compressed $args] == 1 } {
		puts "Test054: skipping remainder of test for method $method."
		return
	}

	puts "\tTest054.b: Duplicate Tests"
	append args " -dup"

	# Open a new database for the dup tests so -truncate is not needed.
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test054-dup.db
		set env NULL
	} else {
		set testfile test054-dup.db
		set env [lindex $args $eindex]
		set testdir [get_home $env]
	}
	cleanup $testdir $env

	set flags ""
	set txn ""

	set db [eval {berkdb_open} $args {$omethod $testfile}]
	error_check_good db_open:dup [is_valid_db $db] TRUE

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
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set curs [eval {$db cursor} $txn]
	error_check_good curs_open:dup [is_valid_cursor $curs $db] TRUE

	set i 1
	for {set d [$curs get -first] } \
	    {[llength $d] != 0 } \
		 {set d [$curs get -nextdup] } {
		set key_set($i) [lindex [lindex $d 0] 0]
		incr i
	}

	# Now put in a bunch of duplicates for key 2
	for { set d 1 } { $d <= 5 } {incr d} {
		set r [eval {$db put} $txn $flags {$key_set(2) dup_$d}]
		error_check_good dup:put $r 0
	}

	# Test case #5.
	puts "\tTest054.b1: Delete dup w/cursor on first item.  Get on key."

	# Now set the cursor on the first of the duplicate set.
	set r [eval {$curs get} -set {$key_set(2)}]
	error_check_bad cursor_get:DB_SET [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_SET:key $k $key_set(2)
	error_check_good curs_get:DB_SET:data $d datum$key_set(2)

	# Now do the delete
	set r [$curs del]
	error_check_good curs_del $r 0

	# Now do the get
	set r [eval {$db get} $txn {$key_set(2)}]
	error_check_good get_after_del [lindex [lindex $r 0] 1] dup_1

	# Test case #6.
	puts "\tTest054.b2: Now get the next duplicate from the cursor."

	# Now do next on cursor
	set r [$curs get -nextdup]
	error_check_bad cursor_get:DB_NEXT [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_NEXT:key $k $key_set(2)
	error_check_good curs_get:DB_NEXT:data $d dup_1

	# Test case #3.
	puts "\tTest054.b3: Two cursors in set; each delete different items"

	# Open a new cursor.
	set curs2 [eval {$db cursor} $txn]
	error_check_good curs_open [is_valid_cursor $curs2 $db] TRUE

	# Set on last of duplicate set.
	set r [$curs2 get -set $key_set(3)]
	error_check_bad cursor_get:DB_SET [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_SET:key $k $key_set(3)
	error_check_good curs_get:DB_SET:data $d datum$key_set(3)

	set r [$curs2 get -prev]
	error_check_bad cursor_get:DB_PREV [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_PREV:key $k $key_set(2)
	error_check_good curs_get:DB_PREV:data $d dup_5

	# Delete the item at cursor 1 (dup_1)
	error_check_good curs1_del [$curs del] 0

	# Verify curs1 and curs2
	# current should fail
	set ret [$curs get -current]
	error_check_good curs1_get_after_del $ret ""

	set r [$curs2 get -current]
	error_check_bad curs2_get [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_CURRENT:key $k $key_set(2)
	error_check_good curs_get:DB_CURRENT:data $d dup_5

	# Now delete the item at cursor 2 (dup_5)
	error_check_good curs2_del [$curs2 del] 0

	# Verify curs1 and curs2
	set ret [$curs get -current]
	error_check_good curs1_get:del2 $ret ""

	set ret [$curs2 get -current]
	error_check_good curs2_get:del2 $ret ""

	# Now verify that next and prev work.

	set r [$curs2 get -prev]
	error_check_bad cursor_get:DB_PREV [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_PREV:key $k $key_set(2)
	error_check_good curs_get:DB_PREV:data $d dup_4

	set r [$curs get -next]
	error_check_bad cursor_get:DB_NEXT [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_NEXT:key $k $key_set(2)
	error_check_good curs_get:DB_NEXT:data $d dup_2

	puts "\tTest054.b4: Two cursors same item, one delete, one get"

	# Move curs2 onto dup_2
	set r [$curs2 get -prev]
	error_check_bad cursor_get:DB_PREV [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_PREV:key $k $key_set(2)
	error_check_good curs_get:DB_PREV:data $d dup_3

	set r [$curs2 get -prev]
	error_check_bad cursor_get:DB_PREV [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_PREV:key $k $key_set(2)
	error_check_good curs_get:DB_PREV:data $d dup_2

	# delete on curs 1
	error_check_good curs1_del [$curs del] 0

	# Verify gets on both 1 and 2
	set ret [$curs get -current]
	error_check_good \
	    curs1_get:deleted $ret ""
	set ret [$curs2 get -current]
	error_check_good \
	    curs2_get:deleted $ret ""

	puts "\tTest054.b5: Now do a next on both cursors"

	set r [$curs get -next]
	error_check_bad cursor_get:DB_NEXT [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_NEXT:key $k $key_set(2)
	error_check_good curs_get:DB_NEXT:data $d dup_3

	set r [$curs2 get -next]
	error_check_bad cursor_get:DB_NEXT [llength $r] 0
	set k [lindex [lindex $r 0] 0]
	set d [lindex [lindex $r 0] 1]
	error_check_good curs_get:DB_NEXT:key $k $key_set(2)
	error_check_good curs_get:DB_NEXT:data $d dup_3

	# Close cursor
	error_check_good curs_close [$curs close] 0
	error_check_good curs2_close [$curs2 close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0
}
