# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test109
# TEST
# TEST	Test of sequences.
proc test109 { method {tnum "109"} args } {
	source ./include.tcl
	global rand_init
	global fixed_len
	global errorCode

	set eindex [lsearch -exact $args "-env"]
	set txnenv 0
	set rpcenv 0
	set sargs " -thread "

	if { [is_partitioned $args] == 1 } {
		puts "Test109 skipping for partitioned $method"
		return
	}
	if { $eindex == -1 } {
		set env NULL
	} else {
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		set rpcenv [is_rpcenv $env]
		if { $rpcenv == 1 } {
			puts "Test$tnum: skipping for RPC"
			return
		}
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set testdir [get_home $env]
	}

	# Fixed_len must be increased from the default to
	# accommodate fixed-record length methods.
	set orig_fixed_len $fixed_len
	set fixed_len 128
	set args [convert_args $method $args]
	set omethod [convert_method $method]
	error_check_good random_seed [berkdb srand $rand_init] 0

	# Test with in-memory dbs, regular dbs, and subdbs.
	foreach filetype { subdb regular in-memory } {
		puts "Test$tnum: $method ($args) Test of sequences ($filetype)."

		# Skip impossible combinations.
		if { $filetype == "subdb" && [is_queue $method] } {
			puts "Skipping $filetype test for method $method."
			continue
		}
		if { $filetype == "in-memory" && [is_queueext $method] } {
			puts "Skipping $filetype test for method $method."
			continue
		}

		# Reinitialize file name for each file type, then adjust.
		if { $eindex == -1 } {
			set testfile $testdir/test$tnum.db
		} else {
			set testfile test$tnum.db
			set testdir [get_home $env]
		}
		if { $filetype == "subdb" } {
			lappend testfile SUBDB
		}
		if { $filetype == "in-memory" } {
			set testfile ""
		}

		cleanup $testdir $env

		# Make the key numeric so we can test record-based methods.
		set key 1

		# Open a noerr db, since we expect errors.
		set db [eval {berkdb_open_noerr \
		    -create -mode 0644} $args $omethod $testfile]
		error_check_good dbopen [is_valid_db $db] TRUE

		puts "\tTest$tnum.a: Max must be greater than min."
		set errorCode NONE
		catch {set seq [eval {berkdb sequence} -create $sargs \
		    -init 0 -min 100 -max 0 $db $key]} res
		error_check_good max>min [is_substr $errorCode EINVAL] 1

		puts "\tTest$tnum.b: Init can't be out of the min-max range."
		set errorCode NONE
		catch {set seq [eval {berkdb sequence} -create $sargs \
			-init 101 -min 0 -max 100 $db $key]} res
		error_check_good init [is_substr $errorCode EINVAL] 1

		# Test increment and decrement.
		set min 0
		set max 100
		foreach { init inc } { $min -inc $max -dec } {
			puts "\tTest$tnum.c: Test for overflow error with $inc."
			test_sequence $env $db $key $min $max $init $inc
		}

		# Test cachesize without wrap.  Make sure to test both
		# cachesizes that evenly divide the number of items in the
		# sequence, and that leave unused elements at the end.
		set min 0
		set max 99
		set init 1
		set cachesizes [list 2 7 11]
		foreach csize $cachesizes {
			foreach inc { -inc -dec } {
				puts "\tTest$tnum.d:\
				    -cachesize $csize, $inc, no wrap."
				test_sequence $env $db $key \
				    $min $max $init $inc $csize
			}
		}
		error_check_good db_close [$db close] 0

		# Open a regular db; we expect success on the rest of the tests.
		set db [eval {berkdb_open \
		     -create -mode 0644} $args $omethod $testfile]
		error_check_good dbopen [is_valid_db $db] TRUE

		# Test increment and decrement with wrap.  Cross from negative
		# to positive integers.
		set min -50
		set max 99
		set wrap "-wrap"
		set csize 1
		foreach { init inc } { $min -inc $max -dec } {
			puts "\tTest$tnum.e: Test wrapping with $inc."
			test_sequence $env $db $key \
			    $min $max $init $inc $csize $wrap
		}

		# Test cachesize with wrap.
		set min 0
		set max 99
		set init 0
		set wrap "-wrap"
		foreach csize $cachesizes {
			puts "\tTest$tnum.f: Test -cachesize $csize with wrap."
			test_sequence $env $db $key \
			    $min $max $init $inc $csize $wrap
		}

		# Test multiple handles on the same sequence.
		foreach csize $cachesizes {
			puts "\tTest$tnum.g:\
			    Test multiple handles (-cachesize $csize) with wrap."
			test_sequence $env $db $key \
			    $min $max $init $inc $csize $wrap 1
		}
		error_check_good db_close [$db close] 0
	}
	set fixed_len $orig_fixed_len
	return
}

proc test_sequence { env db key min max init \
    {inc "-inc"} {csize 1} {wrap "" } {second_handle 0} } {
	global rand_init
	global errorCode

	set txn ""
	set txnenv 0
	if { $env != "NULL" } {
		set txnenv [is_txnenv $env]
	}

	set sargs " -thread "

	# The variable "skip" is the cachesize with a direction.
	set skip $csize
	if { $inc == "-dec" } {
		set skip [expr $csize * -1]
	}

	# The "limit" is the closest number to the end of the
	# sequence we can ever see.
	set limit [expr [expr $max + 1] - $csize]
	if { $inc == "-dec" } {
		set limit [expr [expr $min - 1] + $csize]
	}

	# The number of items in the sequence.
	set n [expr [expr $max - $min] + 1]

	# Calculate the number of values returned in the first
	# cycle, and in all other cycles.
	if { $inc == "-inc" } {
		set firstcyclehits \
		    [expr [expr [expr $max - $init] + 1] / $csize]
	} elseif { $inc == "-dec" } {
		set firstcyclehits \
		    [expr [expr [expr $init - $min] + 1] / $csize]
	} else {
		puts "FAIL: unknown inc flag $inc"
	}
	set hitspercycle [expr $n / $csize]

	# Create the sequence.
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set seq [eval {berkdb sequence} -create $sargs -cachesize $csize \
	    $wrap -init $init -min $min -max $max $txn $inc $db $key]
	error_check_good is_valid_seq [is_valid_seq $seq] TRUE
	if { $second_handle == 1 } {
		set seq2 [eval {berkdb sequence} -create $sargs $txn $db $key]
		error_check_good is_valid_seq2 [is_valid_seq $seq2] TRUE
	}
	if { $txnenv == 1 } {
		error_check_good txn_commit [$t commit] 0
	}

	# Exercise get options.
	set getdb [$seq get_db]
	error_check_good seq_get_db $getdb $db

	set flags [$seq get_flags]
	set exp_flags [list $inc $wrap]
	foreach item $exp_flags {
		if { [llength $item] == 0 } {
			set idx [lsearch -exact $exp_flags $item]
			set exp_flags [lreplace $exp_flags $idx $idx]
		}
	}
	error_check_good get_flags $flags $exp_flags

	set range [$seq get_range]
	error_check_good get_range_min [lindex $range 0] $min
	error_check_good get_range_max [lindex $range 1] $max

	set cache [$seq get_cachesize]
	error_check_good get_cachesize $cache $csize

	# Within the loop, for each successive seq get we calculate
	# the value we expect to receive, then do the seq get and
	# compare.
	#
	# Always test some multiple of the number of items in the
	# sequence; this tests overflow and wrap-around.
	#
	set mult 2
	for { set i 0 } { $i < [expr $n * $mult] } { incr i } {
		#
		# Calculate expected return value.
		#
		# On the first cycle, start from init.
		set expected [expr $init + [expr $i * $skip]]
		if { $i >= $firstcyclehits && $wrap != "-wrap" } {
			set expected "overflow"
		}

		# On second and later cycles, start from min or max.
		# We do a second cycle only if wrapping is specified.
		if { $wrap == "-wrap" } {
			if { $inc == "-inc" && $expected > $limit } {
				set j [expr $i - $firstcyclehits]
				while { $j >= $hitspercycle } {
					set j [expr $j - $hitspercycle]
				}
				set expected [expr $min + [expr $j * $skip]]
			}

			if { $inc == "-dec" && $expected < $limit } {
				set j [expr $i - $firstcyclehits]
				while { $j >= $hitspercycle } {
					set j [expr $j - $hitspercycle]
				}
				set expected [expr $max + [expr $j * $skip]]
			}
		}

		# Get return value.  If we've got a second handle, choose
		# randomly which handle does the seq get.
		if { $env != "NULL" && [is_txnenv $env] } {
			set syncarg " -nosync "
		} else {
			set syncarg ""
		}
		set errorCode NONE
		if { $second_handle == 0 } {
			catch {eval {$seq get} $syncarg $csize} res
		} elseif { [berkdb random_int 0 1] == 0 } {
			catch {eval {$seq get} $syncarg $csize} res
		} else {
			catch {eval {$seq2 get} $syncarg $csize} res
		}

		# Compare expected to actual value.
		if { $expected == "overflow" } {
			error_check_good overflow [is_substr $errorCode EINVAL] 1
		} else {
			error_check_good seq_get_wrap $res $expected
		}
	}

	# A single handle requires a 'seq remove', but a second handle
	# should be closed, and then we can remove the sequence.
	if { $second_handle == 1 } {
		error_check_good seq2_close [$seq2 close] 0
	}
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	error_check_good seq_remove [eval {$seq remove} $txn] 0
	if { $txnenv == 1 } {
		error_check_good txn_commit [$t commit] 0
	}
}
