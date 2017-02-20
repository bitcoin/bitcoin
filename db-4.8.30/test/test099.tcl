# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test099
# TEST
# TEST	Test of DB->get and DBC->c_get with set_recno and get_recno.
# TEST
# TEST	Populate a small btree -recnum database.
# TEST	After all are entered, retrieve each using -recno with DB->get.
# TEST	Open a cursor and do the same for DBC->c_get with set_recno.
# TEST	Verify that set_recno sets the record number position properly.
# TEST	Verify that get_recno returns the correct record numbers.
# TEST
# TEST	Using the same database, open 3 cursors and position one at
# TEST	the beginning, one in the middle, and one at the end.  Delete
# TEST	by cursor and check that record renumbering is done properly.
#
proc test099 { method {nentries 10000} args } {
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	puts "Test099: Test of set_recno and get_recno in DBC->c_get."
	if { [is_rbtree $method] != 1 } {
		puts "Test099: skipping for method $method."
		return
	}

	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test099.db
		set env NULL
	} else {
		set testfile test099.db
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
			#
			# If we are using txns and running with the
			# default, set the default down a bit.
			#
			if { $nentries == 10000 } {
				set nentries 100
			}
		}
		set testdir [get_home $env]
	}
	set t1 $testdir/t1
	cleanup $testdir $env

	# Create the database and open the dictionary
	set db [eval {berkdb_open \
	     -create -mode 0644} $args {$omethod $testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE

	set did [open $dict]

	set pflags ""
	set gflags ""
	set txn ""
	set count 1

	append gflags " -recno"

	puts "\tTest099.a: put loop"
	# Here is the loop where we put each key/data pair
	while { [gets $did str] != -1 && $count <= $nentries } {
		set key $str
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set r [eval {$db put} \
		    $txn $pflags {$key [chop_data $method $str]}]
		error_check_good db_put $r 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
		incr count
	}
	close $did

	puts "\tTest099.b: dump file"
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	dump_file $db $txn $t1 test099.check
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0

	puts "\tTest099.c: Test set_recno then get_recno"
	set db [eval {berkdb_open -rdonly} $args $omethod $testfile ]
	error_check_good dbopen [is_valid_db $db] TRUE

	# Open a cursor
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good db_cursor [is_valid_cursor $dbc $db] TRUE

	set did [open $t1]
	set recno 1

	# Create key(recno) array to use for later comparison
	while { [gets $did str] != -1 } {
		set kvals($recno) $str
		incr recno
	}

	set recno 1
	set ret [$dbc get -first]
	error_check_bad dbc_get_first [llength $ret] 0

	# First walk forward through the database ....
	while { $recno < $count } {
		# Test set_recno: verify it sets the record number properly.
		set current [$dbc get -current]
		set r [$dbc get -set_recno $recno]
		error_check_good set_recno $current $r
		# Test set_recno: verify that we find the expected key
		# at the current record number position.
		set k [lindex [lindex $r 0] 0]
		error_check_good set_recno $kvals($recno) $k

		# Test get_recno: verify that the return from
		# get_recno matches the record number just set.
		set g [$dbc get -get_recno]
		error_check_good get_recno $recno $g
		set ret [$dbc get -next]
		incr recno
	}

	# ... and then backward.
	set recno [expr $count - 1]
	while { $recno > 0 } {
		# Test set_recno: verify that we find the expected key
		# at the current record number position.
		set r [$dbc get -set_recno $recno]
		set k [lindex [lindex $r 0] 0]
		error_check_good set_recno $kvals($recno) $k

		# Test get_recno: verify that the return from
		# get_recno matches the record number just set.
		set g [$dbc get -get_recno]
		error_check_good get_recno $recno $g
		set recno [expr $recno - 1]
	}

	error_check_good cursor_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0
	close $did

	puts "\tTest099.d: Test record renumbering with cursor deletes."
	# Reopen the database, this time with write permission.
	set db [eval {berkdb_open} $args $omethod $testfile ]
	error_check_good dbopen [is_valid_db $db] TRUE

	# Open three cursors.
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc0 [eval {$db cursor} $txn]
	error_check_good db_cursor [is_valid_cursor $dbc0 $db] TRUE
	set dbc1 [eval {$db cursor} $txn]
	error_check_good db_cursor [is_valid_cursor $dbc1 $db] TRUE
	set dbc2 [eval {$db cursor} $txn]
	error_check_good db_cursor [is_valid_cursor $dbc2 $db] TRUE

	# Initialize cursor positions.  Set dbc0 at the beginning,
	# dbc1 at the middle, and dbc2 at the end.
	set ret [$dbc0 get -first]
	error_check_bad dbc0_get_first [llength $ret] 0

	set middle [expr $nentries / 2 + 1]
	set ret [$dbc1 get -set_recno $middle]
	error_check_bad dbc1_get_middle [llength $ret] 0

	set ret [$dbc2 get -last]
	error_check_bad dbc2_get_last [llength $ret] 0

	# At each iteration, delete the first entry, delete the middle
	# entry, and check the record number for beginning, middle and end.
	set count 1
	while { $count <= [expr $nentries / 2] } {
		# Delete first item.
		error_check_good dbc0_del [$dbc0 del] 0

		# For non-txn env's, check that db_stat is recalculating
		# to adjust for items marked for deletion.  We can't do this
		# in txn env's because the live txn will cause deadlock.
		if { $txnenv == 0 } {
			set nkeys [expr $nentries - [expr $count * 2] + 1]
			set stat [$db stat]
			error_check_good keys_after_delete [is_substr $stat \
			     "{Number of keys} $nkeys"] 1
			error_check_good records_after_delete [is_substr $stat \
			     "{Number of records} $nkeys"] 1

			# Now delete the same entry again (which should not
			# change the database) and make sure db->stat returns
			# the same number of keys and records as before.
			catch {[$dbc0 del]} result

			set stat [$db stat]
			error_check_good keys_after_baddelete [is_substr $stat \
			     "{Number of keys} $nkeys"] 1
			error_check_good recs_after_baddelete [is_substr $stat \
			     "{Number of records} $nkeys"] 1
		}

		# Reposition cursor to new first item, check that record number
		# is 1.
		set ret0 [$dbc0 get -next]
		error_check_good beginning_recno [$dbc0 get -get_recno] 1

		# Calculate the current middle recno and compare to actual.
		set middle [$dbc1 get -get_recno]
		set calcmiddle [expr [expr $nentries / 2] - $count + 1]
		error_check_good middle_recno $middle $calcmiddle

		# Delete middle item, reposition cursor to next item.
		error_check_good dbc1_del [$dbc1 del] 0
		set ret1 [$dbc1 get -next]

		# Calculate the expected end recno and compare to actual.
		set end [$dbc2 get -get_recno]
		set calcend [expr $nentries - [expr $count * 2]]
		# On the last iteration, all items have been deleted so
		# there is no recno.
		if { $calcend == 0 } {
			error_check_good end_recno $end ""
		} else {
			error_check_good end_recno $end $calcend
		}
		incr count
	}

	# Close all three cursors.
	error_check_good cursor_close [$dbc0 close] 0
	error_check_good cursor_close [$dbc1 close] 0
	error_check_good cursor_close [$dbc2 close] 0

	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0
}

# Check function for dumped file; data should be fixed are identical
proc test099.check { key data } {
	error_check_good "data mismatch for key $key" $key $data
}
