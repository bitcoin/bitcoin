# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test021
# TEST	Btree range tests.
# TEST
# TEST	Use the first 10,000 entries from the dictionary.
# TEST	Insert each with self, reversed as key and self as data.
# TEST	After all are entered, retrieve each using a cursor SET_RANGE, and
# TEST	getting about 20 keys sequentially after it (in some cases we'll
# TEST	run out towards the end of the file).
proc test021 { method {nentries 10000} args } {
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	# Create the database and open the dictionary
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test021.db
		set env NULL
	} else {
		set testfile test021.db
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
	puts "Test021: $method ($args) $nentries equal key/data pairs"

	set t1 $testdir/t1
	set t2 $testdir/t2
	set t3 $testdir/t3
	cleanup $testdir $env
	set db [eval {berkdb_open \
	     -create -mode 0644} $args {$omethod $testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE

	set did [open $dict]

	set pflags ""
	set gflags ""
	set txn ""
	set count 0

	if { [is_record_based $method] == 1 } {
		set checkfunc test021_recno.check
		append gflags " -recno"
	} else {
		set checkfunc test021.check
	}
	puts "\tTest021.a: put loop"
	# Here is the loop where we put each key/data pair
	while { [gets $did str] != -1 && $count < $nentries } {
		if { [is_record_based $method] == 1 } {
			global kvals

			set key [expr $count + 1]
			set kvals($key) [pad_data $method $str]
		} else {
			set key [reverse $str]
		}

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

	# Now we will get each key from the DB and retrieve about 20
	# records after it.
	error_check_good db_close [$db close] 0

	puts "\tTest021.b: test ranges"
	set db [eval {berkdb_open -rdonly} $args $omethod $testfile ]
	error_check_good dbopen [is_valid_db $db] TRUE

	# Open a cursor
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good db_cursor [is_substr $dbc $db] 1

	set did [open $dict]
	set i 0
	while { [gets $did str] != -1 && $i < $count } {
		if { [is_record_based $method] == 1 } {
			set key [expr $i + 1]
		} else {
			set key [reverse $str]
		}

		set r [$dbc get -set_range $key]
		error_check_bad dbc_get:$key [string length $r] 0
		set k [lindex [lindex $r 0] 0]
		set d [lindex [lindex $r 0] 1]
		$checkfunc $k $d

		for { set nrecs 0 } { $nrecs < 20 } { incr nrecs } {
			set r [$dbc get "-next"]
			# no error checking because we may run off the end
			# of the database
			if { [llength $r] == 0 } {
				continue;
			}
			set k [lindex [lindex $r 0] 0]
			set d [lindex [lindex $r 0] 1]
			$checkfunc $k $d
		}
		incr i
	}
	error_check_good dbc_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0
	close $did
}

# Check function for test021; keys and data are reversed
proc test021.check { key data } {
	error_check_good "key/data mismatch for $key" $data [reverse $key]
}

proc test021_recno.check { key data } {
	global dict
	global kvals

	error_check_good key"$key"_exists [info exists kvals($key)] 1
	error_check_good "data mismatch: key $key" $data $kvals($key)
}
