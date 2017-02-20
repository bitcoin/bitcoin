# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
#TEST	txn007
#TEST	Test of DB_TXN_WRITE_NOSYNC
proc txn007 { { iter 50 } } {
	source ./include.tcl
	set testfile txn007.db

	puts "Txn007: DB_TXN_WRITE_NOSYNC"
	env_cleanup $testdir

	# Open/create the txn region
	puts "\tTxn007.a: Create env and database with -wrnosync"
	set e [berkdb_env -create -home $testdir -txn -wrnosync]
	error_check_good env_open [is_valid_env $e] TRUE

	# Open/create database
	set db [berkdb open -auto_commit -env $e \
	    -create -btree -dup $testfile]
	error_check_good db_open [is_valid_db $db] TRUE

	# Put some data
	puts "\tTxn007.b: Put $iter data items in individual transactions"
	for { set i 1 } { $i < $iter } { incr i } {
		# Start a transaction
		set txn [$e txn]
		error_check_good txn [is_valid_txn $txn $e] TRUE
		$db put -txn $txn key$i data$i
		error_check_good txn_commit [$txn commit] 0
	}
	set stat [$e log_stat]
	puts "\tTxn007.c: Check log stats"
	foreach i $stat {
		set txt [lindex $i 0]
		if { [string equal $txt {Times log written}] == 1 } {
			set wrval [lindex $i 1]
		}
		if { [string equal $txt {Times log flushed to disk}] == 1 } {
			set syncval [lindex $i 1]
		}
	}
	error_check_good wrval [expr $wrval >= $iter] 1
	#
	# We should have written at least 'iter' number of times,
	# but not synced on any of those.
	#
	set val [expr $wrval - $iter]
	error_check_good syncval [expr $syncval <= $val] 1

	error_check_good db_close [$db close] 0
	error_check_good env_close [$e close] 0
}
