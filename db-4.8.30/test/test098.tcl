# See the file LICENSE for redistribution information.
#
# Copyright (c) 2002-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test098
# TEST	Test of DB_GET_RECNO and secondary indices.  Open a primary and
# TEST	a secondary, and do a normal cursor get followed by a get_recno.
# TEST	(This is a smoke test for "Bug #1" in [#5811].)

proc test098 { method args } {
	source ./include.tcl

	set omethod [convert_method $method]
	set args [convert_args $method $args]

	puts "Test098: $omethod ($args): DB_GET_RECNO and secondary indices."

	if { [is_rbtree $method] != 1 } {
		puts "\tTest098: Skipping for method $method."
		return
	}

	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	set txn ""
	set auto ""
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set base $testdir/test098
		set env NULL
	} else {
		set base test098
		incr eindex
		set env [lindex $args $eindex]
		set rpcenv [is_rpcenv $env]
		if { $rpcenv == 1 } {
			puts "Test098: Skipping for RPC"
			return
		}
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			set auto " -auto_commit "
		}
		set testdir [get_home $env]
	}
	cleanup $testdir $env

	puts "\tTest098.a: Set up databases."

	set adb [eval {berkdb_open} $omethod $args $auto \
	    {-create} $base-primary.db]
	error_check_good adb_create [is_valid_db $adb] TRUE

	set bdb [eval {berkdb_open} $omethod $args $auto \
	    {-create} $base-secondary.db]
	error_check_good bdb_create [is_valid_db $bdb] TRUE

	set ret [eval $adb associate $auto [callback_n 0] $bdb]
	error_check_good associate $ret 0

	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set ret [eval {$adb put} $txn aaa data1]
	error_check_good put $ret 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}

	set bc [$bdb cursor]
	error_check_good cursor [is_valid_cursor $bc $bdb] TRUE

	puts "\tTest098.b: c_get(DB_FIRST) on the secondary."
	error_check_good get_first [$bc get -first] \
	    [list [list [[callback_n 0] aaa data1] data1]]

	puts "\tTest098.c: c_get(DB_GET_RECNO) on the secondary."
	error_check_good get_recno [$bc get -get_recno] 1

	error_check_good c_close [$bc close] 0

	error_check_good bdb_close [$bdb close] 0
	error_check_good adb_close [$adb close] 0
}
