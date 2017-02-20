# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	txn004
# TEST	Test of wraparound txnids (txn001)
proc txn004 { } {
	source ./include.tcl
	global txn_curid
	global txn_maxid

	set orig_curid $txn_curid
	set orig_maxid $txn_maxid
	puts "\tTxn004.1: wraparound txnids"
	set txn_curid [expr $txn_maxid - 2]
	txn001 "004.1"
	puts "\tTxn004.2: closer wraparound txnids"
	set txn_curid [expr $txn_maxid - 3]
	set txn_maxid [expr $txn_maxid - 2]
	txn001 "004.2"

	puts "\tTxn004.3: test wraparound txnids"
	txn_idwrap_check $testdir
	set txn_curid $orig_curid
	set txn_maxid $orig_maxid
	return
}

proc txn_idwrap_check { testdir } {
	global txn_curid
	global txn_maxid

	env_cleanup $testdir

	# Open/create the txn region
	set e [berkdb_env -create -txn -home $testdir]
	error_check_good env_open [is_substr $e env] 1

	set txn1 [$e txn]
	error_check_good txn1 [is_valid_txn $txn1 $e] TRUE
	error_check_good txn_id_set \
	    [$e txn_id_set [expr $txn_maxid - 1] $txn_maxid] 0

	set txn2 [$e txn]
	error_check_good txn2 [is_valid_txn $txn2 $e] TRUE

	# txn3 will require a wraparound txnid
	# XXX How can we test it has a wrapped id?
	set txn3 [$e txn]
	error_check_good wrap_txn3 [is_valid_txn $txn3 $e] TRUE

	error_check_good free_txn1 [$txn1 commit] 0
	error_check_good free_txn2 [$txn2 commit] 0
	error_check_good free_txn3 [$txn3 commit] 0

	error_check_good close [$e close] 0
}

