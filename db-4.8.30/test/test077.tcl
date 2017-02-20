# See the file LICENSE for redistribution information.
#
# Copyright (c) 2000-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test077
# TEST	Test of DB_GET_RECNO [#1206].
proc test077 { method { nkeys 1000 } { tnum "077" } args } {
	source ./include.tcl
	global alphabet

	set omethod [convert_method $method]
	set args [convert_args $method $args]

	puts "Test$tnum: Test of DB_GET_RECNO."

	if { [is_rbtree $method] != 1 } {
		puts "\tTest$tnum: Skipping for method $method."
		return
	}

	set data $alphabet

	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
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
	cleanup $testdir $env

	set db [eval {berkdb_open -create -mode 0644} \
	    $omethod $args {$testfile}]
	error_check_good db_open [is_valid_db $db] TRUE

	puts "\tTest$tnum.a: Populating database."
	set txn ""

	for { set i 1 } { $i <= $nkeys } { incr i } {
		set key [format %5d $i]
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval {$db put} $txn {$key $data}]
		error_check_good db_put($key) $ret 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
	}

	puts "\tTest$tnum.b: Verifying record numbers."

	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good dbc_open [is_valid_cursor $dbc $db] TRUE

	set i 1
	for { set dbt [$dbc get -first] } \
	    { [string length $dbt] != 0 } \
	    { set dbt [$dbc get -next] } {
		set recno [$dbc get -get_recno]
		set keynum [expr [lindex [lindex $dbt 0] 0]]

		# Verify that i, the number that is the key, and recno
		# are all equal.
		error_check_good key($i) $keynum $i
		error_check_good recno($i) $recno $i
		incr i
	}

	error_check_good dbc_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0
}
