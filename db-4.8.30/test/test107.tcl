# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test107
# TEST	Test of read-committed (degree 2 isolation). [#8689]
# TEST
# TEST	We set up a database.  Open a read-committed transactional cursor and
# TEST	a regular transactional cursor on it. Position each cursor on one page,
# TEST	and do a put to a different page.
# TEST
# TEST	Make sure that:
# TEST	- the put succeeds if we are using degree 2 isolation.
# TEST	- the put deadlocks within a regular transaction with
# TEST 	a regular cursor.
# TEST
proc test107 { method args } {
	source ./include.tcl
	global fixed_len
	global passwd
	set tnum "107"

	set pageargs ""
	split_pageargs $args pageargs
	# If we are using an env, then skip this test.  It needs its own.
	set eindex [lsearch -exact $args "-env"]
	if { $eindex != -1 } {
		incr eindex
		set env [lindex $args $eindex]
		puts "Test$tnum skipping for env $env"
		return
	}

	# We'll make the data pretty good sized so we can easily
	# move to a different page.  Make the data size a little
	# smaller for fixed-length methods so it works with
	# pagesize 512 tests.
	set data_size 512
	set orig_fixed_len $fixed_len
	set fixed_len [expr $data_size - [expr $data_size / 8]]
	set args [convert_args $method $args]
	set encargs ""
	set ddargs ""
	set args [split_encargs $args encargs]
	if { $encargs != "" } {
		set ddargs " -P $passwd "
	}
	set omethod [convert_method $method]

	puts "Test$tnum: Degree 2 Isolation Test ($method $args)"
	set testfile test$tnum.db
	env_cleanup $testdir

	# Create the environment.
	set timeout 10
	set env [eval {berkdb_env -create -mode 0644 -lock \
	    -cachesize { 0 1048576 1 } \
	    -lock_timeout $timeout -txn} $pageargs $encargs -home $testdir]
	error_check_good env_open [is_valid_env $env] TRUE

	# Create the database.
	set db [eval {berkdb_open -env $env -create -auto_commit\
	    -mode 0644 $omethod} $args {$testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE

	puts "\tTest$tnum.a: put loop"
	# The data doesn't need to change from key to key.
	# Use numerical keys so we don't need special handling for
	# record-based methods.
	set origdata "data"
	set len [string length $origdata]
	set data [repeat $origdata [expr $data_size / $len]]
	set nentries 200
	set txn [$env txn]
	for { set i 1 } { $i <= $nentries } { incr i } {
		set key $i
		set ret [eval {$db put} \
		    -txn $txn {$key [chop_data $method $data]}]
		error_check_good put:$db $ret 0
	}
	error_check_good txn_commit [$txn commit] 0

	puts "\tTest$tnum.b: Start deadlock detector."
	# Start up a deadlock detector so we can break self-deadlocks.
	set dpid [eval {exec $util_path/db_deadlock} -v -ae -t 1.0 \
	    -h $testdir $ddargs >& $testdir/dd.out &]

	puts "\tTest$tnum.c: Open txns and cursors."
	# We can get degree 2 isolation with either a degree 2
	# txn or a degree 2 cursor or both.  However, the case
	# of a regular txn and regular cursor should deadlock.
	# We put this case last so it won't deadlock the cases
	# which should succeed.
	#
	# Cursors and transactions are named according to
	# whether they specify degree 2 (c2, t2) or not (c, t).
	# Set up all four possibilities.
	#
	set t [$env txn]
	error_check_good reg_txn_begin [is_valid_txn $t $env] TRUE
	set t2 [$env txn -read_committed]
	error_check_good deg2_txn_begin [is_valid_txn $t2 $env] TRUE

	set c2t [$db cursor -txn $t -read_committed]
	error_check_good valid_c2t [is_valid_cursor $c2t $db] TRUE
	set ct2 [$db cursor -txn $t2]
	error_check_good valid_ct2 [is_valid_cursor $ct2 $db] TRUE
	set c2t2 [$db cursor -txn $t2 -read_committed]
	error_check_good valid_c2t2 [is_valid_cursor $c2t2 $db] TRUE
	set ct [$db cursor -txn $t]
	error_check_good valid_ct [is_valid_cursor $ct $db] TRUE

	set curslist [list $c2t $ct2 $c2t2 $ct]
	set newdata newdata
	set offpagekey [expr $nentries - 1]

	# For one cursor at a time, read the first item in the
	# database, then move to an item on a different page.
	# Put a new value in the first item on the first page.  This
	# should work with degree 2 isolation and hang without it.
	#
	# Wrap the whole thing in a catch statement so we still get
	# around to killing the deadlock detector and cleaning up
	# even if the test fails.
	#
	puts "\tTest$tnum.d: Test for read-committed (degree 2 isolation)."
	set status [catch {
		foreach cursor $curslist {
			set retfirst [$cursor get -first]
			set firstkey [lindex [lindex $retfirst 0] 0]
			set ret [$cursor get -set $offpagekey]
			error_check_good cursor_off_page \
			    [lindex [lindex $ret 0] 0] $offpagekey
			if { [catch {eval {$db put} \
			    $firstkey [chop_data $method $newdata]} res]} {
				error_check_good error_is_deadlock \
				    [is_substr $res DB_LOCK_DEADLOCK] 1
				error_check_good right_cursor_failed $cursor $ct
			} else {
				set ret [lindex [lindex [$db get $firstkey] 0] 1]
				error_check_good data_changed \
				    $ret [pad_data $method $newdata]
				error_check_bad right_cursor_succeeded $cursor $ct
			}
			error_check_good close_cursor [$cursor close] 0
		}
	} res]
	if { $status != 0 } {
		puts $res
	}

	# Smoke test for db_stat -txn -read_committed.
	puts "\tTest$tnum.e: Smoke test for db_stat -txn -read_committed"
	if { [catch {set statret [$db stat -txn $t -read_committed]} res] } {
		puts "FAIL: db_stat -txn -read_committed returned $res"
	}

	# End deadlock detection and clean up handles
	puts "\tTest$tnum.f: Clean up."
	tclkill $dpid
	set fixed_len $orig_fixed_len
	error_check_good t_commit [$t commit] 0
	error_check_good t2_commit [$t2 commit] 0
	error_check_good dbclose [$db close] 0
	error_check_good envclose [$env close] 0
}
