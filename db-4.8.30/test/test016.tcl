# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test016
# TEST	Partial put test
# TEST	Partial put where the datum gets shorter as a result of the put.
# TEST
# TEST	Partial put test where partial puts make the record smaller.
# TEST	Use the first 10,000 entries from the dictionary.
# TEST	Insert each with self as key and a fixed, medium length data string;
# TEST	retrieve each. After all are entered, go back and do partial puts,
# TEST	replacing a random-length string with the key value.
# TEST	Then verify.

proc test016 { method {nentries 10000} args } {
	global datastr
	global dvals
	global rand_init
	source ./include.tcl

	berkdb srand $rand_init

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	if { [is_fixed_length $method] == 1 } {
		puts "Test016: skipping for method $method"
		return
	}

	# Create the database and open the dictionary
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test016.db
		set env NULL
	} else {
		set testfile test016.db
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
	puts "Test016: $method ($args) $nentries partial put shorten"

	set t1 $testdir/t1
	set t2 $testdir/t2
	set t3 $testdir/t3
	cleanup $testdir $env
	set db [eval {berkdb_open \
	     -create -mode 0644} $args {$omethod $testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE

	set pflags ""
	set gflags ""
	set txn ""
	set count 0

	if { [is_record_based $method] == 1 } {
		append gflags " -recno"
	}

	# Here is the loop where we put and get each key/data pair
	puts "\tTest016.a: put/get loop"
	set did [open $dict]
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
		set ret [eval {$db put} \
		    $txn $pflags {$key [chop_data $method $datastr]}]
		error_check_good put $ret 0

		set ret [eval {$db get} $txn $gflags {$key}]
		error_check_good \
		    get $ret [list [list $key [pad_data $method $datastr]]]
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
		incr count
	}
	close $did

	# Next we will do a partial put replacement, making the data
	# shorter
	puts "\tTest016.b: partial put loop"
	set did [open $dict]
	set count 0
	set len [string length $datastr]
	while { [gets $did str] != -1 && $count < $nentries } {
		if { [is_record_based $method] == 1 } {
			set key [expr $count + 1]
		} else {
			set key $str
		}

		set repl_len [berkdb random_int [string length $key] $len]
		set repl_off [berkdb random_int 0 [expr $len - $repl_len] ]
		set s1 [string range $datastr 0 [ expr $repl_off - 1] ]
		set s2 [string toupper $key]
		set s3 [string range $datastr [expr $repl_off + $repl_len] end ]
		set dvals($key) [pad_data $method $s1$s2$s3]
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval {$db put} $txn {-partial \
		    [list $repl_off $repl_len] $key [chop_data $method $s2]}]
		error_check_good put $ret 0
		set ret [eval {$db get} $txn $gflags {$key}]
		error_check_good \
		    put $ret [list [list $key [pad_data $method $s1$s2$s3]]]
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
		incr count
	}
	close $did

	# Now we will get each key from the DB and compare the results
	# to the original.
	puts "\tTest016.c: dump file"
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	dump_file $db $txn $t1 test016.check
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
		file rename -force $t1 $t3
	} else {
		set q q
		filehead $nentries $dict $t3
		filesort $t3 $t2
		filesort $t1 $t3
	}

	error_check_good Test016:diff($t3,$t2) \
	    [filecmp $t3 $t2] 0

	# Now, reopen the file and run the last test again.
	puts "\tTest016.d: close, open, and dump file"
	eval open_and_dump_file $testfile $env $t1 test016.check \
	    dump_file_direction "-first" "-next" $args

	if { [ is_record_based $method ] == 0 } {
		filesort $t1 $t3
	}
	error_check_good Test016:diff($t3,$t2) \
	    [filecmp $t3 $t2] 0

	# Now, reopen the file and run the last test again in reverse direction.
	puts "\tTest016.e: close, open, and dump file in reverse direction"
	eval open_and_dump_file $testfile $env $t1 test016.check \
	    dump_file_direction "-last" "-prev" $args

	if { [ is_record_based $method ] == 0 } {
		filesort $t1 $t3
	}
	error_check_good Test016:diff($t3,$t2) \
	    [filecmp $t3 $t2] 0
}

# Check function for test016; data should be whatever is set in dvals
proc test016.check { key data } {
	global datastr
	global dvals

	error_check_good key"$key"_exists [info exists dvals($key)] 1
	error_check_good "data mismatch for key $key" $data $dvals($key)
}
