# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST lock005
# TEST Check that page locks are being released properly.

proc lock005 { } {
	source ./include.tcl

	puts "Lock005: Page lock release test"

	# Clean up after previous runs
	env_cleanup $testdir

	# Open/create the lock region
	set e [berkdb_env -create -lock -home $testdir -txn -log]
	error_check_good env_open [is_valid_env $e] TRUE

	# Open/create the database
	set db [berkdb open -create -auto_commit -env $e -len 10 -queue q.db]
	error_check_good dbopen [is_valid_db $db] TRUE

	# Check that records are locking by trying to
	# fetch a record on the wrong transaction.
	puts "\tLock005.a: Verify that we are locking"

	# Start the first transaction
	set txn1 [$e txn -nowait]
	error_check_good txn_begin [is_valid_txn $txn1 $e] TRUE
	set ret [catch {$db put -txn $txn1 -append record1} recno1]
	error_check_good dbput_txn1 $ret 0

	# Start second txn while the first is still running ...
	set txn2 [$e txn -nowait]
	error_check_good txn_begin [is_valid_txn $txn2 $e] TRUE

	# ... and try to get a record from the first txn (should fail)
	set ret [catch {$db get -txn $txn2 $recno1} res]
	error_check_good dbget_wrong_record \
	    [is_substr $res "deadlock"] 1

	# End transactions
	error_check_good txn1commit [$txn1 commit] 0
	how_many_locks 1 $e
	error_check_good txn2commit [$txn2 commit] 0
	# The number of locks stays the same here because the first
	# lock is released and the second lock was never granted.
	how_many_locks 1 $e

	# Test lock behavior for both abort and commit
	puts "\tLock005.b: Verify locks after abort or commit"
	foreach endorder {forward reverse} {
		end_order_test $db $e commit abort $endorder
		end_order_test $db $e abort commit $endorder
		end_order_test $db $e commit commit $endorder
		end_order_test $db $e abort abort $endorder
	}

	# Clean up
	error_check_good db_close [$db close] 0
	error_check_good env_close [$e close] 0
}

proc end_order_test { db e txn1end txn2end endorder } {
	# Start one transaction
	set txn1 [$e txn -nowait]
	error_check_good txn_begin [is_valid_txn $txn1 $e] TRUE
	set ret [catch {$db put -txn $txn1 -append record1} recno1]
	error_check_good dbput_txn1 $ret 0

	# Check number of locks
	how_many_locks 2 $e

	# Start a second transaction while first is still running
	set txn2 [$e txn -nowait]
	error_check_good txn_begin [is_valid_txn $txn2 $e] TRUE
	set ret [catch {$db put -txn $txn2 -append record2} recno2]
	error_check_good dbput_txn2 $ret 0
	how_many_locks 3 $e

	# Now commit or abort one txn and make sure the other is okay
	if {$endorder == "forward"} {
		# End transaction 1 first
		puts "\tLock005.b.1: $txn1end txn1 then $txn2end txn2"
		error_check_good txn_$txn1end [$txn1 $txn1end] 0
		how_many_locks 2 $e

		# txn1 is now ended, but txn2 is still running
		set ret1 [catch {$db get -txn $txn2 $recno1} res1]
		set ret2 [catch {$db get -txn $txn2 $recno2} res2]
		if { $txn1end == "commit" } {
			error_check_good txn2_sees_txn1 $ret1 0
			error_check_good txn2_sees_txn2 $ret2 0
		} else {
			# transaction 1 was aborted
			error_check_good txn2_cantsee_txn1 [llength $res1] 0
		}

		# End transaction 2 second
		error_check_good txn_$txn2end [$txn2 $txn2end] 0
		how_many_locks 1 $e

		# txn1 and txn2 should both now be invalid
		# The get no longer needs to be transactional
		set ret3 [catch {$db get $recno1} res3]
		set ret4 [catch {$db get $recno2} res4]

		if { $txn2end == "commit" } {
			error_check_good txn2_sees_txn1 $ret3 0
			error_check_good txn2_sees_txn2 $ret4 0
			error_check_good txn2_has_record2 \
			    [is_substr $res4 "record2"] 1
		} else {
			# transaction 2 was aborted
			error_check_good txn2_cantsee_txn1 $ret3 0
			error_check_good txn2_aborted [llength $res4] 0
		}

	} elseif { $endorder == "reverse" } {
		# End transaction 2 first
		puts "\tLock005.b.2: $txn2end txn2 then $txn1end txn1"
		error_check_good txn_$txn2end [$txn2 $txn2end] 0
		how_many_locks 2 $e

		# txn2 is ended, but txn1 is still running
		set ret1 [catch {$db get -txn $txn1 $recno1} res1]
		set ret2 [catch {$db get -txn $txn1 $recno2} res2]
		if { $txn2end == "commit" } {
			error_check_good txn1_sees_txn1 $ret1 0
			error_check_good txn1_sees_txn2 $ret2 0
		} else {
			# transaction 2 was aborted
			error_check_good txn1_cantsee_txn2 [llength $res2] 0
		}

		# End transaction 1 second
		error_check_good txn_$txn1end [$txn1 $txn1end] 0
		how_many_locks 1 $e

		# txn1 and txn2 should both now be invalid
		# The get no longer needs to be transactional
		set ret3 [catch {$db get $recno1} res3]
		set ret4 [catch {$db get $recno2} res4]

		if { $txn1end == "commit" } {
			error_check_good txn1_sees_txn1 $ret3 0
			error_check_good txn1_sees_txn2 $ret4 0
			error_check_good txn1_has_record1 \
			    [is_substr $res3 "record1"] 1
		} else {
			# transaction 1 was aborted
			error_check_good txn1_cantsee_txn2 $ret4 0
			error_check_good txn1_aborted [llength $res3] 0
		}
	}
}

proc how_many_locks { expected env } {
	set stat [$env lock_stat]
	set str "Current number of locks"
	set checked 0
	foreach statpair $stat {
		if { $checked == 1 } {
			break
		}
		if { [is_substr [lindex $statpair 0] $str] != 0} {
			set checked 1
			set nlocks [lindex $statpair 1]
			error_check_good expected_nlocks $nlocks $expected
		}
	}
	error_check_good checked $checked 1
}
