# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test023
# TEST	Duplicate test
# TEST	Exercise deletes and cursor operations within a duplicate set.
# TEST	Add a key with duplicates (first time on-page, second time off-page)
# TEST	Number the dups.
# TEST	Delete dups and make sure that CURRENT/NEXT/PREV work correctly.
proc test023 { method args } {
	global alphabet
	global dupnum
	global dupstr
	global errorInfo
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	# Btree with compression does not support unsorted duplicates.
	if { [is_compressed $args] == 1 } {
		puts "Test023 skipping for btree with compression."
		return
	}

	puts "Test023: $method delete duplicates/check cursor operations"
	if { [is_record_based $method] == 1 || \
	    [is_rbtree $method] == 1 } {
		puts "Test023: skipping for method $omethod"
		return
	}

	# Create the database and open the dictionary
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test023.db
		set env NULL
	} else {
		set testfile test023.db
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set testdir [get_home $env]
	}
	set t1 $testdir/t1
	cleanup $testdir $env
	set db [eval {berkdb_open \
	    -create -mode 0644 -dup} $args {$omethod $testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE

	set pflags ""
	set gflags ""
	set txn ""

	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good db_cursor [is_valid_cursor $dbc $db] TRUE

	foreach i { onpage offpage } {
		if { $i == "onpage" } {
			set dupstr DUP
		} else {
			set dupstr [repeat $alphabet 50]
		}
		puts "\tTest023.a: Insert key w/$i dups"
		set key "duplicate_val_test"
		for { set count 0 } { $count < 20 } { incr count } {
			set ret \
			    [eval {$db put} $txn $pflags {$key $count$dupstr}]
			error_check_good db_put $ret 0
		}

		# Now let's get all the items and make sure they look OK.
		puts "\tTest023.b: Check initial duplicates"
		set dupnum 0
		dump_file $db $txn $t1 test023.check

		# Delete a couple of random items (FIRST, LAST one in middle)
		# Make sure that current returns an error and that NEXT and
		# PREV do the right things.

		set ret [$dbc get -set $key]
		error_check_bad dbc_get:SET [llength $ret] 0

		puts "\tTest023.c: Delete first and try gets"
		# This should be the first duplicate
		error_check_good \
		    dbc_get:SET $ret [list [list duplicate_val_test 0$dupstr]]

		# Now delete it.
		set ret [$dbc del]
		error_check_good dbc_del:FIRST $ret 0

		# Now current should fail
		set ret [$dbc get -current]
		error_check_good dbc_get:CURRENT $ret ""

		# Now Prev should fail
		set ret [$dbc get -prev]
		error_check_good dbc_get:prev0 [llength $ret] 0

		# Now 10 nexts should work to get us in the middle
		for { set j 1 } { $j <= 10 } { incr j } {
			set ret [$dbc get -next]
			error_check_good \
			    dbc_get:next [llength [lindex $ret 0]] 2
			error_check_good \
			    dbc_get:next [lindex [lindex $ret 0] 1] $j$dupstr
		}

		puts "\tTest023.d: Delete middle and try gets"
		# Now do the delete on the current key.
		set ret [$dbc del]
		error_check_good dbc_del:10 $ret 0

		# Now current should fail
		set ret [$dbc get -current]
		error_check_good dbc_get:deleted $ret ""

		# Prev and Next should work
		set ret [$dbc get -next]
		error_check_good dbc_get:next [llength [lindex $ret 0]] 2
		error_check_good \
		    dbc_get:next [lindex [lindex $ret 0] 1] 11$dupstr

		set ret [$dbc get -prev]
		error_check_good dbc_get:next [llength [lindex $ret 0]] 2
		error_check_good \
		    dbc_get:next [lindex [lindex $ret 0] 1] 9$dupstr

		# Now go to the last one
		for { set j 11 } { $j <= 19 } { incr j } {
			set ret [$dbc get -next]
			error_check_good \
			    dbc_get:next [llength [lindex $ret 0]] 2
			error_check_good \
			    dbc_get:next [lindex [lindex $ret 0] 1] $j$dupstr
		}

		puts "\tTest023.e: Delete last and try gets"
		# Now do the delete on the current key.
		set ret [$dbc del]
		error_check_good dbc_del:LAST $ret 0

		# Now current should fail
		set ret [$dbc get -current]
		error_check_good dbc_get:deleted $ret ""

		# Next should fail
		set ret [$dbc get -next]
		error_check_good dbc_get:next19 [llength $ret] 0

		# Prev should work
		set ret [$dbc get -prev]
		error_check_good dbc_get:next [llength [lindex $ret 0]] 2
		error_check_good \
		    dbc_get:next [lindex [lindex $ret 0] 1] 18$dupstr

		# Now overwrite the current one, then count the number
		# of data items to make sure that we have the right number.

		puts "\tTest023.f: Count keys, overwrite current, count again"
		# At this point we should have 17 keys the (initial 20 minus
		# 3 deletes)
		set dbc2 [eval {$db cursor} $txn]
		error_check_good db_cursor:2 [is_substr $dbc2 $db] 1

		set count_check 0
		for { set rec [$dbc2 get -first] } {
		    [llength $rec] != 0 } { set rec [$dbc2 get -next] } {
			incr count_check
		}
		error_check_good numdups $count_check 17

		set ret [$dbc put -current OVERWRITE]
		error_check_good dbc_put:current $ret 0

		set count_check 0
		for { set rec [$dbc2 get -first] } {
		    [llength $rec] != 0 } { set rec [$dbc2 get -next] } {
			incr count_check
		}
		error_check_good numdups $count_check 17
		error_check_good dbc2_close [$dbc2 close] 0

		# Done, delete all the keys for next iteration
		set ret [eval {$db del} $txn {$key}]
		error_check_good db_delete $ret 0

		# database should be empty

		set ret [$dbc get -first]
		error_check_good first_after_empty [llength $ret] 0
	}

	error_check_good dbc_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0

}

# Check function for test023; keys and data are identical
proc test023.check { key data } {
	global dupnum
	global dupstr
	error_check_good "bad key" $key duplicate_val_test
	error_check_good "data mismatch for $key" $data $dupnum$dupstr
	incr dupnum
}
