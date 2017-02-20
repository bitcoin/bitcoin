# See the file LICENSE for redistribution information.
#
# Copyright (c) 2005-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	txn012
# TEST	Test txn->getname and txn->setname.

proc txn012 { {ntxns 100} } {
	source ./include.tcl
	global util_path

	puts "Txn012: Test txn->setname and txn->getname."
	env_cleanup $testdir
	set txnname "this is a short txn name"
	set longtxnname "transaction names longer than 50 characters will be truncated"

	puts "\tTxn012.a: Set up env and txn."
	set env [berkdb_env -create -home $testdir -txn]
	set db [berkdb_open -create -auto_commit -btree -env $env test.db]
	set txn0 [$env txn]
	set txn1 [$env txn]

	# Name the transactions, check the name.
	error_check_good name_txn0 [$txn0 setname $txnname] 0
	set getname [$txn0 getname]
	error_check_good txnname $getname $txnname

	error_check_good longname_txn [$txn1 setname $longtxnname] 0
	set getlongname [$txn1 getname]
	error_check_good longtxnname $getlongname $longtxnname

	# Run db_stat.  The long txn name will be truncated.
	set stat [exec $util_path/db_stat -h $testdir -t]
	error_check_good stat_name [is_substr $stat $txnname] 1
	error_check_good stat_longname [is_substr $stat $longtxnname] 0
	set truncname [string range $longtxnname 0 49]
	error_check_good stat_truncname [is_substr $stat $truncname] 1

	# Start another process and make sure it can see the names too.
	puts "\tTxn012.b: Fork child process."
	set pid [exec $tclsh_path $test_path/wrap.tcl txn012script.tcl \
	    $testdir/txn012script.log $testdir $txnname $longtxnname &]

	watch_procs $pid 1

	error_check_good txn0_commit [$txn0 commit] 0
	error_check_good txn1_commit [$txn1 commit] 0

	# Check for errors in child log file.
	set errstrings [eval findfail $testdir/txn012script.log]
	foreach str $errstrings {
		puts "FAIL: error message in log file: $str"
	}

	# Clean up.
	error_check_good db_close [$db close] 0
	error_check_good env_close [$env close] 0
}

