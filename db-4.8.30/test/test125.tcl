# See the file LICENSE for redistribution information.
#
# Copyright (c) 2009-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test125
# TEST	Test cursor comparison API.
# TEST
# TEST	The cursor comparison API reports whether two cursors within
# TEST	the same database are at the same position.  It does not report
# TEST	any information about relative position.
# TEST
# TEST	1. Test two uninitialized cursors (error).
# TEST	2. Test one uninitialized cursor, one initialized (error). 
# TEST	3. Test two cursors in different databases (error).
# TEST	4. Put two cursors in the same place, test for match.  Walk
# TEST	them back and forth a bit, more matching. 
# TEST	5. Two cursors in the same spot.  Delete through one.  

proc test125 { method args } {
	global errorInfo
	source ./include.tcl
	set tnum 125

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	set txnenv 0
	set eindex [lsearch -exact $args "-env"]

	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test$tnum.db
		set testfile2 $testdir/test$tnum-2.db
		set env NULL
	} else {
		set testfile test$tnum.db
		set testfile2 test$tnum-2.db
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set testdir [get_home $env]
	}

	set t ""
	set txn ""

	# Run the test with and without duplicates, and with and without
	# large data items. 
	foreach dupflag { "" "-dup" "-dup -dupsort" } {
		if { [is_compressed $args] && $dupflag == "-dup" } {
			puts "Skipping unsorted dups for btree with compression"
			continue
		}
		foreach bigdata { 0 1 } {
			set msg ""
			if { $bigdata == 1 } {
				set msg "with big data"
			}
			puts "Test$tnum ($method $dupflag $msg):\
			    Cursor comparison API."
			if { [llength $dupflag] > 0 } {
				if { [is_record_based $method] ||\
				    [is_rbtree $method] } {
					puts "Skipping test for method $method\
					    with duplicates."
					continue
				}
				set dups 1
			} else { 
				set dups 0
			}

			# Testdir will get reset from the env's home dir back
			# to the default if this calls something that sources
			# include.tcl, since testdir is a global.  Set it correctly
			# here each time through the loop.
			#
			if { $env != "NULL" } {
				set testdir [get_home $env]
			}
			cleanup $testdir $env

			puts "\tTest$tnum.a: Test failure cases." 
			# Open two databases.
			set db [eval {berkdb_open_noerr} -create -mode 0644 \
			    $omethod $args $dupflag {$testfile}]
			error_check_good db_open [is_valid_db $db] TRUE
			set db2 [eval {berkdb_open_noerr} -create -mode 0644 \
			    $omethod $args $dupflag {$testfile2}]
			error_check_good db2_open [is_valid_db $db2] TRUE

			# Populate the databases.
			set nentries 1000
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			populate $db $method $t $nentries $dups $bigdata
			populate $db2 $method $t $nentries $dups $bigdata

			# Test error conditions.
			puts "\t\tTest$tnum.a1: Uninitialized cursors."
			set c1 [eval {$db cursor} $txn]
			set c2 [eval {$db cursor} $txn]
			set ret [catch {$c1 cmp $c2} res]
			error_check_good uninitialized_cursors $ret 1

			puts "\t\tTest$tnum.a2:\
			    One initialized, one uninitialized cursor."
			$c1 get -first
			set ret [catch {$c1 cmp $c2} res]
			error_check_good one_uninitialized_cursor $ret 1

			puts "\t\tTest$tnum.a3: Cursors in different databases."
			set c3 [eval {$db2 cursor} $txn]
			set ret [$c3 get -first]
			set ret [catch {$c1 cmp $c3} res]
			error_check_good cursors_in_different_databases $ret 1

			# Clean up second database - we won't be using it again.
			$c3 close
			$db2 close
			
			# Test valid conditions.  
			# 
			# Initialize second cursor to -first.  Cursor cmp should
			# match; c1 was already there. 
			puts "\tTest$tnum.b: Cursors initialized to -first."
			set ret [$c2 get -first]
			error_check_good c1_and_c2_on_first [$c1 cmp $c2] 0 

			# Walk to the end.  We should alternate between 
			# matching and not matching. 
			puts "\tTest$tnum.c: Walk cursors to the last item."
			for { set i 1 } { $i < $nentries } { incr i } {

				# First move c1; cursors won't match.
				set ret [$c1 get -next]
				error_check_bad cmp_does_not_match [$c1 cmp $c2] 0

				# Now move c2; cursors will match again. 
				set ret [$c2 get -next]
				error_check_good cmp_matches [$c1 cmp $c2] 0
			}	

			# Now do it in reverse, starting at -last and backing up. 
			puts "\tTest$tnum.d: Cursors initialized to -last."
			set ret [$c1 get -last]
			set ret [$c2 get -last]
			error_check_good c1_and_c2_on_last [$c1 cmp $c2] 0 

			puts "\tTest$tnum.e: Walk cursors back to the first item."
			for { set i 1 } { $i < $nentries } { incr i } {

				# First move c1; cursors won't match.
				set ret [$c1 get -prev]
				error_check_bad cmp_does_not_match [$c1 cmp $c2] 0

				# Now move c2; cursors will match again. 
				set ret [$c2 get -prev]
				error_check_good cmp_matches [$c1 cmp $c2] 0
			}	

			# A cursor delete leaves the cursor in the same place, so a 
			# comparison should still work.
			puts "\tTest$tnum.f:\
			    Position comparison works with cursor deletes."
			set ret [$c1 get -first]
			set ret [$c2 get -first]

			# Do the cursor walk again, deleting as we go. 
			puts "\tTest$tnum.g: Cursor walk with deletes."
			for { set i 1 } { $i < $nentries } { incr i } {

				# First move c1; cursors won't match.
				set ret [$c1 get -next]
				error_check_bad cmp_does_not_match [$c1 cmp $c2] 0

				# Now move c2; cursors will match again. 
				set ret [$c2 get -next]
				error_check_good cmp_matches [$c1 cmp $c2] 0

				# Now delete through c2; cursors should still match.
				set ret [$c2 del]
				error_check_good cmp_still_matches [$c1 cmp $c2] 0
			}	

			# Close cursors and database; commit txn.
			error_check_good c1_close [$c1 close] 0
			error_check_good c2_close [$c2 close] 0
			if { $txnenv == 1 } {
				error_check_good txn [$t commit] 0
			}
	
			error_check_good db_close [$db close] 0
		}
	}
}
