# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test047
# TEST	DBcursor->c_get get test with SET_RANGE option.
proc test047 { method args } {
	source ./include.tcl

	set tnum 047
	set args [convert_args $method $args]

	if { [is_btree $method] != 1 } {
		puts "Test$tnum skipping for method $method"
		return
	}
	# Btree with compression does not support unsorted duplicates.
	if { [is_compressed $args] == 1 } {
		puts "Test$tnum skipping for btree with compression."
		return
	}
	set method "-btree"

	puts "\tTest$tnum: Test of SET_RANGE interface to DB->c_get ($method)."

	set key	"key"
	set data	"data"
	set txn ""
	set flags ""

	puts "\tTest$tnum.a: Create $method database."
	set eindex [lsearch -exact $args "-env"]
	set txnenv 0
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test$tnum.db
		set testfile1 $testdir/test$tnum.a.db
		set testfile2 $testdir/test$tnum.b.db
		set env NULL
	} else {
		set testfile test$tnum.db
		set testfile1 test$tnum.a.db
		set testfile2 test$tnum.b.db
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set testdir [get_home $env]
	}
	set t1 $testdir/t1
	cleanup $testdir $env

	set oflags "-create -mode 0644 -dup $args $method"
	set db [eval {berkdb_open} $oflags $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	set nkeys 20
	# Fill page w/ small key/data pairs
	#
	puts "\tTest$tnum.b: Fill page with $nkeys small key/data pairs."
	for { set i 0 } { $i < $nkeys } { incr i } {
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval {$db put} $txn {$key$i $data$i}]
		error_check_good dbput $ret 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
	}

	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	# open curs to db
	set dbc [eval {$db cursor} $txn]
	error_check_good db_cursor [is_valid_cursor $dbc $db] TRUE

	puts "\tTest$tnum.c: Get data with SET_RANGE, then delete by cursor."
	set i 0
	set ret [$dbc get -set_range $key$i]
	error_check_bad dbc_get:set_range [llength $ret] 0
	set curr $ret

	# delete by cursor, make sure it is gone
	error_check_good dbc_del [$dbc del] 0

	set ret [$dbc get -set_range $key$i]
	error_check_bad dbc_get(post-delete):set_range [llength $ret] 0
	error_check_bad dbc_get(no-match):set_range $ret $curr

	puts "\tTest$tnum.d: \
	    Use another cursor to fix item on page, delete by db."
	set dbcurs2 [eval {$db cursor} $txn]
	error_check_good db:cursor2 [is_valid_cursor $dbcurs2 $db] TRUE

	set ret [$dbcurs2 get -set [lindex [lindex $ret 0] 0]]
	error_check_bad dbc_get(2):set [llength $ret] 0
	set curr $ret
	error_check_good db:del [eval {$db del} $txn \
	    {[lindex [lindex $ret 0] 0]}] 0

	# make sure item is gone
	set ret [$dbcurs2 get -set_range [lindex [lindex $curr 0] 0]]
	error_check_bad dbc2_get:set_range [llength $ret] 0
	error_check_bad dbc2_get:set_range $ret $curr

	puts "\tTest$tnum.e: Close for second part of test, close db/cursors."
	error_check_good dbc:close [$dbc close] 0
	error_check_good dbc2:close [$dbcurs2 close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good dbclose [$db close] 0

	# open db
	set db [eval {berkdb_open} $oflags $testfile1]
	error_check_good dbopen2 [is_valid_db $db] TRUE

	set nkeys 10
	puts "\tTest$tnum.f: Fill page with $nkeys pairs, one set of dups."
	for {set i 0} { $i < $nkeys } {incr i} {
		# a pair
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval {$db put} $txn {$key$i $data$i}]
		error_check_good dbput($i) $ret 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
	}

	set j 0
	for {set i 0} { $i < $nkeys } {incr i} {
		# a dup set for same  1 key
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval {$db put} $txn {$key$i DUP_$data$i}]
		error_check_good dbput($i):dup $ret 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
	}

	puts "\tTest$tnum.g: \
	    Get dups key w/ SET_RANGE, pin onpage with another cursor."
	set i 0
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good db_cursor [is_valid_cursor $dbc $db] TRUE
	set ret [$dbc get -set_range $key$i]
	error_check_bad dbc_get:set_range [llength $ret] 0

	set dbc2 [eval {$db cursor} $txn]
	error_check_good db_cursor [is_valid_cursor $dbc2 $db] TRUE
	set ret2 [$dbc2 get -set_range $key$i]
	error_check_bad dbc2_get:set_range [llength $ret] 0

	error_check_good dbc_compare $ret $ret2
	puts "\tTest$tnum.h: \
	    Delete duplicates' key, use SET_RANGE to get next dup."
	set ret [$dbc2 del]
	error_check_good dbc2_del $ret 0
	set ret [$dbc get -set_range $key$i]
	error_check_bad dbc_get:set_range [llength $ret] 0
	error_check_bad dbc_get:set_range $ret $ret2

	error_check_good dbc_close [$dbc close] 0
	error_check_good dbc2_close [$dbc2 close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0

	set db [eval {berkdb_open} $oflags $testfile2]
	error_check_good dbopen [is_valid_db $db] TRUE

	set nkeys 10
	set ndups 1000

	puts "\tTest$tnum.i: Fill page with $nkeys pairs and $ndups dups."
	for {set i 0} { $i < $nkeys } { incr i} {
		# a pair
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval {$db put} $txn {$key$i $data$i}]
		error_check_good dbput $ret 0

		# dups for single pair
		if { $i == 0} {
			for {set j 0} { $j < $ndups } { incr j } {
				set ret [eval {$db put} $txn \
				    {$key$i DUP_$data$i:$j}]
				error_check_good dbput:dup $ret 0
			}
		}
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
	}
	set i 0
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good db_cursor [is_valid_cursor $dbc $db] TRUE
	set dbc2 [eval {$db cursor} $txn]
	error_check_good db_cursor [is_valid_cursor $dbc2 $db] TRUE
	puts "\tTest$tnum.j: \
	    Get key of first dup with SET_RANGE, fix with 2 curs."
	set ret [$dbc get -set_range $key$i]
	error_check_bad dbc_get:set_range [llength $ret] 0

	set ret2 [$dbc2 get -set_range $key$i]
	error_check_bad dbc2_get:set_range [llength $ret] 0
	set curr $ret2

	error_check_good dbc_compare $ret $ret2

	puts "\tTest$tnum.k: Delete item by cursor, use SET_RANGE to verify."
	set ret [$dbc2 del]
	error_check_good dbc2_del $ret 0
	set ret [$dbc get -set_range $key$i]
	error_check_bad dbc_get:set_range [llength $ret] 0
	error_check_bad dbc_get:set_range $ret $curr

	puts "\tTest$tnum.l: Cleanup."
	error_check_good dbc_close [$dbc close] 0
	error_check_good dbc2_close [$dbc2 close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0

	puts "\tTest$tnum complete."
}
