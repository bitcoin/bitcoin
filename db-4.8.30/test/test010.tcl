# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test010
# TEST	Duplicate test
# TEST		Small key/data pairs.
# TEST
# TEST	Use the first 10,000 entries from the dictionary.
# TEST	Insert each with self as key and data; add duplicate records for each.
# TEST	After all are entered, retrieve all; verify output.
# TEST	Close file, reopen, do retrieve and re-verify.
# TEST	This does not work for recno
proc test010 { method {nentries 10000} {ndups 5} {tnum "010"} args } {
	source ./include.tcl

	set omethod $method
	set args [convert_args $method $args]
	set omethod [convert_method $method]

	# Btree with compression does not support unsorted duplicates.
	if { [is_compressed $args] == 1 } {
		puts "Test$tnum skipping for btree with compression."
		return
	}

	if { [is_record_based $method] == 1 || \
	    [is_rbtree $method] == 1 } {
		puts "Test$tnum skipping for method $method"
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
			reduce_dups nentries ndups
		}
		set testdir [get_home $env]
	}
	puts "Test$tnum: $method ($args) $nentries \
	    small $ndups dup key/data pairs"

	set t1 $testdir/t1
	set t2 $testdir/t2
	set t3 $testdir/t3

	cleanup $testdir $env

	set db [eval {berkdb_open \
	     -create -mode 0644 -dup} $args {$omethod $testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE

	set did [open $dict]

	set pflags ""
	set gflags ""
	set txn ""
	set count 0

	# Here is the loop where we put and get each key/data pair
	while { [gets $did str] != -1 && $count < $nentries } {
		for { set i 1 } { $i <= $ndups } { incr i } {
			set datastr $i:$str
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			set ret [eval {$db put} \
			    $txn $pflags {$str [chop_data $method $datastr]}]
			error_check_good put $ret 0
			if { $txnenv == 1 } {
				error_check_good txn [$t commit] 0
			}
		}

		# Now retrieve all the keys matching this key
		set x 1
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set dbc [eval {$db cursor} $txn]
		for {set ret [$dbc get "-set" $str]} \
		    {[llength $ret] != 0} \
		    {set ret [$dbc get "-next"] } {
			if {[llength $ret] == 0} {
				break
			}
			set k [lindex [lindex $ret 0] 0]
			if { [string compare $k $str] != 0 } {
				break
			}
			set datastr [lindex [lindex $ret 0] 1]
			set d [data_of $datastr]
			error_check_good "Test$tnum:get" $d $str
			set id [ id_of $datastr ]
			error_check_good "Test$tnum:dup#" $id $x
			incr x
		}
		error_check_good "Test$tnum:ndups:$str" [expr $x - 1] $ndups
		error_check_good cursor_close [$dbc close] 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}

		incr count
	}
	close $did

	# Now we will get each key from the DB and compare the results
	# to the original.
	puts "\tTest$tnum.a: Checking file for correct duplicates"
	set dlist ""
	for { set i 1 } { $i <= $ndups } {incr i} {
		lappend dlist $i
	}
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	dup_check $db $txn $t1 $dlist
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}

	# Now compare the keys to see if they match the dictionary entries
	set q q
	filehead $nentries $dict $t3
	filesort $t3 $t2
	filesort $t1 $t3

	error_check_good Test$tnum:diff($t3,$t2) \
	    [filecmp $t3 $t2] 0

	error_check_good db_close [$db close] 0
	set db [eval {berkdb_open} $args $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	puts "\tTest$tnum.b: Checking file for correct duplicates after close"
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	dup_check $db $txn $t1 $dlist
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}

	# Now compare the keys to see if they match the dictionary entries
	filesort $t1 $t3
	error_check_good Test$tnum:diff($t3,$t2) \
	    [filecmp $t3 $t2] 0

	error_check_good db_close [$db close] 0
}
