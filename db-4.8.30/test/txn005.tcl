# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	txn005
# TEST	Test transaction ID wraparound and recovery.
proc txn005 {} {
	source ./include.tcl
	global txn_curid
	global txn_maxid

	env_cleanup $testdir
	puts "Txn005: Test transaction wraparound recovery"

	# Open/create the txn region
	puts "\tTxn005.a: Create environment"
	set e [berkdb_env -create -txn -home $testdir]
	error_check_good env_open [is_valid_env $e] TRUE

	set txn1 [$e txn]
	error_check_good txn1 [is_valid_txn $txn1 $e] TRUE

	set db [berkdb_open -env $e -txn $txn1 -create -btree txn005.db]
	error_check_good db [is_valid_db $db] TRUE
	error_check_good txn1_commit [$txn1 commit] 0

	puts "\tTxn005.b: Set txn ids"
	error_check_good txn_id_set \
            [$e txn_id_set [expr $txn_maxid - 1] $txn_maxid] 0

	# txn2 and txn3 will require a wraparound txnid
	set txn2 [$e txn]
	error_check_good txn2 [is_valid_txn $txn2 $e] TRUE

	error_check_good put [$db put -txn $txn2 "a" ""] 0
	error_check_good txn2_commit [$txn2 commit] 0

	error_check_good get_a [$db get "a"] "{a {}}"

	error_check_good close [$db close] 0

	set txn3 [$e txn]
	error_check_good txn3 [is_valid_txn $txn3 $e] TRUE

	set db [berkdb_open -env $e -txn $txn3 -btree txn005.db]
	error_check_good db [is_valid_db $db] TRUE

	error_check_good put2 [$db put -txn $txn3 "b" ""] 0
	error_check_good sync [$db sync] 0
	error_check_good txn3_abort [$txn3 abort] 0
	error_check_good dbclose [$db close] 0
	error_check_good eclose [$e close] 0

	puts "\tTxn005.c: Run recovery"
	set stat [catch {exec $util_path/db_recover -h $testdir -e -c} result]
	if { $stat == 1 } {
		error "FAIL: Recovery error: $result."
	}

	puts "\tTxn005.d: Check data"
	set e [berkdb_env -txn -home $testdir]
	error_check_good env_open [is_valid_env $e] TRUE

	set db [berkdb_open -env $e -auto_commit -btree txn005.db]
	error_check_good db [is_valid_db $db] TRUE

	error_check_good get_a [$db get "a"] "{a {}}"
	error_check_bad get_b [$db get "b"] "{b {}}"
	error_check_good dbclose [$db close] 0
	error_check_good eclose [$e close] 0
}
