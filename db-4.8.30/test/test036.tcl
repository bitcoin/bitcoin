# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test036
# TEST	Test KEYFIRST and KEYLAST when the key doesn't exist
# TEST	Put nentries key/data pairs (from the dictionary) using a cursor
# TEST	and KEYFIRST and KEYLAST (this tests the case where use use cursor
# TEST	put for non-existent keys).
proc test036 { method {nentries 10000} args } {
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]
	if { [is_record_based $method] == 1 } {
		puts "Test036 skipping for method recno"
		return
	}

	# Create the database and open the dictionary
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test036.db
		set env NULL
	} else {
		set testfile test036.db
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

	puts "Test036: $method ($args) $nentries equal key/data pairs"
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
		set checkfunc test036_recno.check
		append gflags " -recno"
	} else {
		set checkfunc test036.check
	}
	puts "\tTest036.a: put/get loop KEYFIRST"
	# Here is the loop where we put and get each key/data pair
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good cursor [is_valid_cursor $dbc $db] TRUE
	while { [gets $did str] != -1 && $count < $nentries } {
		if { [is_record_based $method] == 1 } {
			global kvals

			set key [expr $count + 1]
			set kvals($key) $str
		} else {
			set key $str
		}
		set ret [eval {$dbc put} $pflags {-keyfirst $key $str}]
		error_check_good put $ret 0

		set ret [eval {$db get} $txn $gflags {$key}]
		error_check_good get [lindex [lindex $ret 0] 1] $str
		incr count
	}
	error_check_good dbc_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}

	puts "\tTest036.a: put/get loop KEYLAST"
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good cursor [is_valid_cursor $dbc $db] TRUE
	while { [gets $did str] != -1 && $count < $nentries } {
		if { [is_record_based $method] == 1 } {
			global kvals

			set key [expr $count + 1]
			set kvals($key) $str
		} else {
			set key $str
		}
		set ret [eval {$dbc put} $txn $pflags {-keylast $key $str}]
		error_check_good put $ret 0

		set ret [eval {$db get} $txn $gflags {$key}]
		error_check_good get [lindex [lindex $ret 0] 1] $str
		incr count
	}
	error_check_good dbc_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	close $did

	# Now we will get each key from the DB and compare the results
	# to the original.
	puts "\tTest036.c: dump file"
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	dump_file $db $txn $t1 $checkfunc
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0

	# Now compare the keys to see if they match the dictionary (or ints)
	if { [is_record_based $method] == 1 } {
		set oid [open $t2 w]
		for {set i 1} {$i <= $nentries} {set i [incr i]} {
			puts $oid $i
		}
		close $oid
		file rename -force $t1 $t3
	} else {
		set q q
		filehead $nentries $dict $t3
		filesort $t3 $t2
		filesort $t1 $t3
	}

}

# Check function for test036; keys and data are identical
proc test036.check { key data } {
	error_check_good "key/data mismatch" $data $key
}

proc test036_recno.check { key data } {
	global dict
	global kvals

	error_check_good key"$key"_exists [info exists kvals($key)] 1
	error_check_good "key/data mismatch, key $key" $data $kvals($key)
}
