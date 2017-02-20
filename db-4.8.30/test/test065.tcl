# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test065
# TEST	Test of DB->stat, both -DB_FAST_STAT and row
# TEST	counts with DB->stat -txn.
proc test065 { method args } {
	source ./include.tcl
	global errorCode
	global alphabet

	set nentries 10000
	set args [convert_args $method $args]
	set omethod [convert_method $method]
	set tnum "065"

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
	cleanup $testdir $env

	puts "Test$tnum: $method ($args) DB->stat(DB_FAST_STAT) test."

	puts "\tTest$tnum.a: Create database and check it while empty."

	set db [eval {berkdb_open_noerr -create -mode 0644} \
	    $omethod $args $testfile]
	error_check_good db_open [is_valid_db $db] TRUE

	set ret [catch {eval $db stat -faststat} res]

	error_check_good db_close [$db close] 0

	if { ([is_record_based $method] && ![is_queue $method]) \
	    || [is_rbtree $method] } {
		error_check_good recordcount_ok [is_substr $res \
		    "{{Number of keys} 0}"] 1
	} else {
		puts "\tTest$tnum: Test complete for method $method."
		return
	}

	# If we've got this far, we're on an access method for
	# which record counts makes sense.  Thus, we no longer
	# catch EINVALs, and no longer care about __db_errs.
	set db [eval {berkdb_open -create -mode 0644} $omethod $args $testfile]

	puts "\tTest$tnum.b: put $nentries keys."

	if { [is_record_based $method] } {
		set gflags " -recno "
		set keypfx ""
	} else {
		set gflags ""
		set keypfx "key"
	}

	set txn ""
	set data [pad_data $method $alphabet]

	for { set ndx 1 } { $ndx <= $nentries } { incr ndx } {
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval {$db put} $txn {$keypfx$ndx $data}]
		error_check_good db_put $ret 0
		set statret [eval {$db stat} $txn]
		set rowcount [getstats $statret "Number of records"]
		error_check_good rowcount $rowcount $ndx
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
	}

	set ret [$db stat -faststat]
	error_check_good recordcount_after_puts \
	    [is_substr $ret "{{Number of keys} $nentries}"] 1

	puts "\tTest$tnum.c: delete 90% of keys."
	set end [expr {$nentries / 10 * 9}]
	for { set ndx 1 } { $ndx <= $end } { incr ndx } {
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		if { [is_rrecno $method] == 1 } {
			# if we're renumbering, when we hit key 5001 we'll
			# have deleted 5000 and we'll croak!  So delete key
			# 1, repeatedly.
			set ret [eval {$db del} $txn {[concat $keypfx 1]}]
			set statret [eval {$db stat} $txn]
			set rowcount [getstats $statret "Number of records"]
			error_check_good rowcount $rowcount [expr $nentries - $ndx]
		} else {
			set ret [eval {$db del} $txn {$keypfx$ndx}]
			set rowcount [getstats $statret "Number of records"]
			error_check_good rowcount $rowcount $nentries
		}
		error_check_good db_del $ret 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
	}

	set ret [$db stat -faststat]
	if { [is_rrecno $method] == 1 || [is_rbtree $method] == 1 } {
		# We allow renumbering--thus the stat should return 10%
		# of nentries.
		error_check_good recordcount_after_dels [is_substr $ret \
		    "{{Number of keys} [expr {$nentries / 10}]}"] 1
	} else {
		# No renumbering--no change in RECORDCOUNT!
		error_check_good recordcount_after_dels \
		    [is_substr $ret "{{Number of keys} $nentries}"] 1
	}

	puts "\tTest$tnum.d: put new keys at the beginning."
	set end [expr {$nentries / 10 * 8}]
	for { set ndx 1 } { $ndx <= $end } {incr ndx } {
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval {$db put} $txn {$keypfx$ndx $data}]
		error_check_good db_put_beginning $ret 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
	}

	set ret [$db stat -faststat]
	if { [is_rrecno $method] == 1 } {
		# With renumbering we're back up to 80% of $nentries
		error_check_good recordcount_after_dels [is_substr $ret \
		    "{{Number of keys} [expr {$nentries / 10 * 8}]}"] 1
	} elseif { [is_rbtree $method] == 1 } {
		# Total records in a btree is now 90% of $nentries
		error_check_good recordcount_after_dels [is_substr $ret \
		    "{{Number of keys} [expr {$nentries / 10 * 9}]}"] 1
	} else {
		# No renumbering--still no change in RECORDCOUNT.
		error_check_good recordcount_after_dels [is_substr $ret \
		    "{{Number of keys} $nentries}"] 1
	}

	puts "\tTest$tnum.e: put new keys at the end."
	set start [expr {1 + $nentries / 10 * 9}]
	set end [expr {($nentries / 10 * 9) + ($nentries / 10 * 8)}]
	for { set ndx $start } { $ndx <= $end } { incr ndx } {
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval {$db put} $txn {$keypfx$ndx $data}]
		error_check_good db_put_end $ret 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
	}

	set ret [$db stat -faststat]
	if { [is_rbtree $method] != 1 } {
		# If this is a recno database, the record count should be up
		# to (1.7 x nentries), the largest number we've seen, with
		# or without renumbering.
		error_check_good recordcount_after_puts2 [is_substr $ret \
		    "{{Number of keys} [expr {$start - 1 + $nentries / 10 * 8}]}"] 1
	} else {
		# In an rbtree, 1000 of those keys were overwrites, so there
		# are (.7 x nentries) new keys and (.9 x nentries) old keys
		# for a total of (1.6 x nentries).
		error_check_good recordcount_after_puts2 [is_substr $ret \
		    "{{Number of keys} [expr {$start -1 + $nentries / 10 * 7}]}"] 1
	}

	error_check_good db_close [$db close] 0
}
