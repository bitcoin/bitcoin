# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test110
# TEST	Partial get test with duplicates.
# TEST
# TEST	For hash and btree, create and populate a database
# TEST	with dups. Randomly selecting offset and length,
# TEST	retrieve data from each record and make sure we
# TEST	get what we expect.
proc test110 { method {nentries 10000} {ndups 3} args } {
	global rand_init
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	if { [is_record_based $method] == 1 || \
		[is_rbtree $method] == 1 } {
		puts "Test110 skipping for method $method"
		return
	}

	# Btree with compression does not support unsorted duplicates.
	if { [is_compressed $args] == 1 } {
		puts "Test110 skipping for btree with compression."
		return
	}
	# Create the database and open the dictionary
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test110.db
		set env NULL
	} else {
		set testfile test110.db
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
	puts "Test110: $method ($args) $nentries partial get test with duplicates"

	cleanup $testdir $env

	set db [eval {berkdb_open \
	     -create -mode 0644} -dup $args {$omethod $testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE
	set did [open $dict]
	berkdb srand $rand_init

	set txn ""
	set count 0

	puts "\tTest110.a: put/get loop"
	for { set i 0 } { [gets $did str] != -1 && $i < $nentries } \
	    { incr i } {

		set key $str
		set repl [berkdb random_int 1 100]
		set kvals($key) $repl
		set data [chop_data $method [replicate $str $repl]]
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		for { set j 0 } { $j < $ndups } { incr j } {
			set ret [eval {$db put} $txn {$key $j.$data}]
			error_check_good dbput:$key:$j $ret 0
		}

		set dbc [eval {$db cursor} $txn]
		error_check_good db_cursor [is_valid_cursor $dbc $db] TRUE

		set ret [$dbc get -set $key]

		set j 0
		for { set dbt [$dbc get -current] } \
		    { $j < $ndups } \
		    { set dbt [$dbc get -next] } {
			set d [lindex [lindex $dbt 0] 1]
			error_check_good dupget:$key:$j $d [pad_data $method $j.$data]
			incr j
		}
		error_check_good cursor_close [$dbc close] 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
	}
	close $did

	puts "\tTest110.b: partial get loop"
	set did [open $dict]
	for { set i 0 } { [gets $did str] != -1 && $i < $nentries } \
	    { incr i } {
		set key $str

		set data [pad_data $method [replicate $str $kvals($key)]]
		set j 0

		# Set up cursor.  We will use the cursor to walk the dups.
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}

		set dbc [eval {$db cursor} $txn]
		error_check_good db_cursor [is_valid_cursor $dbc $db] TRUE

 		# Position cursor at the first of the dups.
		set ret [$dbc get -set $key]

		for { set dbt [$dbc get -current] } \
		    { $j < $ndups } \
		    { set dbt [$dbc get -next] } {

			set dupdata $j.$data
			set length [expr [string length $dupdata]]
			set maxndx [expr $length + 1]

			if { $maxndx > 0 } {
				set beg [berkdb random_int 0 [expr $maxndx - 1]]
				set len [berkdb random_int 0 [expr $maxndx * 2]]
			} else {
				set beg 0
				set len 0
			}

			set ret [eval {$dbc get} -current \
			    {-partial [list $beg $len]}]

			# In order for tcl to handle this, we have to overwrite the
			# last character with a NULL.  That makes the length one less
			# than we expect.
			set k [lindex [lindex $ret 0] 0]
			set d [lindex [lindex $ret 0] 1]
			error_check_good dbget_key $k $key
			error_check_good dbget_data $d \
			    [string range $dupdata $beg [expr $beg + $len - 1]]
			incr j
		}

		error_check_good cursor_close [$dbc close] 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
	}
	error_check_good db_close [$db close] 0
	close $did
}
