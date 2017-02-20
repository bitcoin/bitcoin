# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test025
# TEST	DB_APPEND flag test.
proc test025 { method {nentries 10000} {start 0 } {tnum "025"} args} {
	global kvals
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]
	puts "Test$tnum: $method ($args)"

	if { [string compare $omethod "-btree"] == 0 } {
		puts "Test$tnum skipping for method BTREE"
		return
	}
	if { [string compare $omethod "-hash"] == 0 } {
		puts "Test$tnum skipping for method HASH"
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
	set db [eval {berkdb_open \
	     -create -mode 0644} $args {$omethod $testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE
	set did [open $dict]

	puts "\tTest$tnum.a: put/get loop"
	set gflags " -recno"
	set pflags " -append"
	set txn ""
	set checkfunc test025_check

	# Here is the loop where we put and get each key/data pair
	set count $start
	set nentries [expr $start + $nentries]
	if { $count != 0 } {
		gets $did str
		set k [expr $count + 1]
		set kvals($k) [pad_data $method $str]
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval {$db put} $txn {$k [chop_data $method $str]}]
		error_check_good db_put $ret 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
		incr count
	}

	while { [gets $did str] != -1 && $count < $nentries } {
		set k [expr $count + 1]
		set kvals($k) [pad_data $method $str]
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval {$db put} $txn $pflags {[chop_data $method $str]}]
		error_check_good db_put $ret $k

		set ret [eval {$db get} $txn $gflags {$k}]
		error_check_good \
		    get $ret [list [list $k [pad_data $method $str]]]
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}

		# The recno key will be count + 1, so when we hit
		# UINT32_MAX - 1, reset to 0.
		if { $count == [expr 0xfffffffe] } {
			set count 0
		} else {
			incr count
		}
	}
	close $did

	# Now we will get each key from the DB and compare the results
	# to the original.
	puts "\tTest$tnum.b: dump file"
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

	puts "\tTest$tnum.c: close, open, and dump file"
	# Now, reopen the file and run the last test again.
	eval open_and_dump_file $testfile $env $t1 $checkfunc \
	    dump_file_direction -first -next $args

	# Now, reopen the file and run the last test again in the
	# reverse direction.
	puts "\tTest$tnum.d: close, open, and dump file in reverse direction"
	eval open_and_dump_file $testfile $env $t1 $checkfunc \
		dump_file_direction -last -prev $args
}

proc test025_check { key data } {
	global kvals

	error_check_good key"$key"_exists [info exists kvals($key)] 1
	error_check_good " key/data mismatch for |$key|" $data $kvals($key)
}
