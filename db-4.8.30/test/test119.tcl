# See the file LICENSE for redistribution information.
#
# Copyright (c) 2003-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test119
# TEST	Test behavior when Berkeley DB returns DB_BUFFER_SMALL on a cursor.
# TEST
# TEST	If the user-supplied buffer is not large enough to contain
# TEST	the returned value, DB returns BUFFER_SMALL.  If it does,
# TEST	check that the cursor does not move -- if it moves, it will
# TEST	skip items. [#13815]

proc test119 { method {tnum "119"} args} {
	source ./include.tcl
	global alphabet
	global errorCode

	set args [convert_args $method $args]
	set omethod [convert_method $method]
	puts "Test$tnum: $method ($args) Test of DB_BUFFER_SMALL."

	# Skip for queue; it has fixed-length records, so overflowing
	# the buffer isn't possible with an ordinary get.
	if { [is_queue $method] == 1 } {
		puts "Skipping test$tnum for method $method"
		return
	}

	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	set txnenv 0
	set txn ""
	set eindex [lsearch -exact $args "-env"]
	if { $eindex == -1 } {
		set testfile $testdir/test$tnum.db
		set env NULL
	} else {
		set testfile test$tnum.db
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set testdir [get_home $env]
	}

	cleanup $testdir $env

	puts "\tTest$tnum.a: Set up database."
	set db [eval \
	    {berkdb_open_noerr -create -mode 0644} $args $omethod $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	# Test -data_buf_size with db->get.
	puts "\tTest$tnum.b: Test db get with -data_buf_size."
	set datalength 20
	set data [repeat "a" $datalength]
	set key 1

	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}

	error_check_good db_put \
	    [eval {$db put} $txn {$key [chop_data $method $data]}] 0

	# A get with data_buf_size equal to the data size should work.
	set ret [eval {$db get} $txn -data_buf_size $datalength $key]
	error_check_good db_get_key [lindex [lindex $ret 0] 0] $key
	error_check_good db_get_data [lindex [lindex $ret 0] 1] $data

	# A get with a data_buf_size decreased by one should fail.
	catch {eval {$db get}\
	    $txn -data_buf_size [expr $datalength - 1] $key} res
	error_check_good buffer_small_error [is_substr $res DB_BUFFER_SMALL] 1

	# Delete the item so it won't get in the way of the cursor test.
	error_check_good db_del [eval {$db del} $txn $key] 0
	if { $txnenv == 1 } {
		error_check_good txn_commit [$t commit] 0
	}

	# Test -data_buf_size and -key_buf_size with dbc->get.
	#
	# Set up a database that includes large and small keys and
	# large and small data in various combinations.
	#
	# Create small buffer equal to the largest page size.  This will
	# get DB_BUFFER_SMALL errors.
	# Create big buffer large enough to never get DB_BUFFER_SMALL
	# errors with this data set.

	puts "\tTest$tnum.c:\
	    Test cursor get with -data_buf_size and -key_buf_size."
	set key $alphabet
	set data $alphabet
	set nentries 100
	set start 100
	set bigkey [repeat $key 8192]
	set bigdata [repeat $data 8192]
	set buffer [expr 64 * 1024]
	set bigbuf [expr $buffer * 8]

	puts "\tTest$tnum.c1: Populate database."
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}

	# Put in a big key every X data items, and big data every
	# Y data items.  X and Y should be small enough that we
	# hit the case where both X and Y are big.
	set x 5
	set y 7
	for { set i $start } { $i < [expr $nentries + $start] } { incr i } {
		# If we have a record-based method, we can't have big keys.
		# Just use the count.
		if { [is_record_based $method] == 1 } {
			set k $i
		} else {
			if { [expr $i % $x] == 1 } {
				set k $i.$bigkey
			} else {
				set k $i.$key
			}
		}

		# We can have big data on any method.
		if { [expr $i % $y] == 1 } {
			set d $i.$bigdata
		} else {
			set d $i.$data
		}
		error_check_good db_put \
		    [eval {$db put} $txn {$k [chop_data $method $d]}] 0
	}
	if { $txnenv == 1 } {
		error_check_good txn_commit [$t commit] 0
	}

	# Walk the database with a cursor.  When we hit DB_BUFFER_SMALL,
	# make sure DB returns the appropriate key/data pair.
	puts "\tTest$tnum.c2: Walk the database with a cursor."
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set curs [eval {$db cursor} $txn]
	error_check_good cursor [is_valid_cursor $curs $db] TRUE

	# Since hash is not sorted, we'll test that no items are
	# skipped by keeping a list of all items retrieved, and
	# making sure it is complete and that each item is unique
	# at the end of the test.
	set hashitems {}

	set count $start
	for { set kd [catch {eval $curs get \
	    -key_buf_size $buffer -data_buf_size $buffer -first} res] } \
	    { $count < [expr $nentries + $start] } \
	    { set kd [catch {eval $curs get \
	    -key_buf_size $buffer -data_buf_size $buffer -next} res] } {
		if { $kd == 1 } {
			# Make sure we have the expected error.
			error_check_good buffer_small_error \
			    [is_substr $errorCode DB_BUFFER_SMALL] 1

			# Adjust the buffer sizes to fit the big key or data.
			if { [expr $count % $x] == 1 } {
				set key_buf $bigbuf
			} else {
				set key_buf $buffer
			}
			if { [expr $count % $y] == 1 } {
				set data_buf $bigbuf
			} else {
				set data_buf $buffer
			}

			# Hash is not sorted, so just make sure we can get
			# the item with a large buffer and check it later.
			# Likewise for partition callback.
			if { [is_hash $method] == 1  || \
			    [is_partition_callback $args] == 1} {
				set data_buf $bigbuf
				set key_buf $bigbuf
			}

			# Retrieve with big buffer; there should be no error.
			# This also walks the cursor forward.
			set nextbig [catch {eval $curs get -key_buf_size \
			    $key_buf -data_buf_size $data_buf -next} res]
			error_check_good data_big_buffer_get $nextbig 0

			# Extract the item number.
			set key [lindex [lindex $res 0] 0]
			set data [lindex [lindex $res 0] 1]
			if { [string first . $key] != -1 } {
				set keyindex [string first . $key]
				set keynumber \
				    [string range $key 0 [expr $keyindex - 1]]
			} else {
				set keynumber $key
			}
			set dataindex [string first . $data]
			set datanumber \
			    [string range $data 0 [expr $dataindex - 1]]

			# If not hash, check that item number is correct.
			# If hash, save the number for later verification.
			if { [is_hash $method] == 0 \
				&& [is_partition_callback $args] == 0 } {
				error_check_good key_number $keynumber $count
				error_check_good data_number $datanumber $count
			} else {
				lappend hashitems $keynumber
			}
		} else {
			# For hash, save the item numbers of all items
			# retrieved, not just those returning DB_BUFFER_SMALL.
			if { [is_hash $method] == 1  || \
			    [is_partition_callback $args] == 1} {
				set key [lindex [lindex $res 0] 0]
				set keyindex [string first . $key]
				set keynumber \
				    [string range $key 0 [expr $keyindex - 1]]
				lappend hashitems $keynumber
			}
		}
		incr count
		set errorCode NONE
	}
	error_check_good curs_close [$curs close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}

	# Now check the list of items retrieved from hash.
	if { [is_hash $method] == 1  || \
	    [is_partition_callback $args] == 1} {
		set sortedhashitems [lsort $hashitems]
		for { set i $start } \
		    { $i < [expr $nentries + $start] } { incr i } {
			set hashitem \
			    [lindex $sortedhashitems [expr $i - $start]]
			error_check_good hash_check $hashitem $i
		}
	}
	error_check_good db_close [$db close] 0
}

