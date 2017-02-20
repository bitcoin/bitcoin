# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test088
# TEST	Test of cursor stability across btree splits with very
# TEST	deep trees (a variant of test048). [#2514]
proc test088 { method args } {
	source ./include.tcl
	global alphabet
	global errorCode
	global is_je_test

	set tstn 088
	set args [convert_args $method $args]

	if { [is_btree $method] != 1 } {
		puts "Test$tstn skipping for method $method."
		return
	}
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Test088: skipping for specific pagesizes"
		return
	}

	set method "-btree"

	puts "\tTest$tstn: Test of cursor stability across btree splits."

	set key	"key$alphabet$alphabet$alphabet"
	set data "data$alphabet$alphabet$alphabet"
	set txn ""
	set flags ""

	puts "\tTest$tstn.a: Create $method database."
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test$tstn.db
		set env NULL
	} else {
		set testfile test$tstn.db
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

	set ps 512
	set txn ""
	set oflags "-create -pagesize $ps -mode 0644 $args $method"
	set db [eval {berkdb_open} $oflags $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	set nkeys 5
	# Fill page w/ key/data pairs.
	#
	puts "\tTest$tstn.b: Fill page with $nkeys small key/data pairs."
	for { set i 0 } { $i < $nkeys } { incr i } {
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval {$db put} $txn {${key}00000$i $data$i}]
		error_check_good dbput $ret 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
	}

	# get db ordering, set cursors
	puts "\tTest$tstn.c: Set cursors on each of $nkeys pairs."
	# if mkeys is above 1000, need to adjust below for lexical order
	set mkeys 30000
	if { [is_compressed $args] } {
		set mkeys 300
	}
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
		set mkeys 300
	}
	for {set i 0; set ret [$db get ${key}00000$i]} {\
			$i < $nkeys && [llength $ret] != 0} {\
			incr i; set ret [$db get ${key}00000$i]} {
		set key_set($i) [lindex [lindex $ret 0] 0]
		set data_set($i) [lindex [lindex $ret 0] 1]
		set dbc [eval {$db cursor} $txn]
		set dbc_set($i) $dbc
		error_check_good db_cursor:$i [is_substr $dbc_set($i) $db] 1
		set ret [$dbc_set($i) get -set $key_set($i)]
		error_check_bad dbc_set($i)_get:set [llength $ret] 0
	}

	puts "\tTest$tstn.d: Add $mkeys pairs to force splits."
	for {set i $nkeys} { $i < $mkeys } { incr i } {
		if { $i >= 10000 } {
			set ret [eval {$db put} $txn {${key}0$i $data$i}]
		} elseif { $i >= 1000 } {
			set ret [eval {$db put} $txn {${key}00$i $data$i}]
		} elseif { $i >= 100 } {
			set ret [eval {$db put} $txn {${key}000$i $data$i}]
		} elseif { $i >= 10 } {
			set ret [eval {$db put} $txn {${key}0000$i $data$i}]
		} else {
			set ret [eval {$db put} $txn {${key}00000$i $data$i}]
		}
		error_check_good dbput:more $ret 0
	}

	puts "\tTest$tstn.e: Make sure splits happened."
	# XXX cannot execute stat in presence of txns and cursors.
	if { $txnenv == 0 && !$is_je_test } {
		error_check_bad stat:check-split [is_substr [$db stat] \
						"{{Internal pages} 0}"] 1
	}

	puts "\tTest$tstn.f: Check to see that cursors maintained reference."
	for {set i 0} { $i < $nkeys } {incr i} {
		set ret [$dbc_set($i) get -current]
		error_check_bad dbc$i:get:current [llength $ret] 0
		set ret2 [$dbc_set($i) get -set $key_set($i)]
		error_check_bad dbc$i:get:set [llength $ret2] 0
		error_check_good dbc$i:get(match) $ret $ret2
	}

	puts "\tTest$tstn.g: Delete added keys to force reverse splits."
	for {set i $nkeys} { $i < $mkeys } { incr i } {
		if { $i >= 10000 } {
			set ret [eval {$db del} $txn {${key}0$i}]
		} elseif { $i >= 1000 } {
			set ret [eval {$db del} $txn {${key}00$i}]
		} elseif { $i >= 100 } {
			set ret [eval {$db del} $txn {${key}000$i}]
		} elseif { $i >= 10 } {
			set ret [eval {$db del} $txn {${key}0000$i}]
		} else {
			set ret [eval {$db del} $txn {${key}00000$i}]
		}
		error_check_good dbput:more $ret 0
	}

	puts "\tTest$tstn.h: Verify cursor reference."
	for {set i 0} { $i < $nkeys } {incr i} {
		set ret [$dbc_set($i) get -current]
		error_check_bad dbc$i:get:current [llength $ret] 0
		set ret2 [$dbc_set($i) get -set $key_set($i)]
		error_check_bad dbc$i:get:set [llength $ret2] 0
		error_check_good dbc$i:get(match) $ret $ret2
	}

	puts "\tTest$tstn.i: Cleanup."
	# close cursors
	for {set i 0} { $i < $nkeys } {incr i} {
		error_check_good dbc_close:$i [$dbc_set($i) close] 0
	}
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good dbclose [$db close] 0

	puts "\tTest$tstn complete."
}
