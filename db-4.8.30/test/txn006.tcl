# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
#TEST	txn006
#TEST	Test dump/load in transactional environment.
proc txn006 { { iter 50 } } {
	source ./include.tcl
	set testfile txn006.db

	puts "Txn006: Test dump/load in transaction environment"
	env_cleanup $testdir

	puts "\tTxn006.a: Create environment and database"
	# Open/create the txn region
	set e [berkdb_env -create -home $testdir -txn]
	error_check_good env_open [is_valid_env $e] TRUE

	# Open/create database
	set db [berkdb_open -auto_commit -env $e \
	    -create -btree -dup $testfile]
	error_check_good db_open [is_valid_db $db] TRUE

	# Start a transaction
	set txn [$e txn]
	error_check_good txn [is_valid_txn $txn $e] TRUE

	puts "\tTxn006.b: Put data"
	# Put some data
	for { set i 1 } { $i < $iter } { incr i } {
		error_check_good put [$db put -txn $txn key$i data$i] 0
	}

	# End transaction, close db
	error_check_good txn_commit [$txn commit] 0
	error_check_good db_close [$db close] 0
	error_check_good env_close [$e close] 0

	puts "\tTxn006.c: dump/load"
	# Dump and load
	exec $util_path/db_dump -p -h $testdir $testfile | \
	    $util_path/db_load -h $testdir $testfile
}
