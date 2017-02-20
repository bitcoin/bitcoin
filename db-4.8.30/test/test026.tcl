# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test026
# TEST	Small keys/medium data w/duplicates
# TEST		Put/get per key.
# TEST		Loop through keys -- delete each key
# TEST		    ... test that cursors delete duplicates correctly
# TEST
# TEST	Keyed delete test through cursor.  If ndups is small; this will
# TEST	test on-page dups; if it's large, it will test off-page dups.
proc test026 { method {nentries 2000} {ndups 5} {tnum "026"} args} {
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	# Btree with compression does not support unsorted duplicates.
	if { [is_compressed $args] == 1 } {
		puts "Test$tnum skipping for btree with compression."
		return
	}
	if { [is_record_based $method] == 1 || \
	    [is_rbtree $method] == 1 } {
		puts "Test$tnum skipping for method $method"
		return
	}
	# Create the database and open the dictionary
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
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
			#
			# If we are using txns and running with the
			# default, set the defaults down a bit.
			# If we are wanting a lot of dups, set that
			# down a bit or repl testing takes very long.
			#
			if { $nentries == 2000 } {
				set nentries 100
			}
			reduce_dups nentries ndups
		}
		set testdir [get_home $env]
	}
	cleanup $testdir $env
	puts "Test$tnum: $method ($args) $nentries keys\
		with $ndups dups; cursor delete test"

	set pflags ""
	set gflags ""
	set txn ""
	set count 0

	# Here is the loop where we put and get each key/data pair

	puts "\tTest$tnum.a: Put loop"
	set db [eval {berkdb_open -create \
		-mode 0644} $args {$omethod -dup $testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE
	set did [open $dict]
	while { [gets $did str] != -1 && $count < [expr $nentries * $ndups] } {
		set datastr [ make_data_str $str ]
		for { set j 1 } { $j <= $ndups} {incr j} {
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
	 		set ret [eval {$db put} \
	     		    $txn $pflags {$str [chop_data $method $j$datastr]}]
			error_check_good db_put $ret 0
			if { $txnenv == 1 } {
				error_check_good txn [$t commit] 0
			}
			incr count
		}
	}
	close $did

	error_check_good db_close [$db close] 0
	set db [eval {berkdb_open} $args $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	# Now we will sequentially traverse the database getting each
	# item and deleting it.
	set count 0
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good db_cursor [is_substr $dbc $db] 1

	puts "\tTest$tnum.b: Get/delete loop"
	set i 1
	for { set ret [$dbc get -first] } {
	    [string length $ret] != 0 } {
	    set ret [$dbc get -next] } {

		set key [lindex [lindex $ret 0] 0]
		set data [lindex [lindex $ret 0] 1]
		if { $i == 1 } {
			set curkey $key
		}
		error_check_good seq_get:key $key $curkey
		error_check_good \
		    seq_get:data $data [pad_data $method $i[make_data_str $key]]

		if { $i == $ndups } {
			set i 1
		} else {
			incr i
		}

		# Now delete the key
		set ret [$dbc del]
		error_check_good db_del:$key $ret 0
	}
	error_check_good dbc_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0

	puts "\tTest$tnum.c: Verify empty file"
	# Double check that file is now empty
	set db [eval {berkdb_open} $args $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good db_cursor [is_substr $dbc $db] 1
	set ret [$dbc get -first]
	error_check_good get_on_empty [string length $ret] 0
	error_check_good dbc_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0
}
