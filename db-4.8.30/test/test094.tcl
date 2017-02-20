# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test094
# TEST	Test using set_dup_compare.
# TEST
# TEST	Use the first 10,000 entries from the dictionary.
# TEST	Insert each with self as key and data; retrieve each.
# TEST	After all are entered, retrieve all; compare output to original.
# TEST	Close file, reopen, do retrieve and re-verify.
proc test094 { method {nentries 10000} {ndups 10} {tnum "094"} args} {
	source ./include.tcl
	global errorInfo

	set dbargs [convert_args $method $args]
	set omethod [convert_method $method]

	if { [is_btree $method] != 1 && [is_hash $method] != 1 } {
		puts "Test$tnum: skipping for method $method."
		return
	}

	# We'll need any encryption args separated from the db args
	# so we can pass them to dbverify.
	set encargs ""
	set dbargs [split_encargs $dbargs encargs]

	set txnenv 0
	set eindex [lsearch -exact $dbargs "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test$tnum-a.db
		set env NULL
		set envargs ""
	} else {
		set testfile test$tnum-a.db
		incr eindex
		set env [lindex $dbargs $eindex]
		set envargs " -env $env "
		set rpcenv [is_rpcenv $env]
		if { $rpcenv == 1 } {
			puts "Test$tnum: skipping for RPC"
			return
		}
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append dbargs " -auto_commit "
			if { $nentries == 10000 } {
				set nentries 100
			}
			reduce_dups nentries ndups
		}
		set testdir [get_home $env]
	}
	puts "Test$tnum: $method ($args) $nentries \
	    with $ndups dups using dupcompare"

	cleanup $testdir $env

	set db [eval {berkdb_open -dupcompare test094_cmp -dup -dupsort\
	     -create -mode 0644} $omethod $encargs $dbargs {$testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE

	set did [open $dict]
	set t1 $testdir/t1
	set pflags ""
	set gflags ""
	set txn ""
	puts "\tTest$tnum.a: $nentries put/get duplicates loop"
	# Here is the loop where we put and get each key/data pair
	set count 0
	set dlist {}
	for {set i 0} {$i < $ndups} {incr i} {
		set dlist [linsert $dlist 0 $i]
	}
	while { [gets $did str] != -1 && $count < $nentries } {
		set key $str
		for {set i 0} {$i < $ndups} {incr i} {
			set data $i:$str
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			set ret [eval {$db put} \
			    $txn $pflags {$key [chop_data $omethod $data]}]
			error_check_good put $ret 0
			if { $txnenv == 1 } {
				error_check_good txn [$t commit] 0
			}
		}

		set ret [eval {$db get} $gflags {$key}]
		error_check_good get [llength $ret] $ndups
		incr count
	}
	close $did
	# Now we will get each key from the DB and compare the results
	# to the original.
	puts "\tTest$tnum.b: traverse checking duplicates before close"
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	dup_check $db $txn $t1 $dlist
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0

	# Now run verify to check the internal structure and order.
	if { [catch {eval {berkdb dbverify} -dupcompare test094_cmp\
	    $envargs $encargs {$testfile}} res] } {
		puts "FAIL: Verification failed with $res"
	}

	# Set up second testfile so truncate flag is not needed.
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test$tnum-b.db
		set env NULL
	} else {
		set testfile test$tnum-b.db
		set env [lindex $dbargs $eindex]
		set testdir [get_home $env]
	}
	cleanup $testdir $env

	#
	# Test dupcompare with data items big enough to force offpage dups.
	#
	puts "\tTest$tnum.c:\
	    big key put/get dup loop key=filename data=filecontents"
	set db [eval {berkdb_open -dupcompare test094_cmp -dup -dupsort \
	     -create -mode 0644} $omethod $encargs $dbargs $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	# Here is the loop where we put and get each key/data pair
	set file_list [get_file_list 1]
	if { [llength $file_list] > $nentries } {
		set file_list [lrange $file_list 1 $nentries]
	}

	set count 0
	foreach f $file_list {
		set fid [open $f r]
		fconfigure $fid -translation binary
		set cont [read $fid]
		close $fid

		set key $f
		for {set i 0} {$i < $ndups} {incr i} {
			set data $i:$cont
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			set ret [eval {$db put} \
			    $txn $pflags {$key [chop_data $omethod $data]}]
			error_check_good put $ret 0
			if { $txnenv == 1 } {
				error_check_good txn [$t commit] 0
			}
		}

		set ret [eval {$db get} $gflags {$key}]
		error_check_good get [llength $ret] $ndups
		incr count
	}

	puts "\tTest$tnum.d: traverse checking duplicates before close"
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	dup_file_check $db $txn $t1 $dlist
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
		set testdir [get_home $env]
	}
	error_check_good db_close [$db close] 0

	# Run verify to check the internal structure and order.
	if { [catch {eval {berkdb dbverify} -dupcompare test094_cmp\
	    $envargs $encargs {$testfile}} res] } {
		puts "FAIL: Verification failed with $res"
	}

	# Clean up the test directory, otherwise the general verify
	# (without dupcompare) will fail.
	cleanup $testdir $env
}

# Simple dup comparison.
proc test094_cmp { a b } {
	return [string compare $b $a]
}
