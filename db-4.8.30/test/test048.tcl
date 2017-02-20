# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test048
# TEST	Cursor stability across Btree splits.
proc test048 { method args } {
	global errorCode
	global is_je_test
	source ./include.tcl

	set tnum 048
	set args [convert_args $method $args]

	if { [is_btree $method] != 1 } {
		puts "Test$tnum skipping for method $method."
		return
	}

	# Compression will change the behavior of page splits. 
	# Skip test for compression.
	if { [is_compressed $args] } {
		puts "Test$tnum skipping for compression"
		return
	}

	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		incr pgindex
		if { [lindex $args $pgindex] > 8192 } {
			puts "Test048: Skipping for large pagesizes"
			return
		}
	}

	set method "-btree"

	puts "\tTest$tnum: Test of cursor stability across btree splits."

	set key	"key"
	set data	"data"
	set txn ""
	set flags ""

	puts "\tTest$tnum.a: Create $method database."
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
		}
		set testdir [get_home $env]
	}
	set t1 $testdir/t1
	cleanup $testdir $env

	set oflags "-create -mode 0644 $args $method"
	set db [eval {berkdb_open} $oflags $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	set nkeys 5
	# Fill page w/ small key/data pairs, keep at leaf
	#
	puts "\tTest$tnum.b: Fill page with $nkeys small key/data pairs."
	for { set i 0 } { $i < $nkeys } { incr i } {
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval {$db put} $txn {key000$i $data$i}]
		error_check_good dbput $ret 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
	}

	# get db ordering, set cursors
	puts "\tTest$tnum.c: Set cursors on each of $nkeys pairs."
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	for {set i 0; set ret [$db get key000$i]} {\
			$i < $nkeys && [llength $ret] != 0} {\
			incr i; set ret [$db get key000$i]} {
		set key_set($i) [lindex [lindex $ret 0] 0]
		set data_set($i) [lindex [lindex $ret 0] 1]
		set dbc [eval {$db cursor} $txn]
		set dbc_set($i) $dbc
		error_check_good db_cursor:$i \
		    [is_valid_cursor $dbc_set($i) $db] TRUE
		set ret [$dbc_set($i) get -set $key_set($i)]
		error_check_bad dbc_set($i)_get:set [llength $ret] 0
	}

	# if mkeys is above 1000, need to adjust below for lexical order
	set mkeys 1000
	puts "\tTest$tnum.d: Add $mkeys pairs to force split."
	for {set i $nkeys} { $i < $mkeys } { incr i } {
		if { $i >= 100 } {
			set ret [eval {$db put} $txn {key0$i $data$i}]
		} elseif { $i >= 10 } {
			set ret [eval {$db put} $txn {key00$i $data$i}]
		} else {
			set ret [eval {$db put} $txn {key000$i $data$i}]
		}
		error_check_good dbput:more $ret 0
	}

	puts "\tTest$tnum.e: Make sure split happened."
	# XXX We cannot call stat with active txns or we deadlock.
	if { $txnenv != 1 && !$is_je_test } {
		error_check_bad stat:check-split [is_substr [$db stat] \
					"{{Internal pages} 0}"] 1
	}

	puts "\tTest$tnum.f: Check to see that cursors maintained reference."
	for {set i 0} { $i < $nkeys } {incr i} {
		set ret [$dbc_set($i) get -current]
		error_check_bad dbc$i:get:current [llength $ret] 0
		set ret2 [$dbc_set($i) get -set $key_set($i)]
		error_check_bad dbc$i:get:set [llength $ret2] 0
		error_check_good dbc$i:get(match) $ret $ret2
	}

	puts "\tTest$tnum.g: Delete added keys to force reverse split."
	for {set i $nkeys} { $i < $mkeys } { incr i } {
		if { $i >= 100 } {
			error_check_good db_del:$i \
			    [eval {$db del} $txn {key0$i}] 0
		} elseif { $i >= 10 } {
			error_check_good db_del:$i \
			    [eval {$db del} $txn {key00$i}] 0
		} else {
			error_check_good db_del:$i \
			    [eval {$db del} $txn {key000$i}] 0
		}
	}

	puts "\tTest$tnum.h: Verify cursor reference."
	for {set i 0} { $i < $nkeys } {incr i} {
		set ret [$dbc_set($i) get -current]
		error_check_bad dbc$i:get:current [llength $ret] 0
		set ret2 [$dbc_set($i) get -set $key_set($i)]
		error_check_bad dbc$i:get:set [llength $ret2] 0
		error_check_good dbc$i:get(match) $ret $ret2
	}

	puts "\tTest$tnum.i: Cleanup."
	# close cursors
	for {set i 0} { $i < $nkeys } {incr i} {
		error_check_good dbc_close:$i [$dbc_set($i) close] 0
	}
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	puts "\tTest$tnum.j: Verify reverse split."
	error_check_good stat:check-reverse_split [is_substr [$db stat] \
					"{{Internal pages} 0}"] 1

	error_check_good dbclose [$db close] 0

	puts "\tTest$tnum complete."
}
