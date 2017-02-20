# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test013
# TEST	Partial put test
# TEST		Overwrite entire records using partial puts.
# TEST		Make sure that NOOVERWRITE flag works.
# TEST
# TEST	1. Insert 10000 keys and retrieve them (equal key/data pairs).
# TEST	2. Attempt to overwrite keys with NO_OVERWRITE set (expect error).
# TEST	3. Actually overwrite each one with its datum reversed.
# TEST
# TEST	No partial testing here.
proc test013 { method {nentries 10000} args } {
	global errorCode
	global errorInfo
	global fixed_len

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
		set testfile $testdir/test013.db
		set env NULL
	} else {
		set testfile test013.db
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
	puts "Test013: $method ($args) $nentries equal key/data pairs, put test"

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
		set checkfunc test013_recno.check
		append gflags " -recno"
		global kvals
	} else {
		set checkfunc test013.check
	}
	puts "\tTest013.a: put/get loop"
	# Here is the loop where we put and get each key/data pair
	while { [gets $did str] != -1 && $count < $nentries } {
		if { [is_record_based $method] == 1 } {
			set key [expr $count + 1]
			set kvals($key) [pad_data $method $str]
		} else {
			set key $str
		}
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval {$db put} \
		    $txn $pflags {$key [chop_data $method $str]}]
		error_check_good put $ret 0

		set ret [eval {$db get} $gflags $txn {$key}]
		error_check_good \
		    get $ret [list [list $key [pad_data $method $str]]]
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
		incr count
	}
	close $did

	# Now we will try to overwrite each datum, but set the
	# NOOVERWRITE flag.
	puts "\tTest013.b: overwrite values with NOOVERWRITE flag."
	set did [open $dict]
	set count 0
	while { [gets $did str] != -1 && $count < $nentries } {
		if { [is_record_based $method] == 1 } {
			set key [expr $count + 1]
		} else {
			set key $str
		}

		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval {$db put} $txn $pflags \
		    {-nooverwrite $key [chop_data $method $str]}]
		error_check_good put [is_substr $ret "DB_KEYEXIST"] 1

		# Value should be unchanged.
		set ret [eval {$db get} $txn $gflags {$key}]
		error_check_good \
		    get $ret [list [list $key [pad_data $method $str]]]
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
		incr count
	}
	close $did

	# Now we will replace each item with its datum capitalized.
	puts "\tTest013.c: overwrite values with capitalized datum"
	set did [open $dict]
	set count 0
	while { [gets $did str] != -1 && $count < $nentries } {
		if { [is_record_based $method] == 1 } {
			set key [expr $count + 1]
		} else {
			set key $str
		}
		set rstr [string toupper $str]
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set r [eval {$db put} \
		    $txn $pflags {$key [chop_data $method $rstr]}]
		error_check_good put $r 0

		# Value should be changed.
		set ret [eval {$db get} $txn $gflags {$key}]
		error_check_good \
		    get $ret [list [list $key [pad_data $method $rstr]]]
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
		incr count
	}
	close $did

	# Now make sure that everything looks OK
	puts "\tTest013.d: check entire file contents"
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
		for {set i 1} {$i <= $nentries} {incr i} {
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

	error_check_good \
	    Test013:diff($t3,$t2) [filecmp $t3 $t2] 0

	puts "\tTest013.e: close, open, and dump file"
	# Now, reopen the file and run the last test again.
	eval open_and_dump_file $testfile $env $t1 $checkfunc \
	    dump_file_direction "-first" "-next" $args

	if { [is_record_based $method] == 0 } {
		filesort $t1 $t3
	}

	error_check_good \
	    Test013:diff($t3,$t2) [filecmp $t3 $t2] 0

	# Now, reopen the file and run the last test again in the
	# reverse direction.
	puts "\tTest013.f: close, open, and dump file in reverse direction"
	eval open_and_dump_file $testfile $env $t1 $checkfunc \
	    dump_file_direction "-last" "-prev" $args

	if { [is_record_based $method] == 0 } {
		filesort $t1 $t3
	}

	error_check_good \
	    Test013:diff($t3,$t2) [filecmp $t3 $t2] 0
}

# Check function for test013; keys and data are identical
proc test013.check { key data } {
	error_check_good \
	    "key/data mismatch for $key" $data [string toupper $key]
}

proc test013_recno.check { key data } {
	global dict
	global kvals

	error_check_good key"$key"_exists [info exists kvals($key)] 1
	error_check_good \
	    "data mismatch for $key" $data [string toupper $kvals($key)]
}
