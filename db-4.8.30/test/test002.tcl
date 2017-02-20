# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test002
# TEST	Small keys/medium data
# TEST		Put/get per key
# TEST		Dump file
# TEST		Close, reopen
# TEST		Dump file
# TEST
# TEST	Use the first 10,000 entries from the dictionary.
# TEST	Insert each with self as key and a fixed, medium length data string;
# TEST	retrieve each. After all are entered, retrieve all; compare output
# TEST	to original. Close file, reopen, do retrieve and re-verify.

proc test002 { method {nentries 10000} args } {
	global datastr
	global pad_datastr
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test002.db
		set env NULL
	} else {
		set testfile test002.db
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
	# Create the database and open the dictionary
	puts "Test002: $method ($args) $nentries key <fixed data> pairs"

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

	# Here is the loop where we put and get each key/data pair

	if { [is_record_based $method] == 1 } {
		append gflags "-recno"
	}
	set pad_datastr [pad_data $method $datastr]
	puts "\tTest002.a: put/get loop"
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
		set ret [eval {$db put} $txn $pflags {$key [chop_data $method $datastr]}]
		error_check_good put $ret 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}

		set ret [eval {$db get} $gflags {$key}]

		error_check_good get $ret [list [list $key [pad_data $method $datastr]]]
		incr count
	}
	close $did

	# Now we will get each key from the DB and compare the results
	# to the original.
	puts "\tTest002.b: dump file"
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	dump_file $db $txn $t1 test002.check
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0

	# Now compare the keys to see if they match the dictionary
	if { [is_record_based $method] == 1 } {
		set oid [open $t2 w]
		for {set i 1} {$i <= $nentries} {set i [incr i]} {
			puts $oid $i
		}
		close $oid
		filesort $t2 $t3
		file rename -force $t3 $t2
	} else {
		set q q
		filehead $nentries $dict $t3
		filesort $t3 $t2
	}
	filesort $t1 $t3

	error_check_good Test002:diff($t3,$t2) \
	    [filecmp $t3 $t2] 0

	# Now, reopen the file and run the last test again.
	puts "\tTest002.c: close, open, and dump file"
	eval open_and_dump_file $testfile $env $t1 test002.check \
	    dump_file_direction "-first" "-next" $args

	if { [string compare $omethod "-recno"] != 0 } {
		filesort $t1 $t3
	}
	error_check_good Test002:diff($t3,$t2) \
	    [filecmp $t3 $t2] 0

	# Now, reopen the file and run the last test again in reverse direction.
	puts "\tTest002.d: close, open, and dump file in reverse direction"
	eval open_and_dump_file $testfile $env $t1 test002.check \
	    dump_file_direction "-last" "-prev" $args

	if { [string compare $omethod "-recno"] != 0 } {
		filesort $t1 $t3
	}
	error_check_good Test002:diff($t3,$t2) \
	    [filecmp $t3 $t2] 0
}

# Check function for test002; data should be fixed are identical
proc test002.check { key data } {
	global pad_datastr
	error_check_good "data mismatch for key $key" $data $pad_datastr
}
