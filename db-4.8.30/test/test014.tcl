# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test014
# TEST	Exercise partial puts on short data
# TEST		Run 5 combinations of numbers of characters to replace,
# TEST		and number of times to increase the size by.
# TEST
# TEST	Partial put test, small data, replacing with same size.  The data set
# TEST	consists of the first nentries of the dictionary.  We will insert them
# TEST	(and retrieve them) as we do in test 1 (equal key/data pairs).  Then
# TEST	we'll try to perform partial puts of some characters at the beginning,
# TEST	some at the end, and some at the middle.
proc test014 { method {nentries 10000} args } {
	set fixed 0
	set args [convert_args $method $args]

	if { [is_fixed_length $method] == 1 } {
		set fixed 1
	}

	puts "Test014: $method ($args) $nentries equal key/data pairs, put test"

	# flagp indicates whether this is a postpend or a
	# normal partial put
	set flagp 0

	eval {test014_body $method $flagp 1 1 $nentries} $args
	eval {test014_body $method $flagp 1 4 $nentries} $args
	eval {test014_body $method $flagp 2 4 $nentries} $args
	eval {test014_body $method $flagp 1 128 $nentries} $args
	eval {test014_body $method $flagp 2 16 $nentries} $args
	if { $fixed == 0 } {
		eval {test014_body $method $flagp 0 1 $nentries} $args
		eval {test014_body $method $flagp 0 4 $nentries} $args
		eval {test014_body $method $flagp 0 128 $nentries} $args

		# POST-PENDS :
		# partial put data after the end of the existent record
		# chars: number of empty spaces that will be padded with null
		# increase: is the length of the str to be appended (after pad)
		#
		set flagp 1
		eval {test014_body $method $flagp 1 1 $nentries} $args
		eval {test014_body $method $flagp 4 1 $nentries} $args
		eval {test014_body $method $flagp 128 1 $nentries} $args
		eval {test014_body $method $flagp 1 4 $nentries} $args
		eval {test014_body $method $flagp 1 128 $nentries} $args
	}
	puts "Test014 complete."
}

proc test014_body { method flagp chars increase {nentries 10000} args } {
	source ./include.tcl

	set omethod [convert_method $method]

	if { [is_fixed_length $method] == 1 && $chars != $increase } {
		puts "Test014: $method: skipping replace\
		    $chars chars with string $increase times larger."
		return
	}

	if { $flagp == 1} {
		puts "Test014: Postpending string of len $increase with \
		    gap $chars."
	} else {
		puts "Test014: Replace $chars chars with string \
		    $increase times larger"
	}

	# Create the database and open the dictionary
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test014.db
		set env NULL
	} else {
		set testfile test014.db
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
	set t2 $testdir/t2
	set t3 $testdir/t3
	cleanup $testdir $env

	set db [eval {berkdb_open \
	     -create -mode 0644} $args {$omethod $testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE

	set gflags ""
	set pflags ""
	set txn ""
	set count 0

	if { [is_record_based $method] == 1 } {
		append gflags " -recno"
	}

	puts "\tTest014.a: put/get loop"
	# Here is the loop where we put and get each key/data pair
	# We will do the initial put and then three Partial Puts
	# for the beginning, middle and end of the string.
	set did [open $dict]
	while { [gets $did str] != -1 && $count < $nentries } {
		if { [is_record_based $method] == 1 } {
			set key [expr $count + 1]
		} else {
			set key $str
		}
		if { $flagp == 1 } {
			# this is for postpend only
			global dvals

			# initial put
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			set ret [eval {$db put} $txn {$key $str}]
			if { $txnenv == 1 } {
				error_check_good txn [$t commit] 0
			}
			error_check_good dbput $ret 0

			set offset [string length $str]

			# increase is the actual number of new bytes
			# to be postpended (besides the null padding)
			set data [repeat "P" $increase]

			# chars is the amount of padding in between
			# the old data and the new
			set len [expr $offset + $chars + $increase]
			set dvals($key) [binary format \
			    a[set offset]x[set chars]a[set increase] \
			    $str $data]
			set offset [expr $offset + $chars]
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			set ret [eval {$db put -partial [list $offset 0]} \
			    $txn {$key $data}]
			error_check_good dbput:post $ret 0
			if { $txnenv == 1 } {
				error_check_good txn [$t commit] 0
			}
		} else {
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			partial_put $method $db $txn \
			    $gflags $key $str $chars $increase
			if { $txnenv == 1 } {
				error_check_good txn [$t commit] 0
			}
		}
		incr count
	}
	close $did

	# Now make sure that everything looks OK
	puts "\tTest014.b: check entire file contents"
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	dump_file $db $txn $t1 test014.check
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

	error_check_good \
	    Test014:diff($t3,$t2) [filecmp $t3 $t2] 0

	puts "\tTest014.c: close, open, and dump file"
	# Now, reopen the file and run the last test again.
	eval open_and_dump_file $testfile $env \
	    $t1 test014.check dump_file_direction "-first" "-next" $args

	if { [string compare $omethod "-recno"] != 0 } {
		filesort $t2 $t3
		file rename -force $t3 $t2
		filesort $t1 $t3
	}

	error_check_good \
	    Test014:diff($t3,$t2) [filecmp $t3 $t2] 0
	# Now, reopen the file and run the last test again in the
	# reverse direction.
	puts "\tTest014.d: close, open, and dump file in reverse direction"
	eval open_and_dump_file $testfile $env $t1 \
	    test014.check dump_file_direction "-last" "-prev" $args

	if { [string compare $omethod "-recno"] != 0 } {
		filesort $t2 $t3
		file rename -force $t3 $t2
		filesort $t1 $t3
	}

	error_check_good \
	    Test014:diff($t3,$t2) [filecmp $t3 $t2] 0
}

# Check function for test014; keys and data are identical
proc test014.check { key data } {
	global dvals

	error_check_good key"$key"_exists [info exists dvals($key)] 1
	error_check_good "data mismatch for key $key" $data $dvals($key)
}
