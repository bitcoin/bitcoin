# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	sdb005
# TEST	Tests cursor operations in subdbs
# TEST		Put/get per key
# TEST		Verify cursor operations work within subdb
# TEST		Verify cursor operations do not work across subdbs
# TEST
#
# We should test this on all btrees, all hash, and a combination thereof
proc sdb005 {method {nentries 100} args } {
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	if { [is_queue $method] == 1 } {
		puts "Subdb005: skipping for method $method"
		return
	}

	puts "Subdb005: $method ( $args ) subdb cursor operations test"
	set txnenv 0
	set envargs ""
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/subdb005.db
		set env NULL
	} else {
		set testfile subdb005.db
		incr eindex
		set env [lindex $args $eindex]
		set envargs " -env $env "
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
			append envargs " -auto_commit "
			if { $nentries == 100 } {
				set nentries 20
			}
		}
		set testdir [get_home $env]
	}

	cleanup $testdir $env
	set txn ""
	set psize 8192
	set duplist {-1 -1 -1 -1 -1}
	build_all_subdb \
	    $testfile [list $method] $psize $duplist $nentries $args
	set numdb [llength $duplist]
	#
	# Get a cursor in each subdb and move past the end of each
	# subdb.  Make sure we don't end up in another subdb.
	#
	puts "\tSubdb005.a: Cursor ops - first/prev and last/next"
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	for {set i 0} {$i < $numdb} {incr i} {
		set db [eval {berkdb_open -unknown} $args {$testfile sub$i.db}]
		error_check_good dbopen [is_valid_db $db] TRUE
		set db_handle($i) $db
		# Used in 005.c test
		lappend subdbnames sub$i.db

		set dbc [eval {$db cursor} $txn]
		error_check_good db_cursor [is_valid_cursor $dbc $db] TRUE

		# Get a second cursor for cursor comparison test.
		set dbc2 [eval {$db cursor} $txn]
		error_check_good db_cursor2 [is_valid_cursor $dbc2 $db] TRUE

		set d [$dbc get -first]
		set d2 [$dbc2 get -first]
		error_check_good dbc_get [expr [llength $d] != 0] 1

		# Cursor comparison: both are on get -first.
		error_check_good dbc2_cmp [$dbc cmp $dbc2] 0

		# Used in 005.b test
		set db_key($i) [lindex [lindex $d 0] 0]

		set d [$dbc get -prev]
		error_check_good dbc_get [expr [llength $d] == 0] 1

		set d [$dbc get -last]
		error_check_good dbc_get [expr [llength $d] != 0] 1

		# Cursor comparison: the first cursor has moved to
		# get -last.
		error_check_bad dbc2_cmp [$dbc cmp $dbc2] 0

		set d [$dbc get -next]
		error_check_good dbc_get [expr [llength $d] == 0] 1
		error_check_good dbc_close [$dbc close] 0
		error_check_good dbc2_close [$dbc2 close] 0
	}
	#
	# Get a key from each subdb and try to get this key in a
	# different subdb.  Make sure it fails
	#
	puts "\tSubdb005.b: Get keys in different subdb's"
	for {set i 0} {$i < $numdb} {incr i} {
		set n [expr $i + 1]
		if {$n == $numdb} {
			set n 0
		}
		set db $db_handle($i)
		if { [is_record_based $method] == 1 } {
			set d [eval {$db get -recno} $txn {$db_key($n)}]
			error_check_good \
			    db_get [expr [llength $d] == 0] 1
		} else {
			set d [eval {$db get} $txn {$db_key($n)}]
			error_check_good db_get [expr [llength $d] == 0] 1
		}
	}
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	#
	# Clean up
	#
	for {set i 0} {$i < $numdb} {incr i} {
		error_check_good db_close [$db_handle($i) close] 0
	}

	#
	# Check contents of DB for subdb names only.  Makes sure that
	# every subdbname is there and that nothing else is there.
	#
	puts "\tSubdb005.c: Check DB is read-only"
	error_check_bad dbopen [catch \
	     {berkdb_open_noerr -unknown $testfile} ret] 0

	puts "\tSubdb005.d: Check contents of DB for subdb names only"
	set db [eval {berkdb_open -unknown -rdonly} $envargs {$testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE
	set subdblist [$db get -glob *]
	foreach kd $subdblist {
		# subname also used in subdb005.e,f below
		set subname [lindex $kd 0]
		set i [lsearch $subdbnames $subname]
		error_check_good subdb_search [expr $i != -1] 1
		set subdbnames [lreplace $subdbnames $i $i]
	}
	error_check_good subdb_done [llength $subdbnames] 0

	error_check_good db_close [$db close] 0
	return
}
