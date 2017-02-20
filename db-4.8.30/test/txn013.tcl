# See the file LICENSE for redistribution information.
#
# Copyright (c) 2005-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	txn013
# TEST	Test of txns used in the wrong environment.
# TEST	Set up two envs.  Start a txn in one env, and attempt to use it
# TEST 	in the other env.  Verify we get the appropriate error message.
proc txn013 { } {
	source ./include.tcl

	set tnum "013"
	puts "Txn$tnum: Test use of txns in wrong environment."
	set testfile FILE.db
	set key KEY
	set data DATA

	env_cleanup $testdir

	puts "\tTxn$tnum.a: Create two environments."
	set env1 [berkdb_env_noerr -create -mode 0644 -txn -home $testdir]
	file mkdir $testdir/SUBDIR
	set env2 \
	    [berkdb_env_noerr -create -mode 0644 -txn -home $testdir/SUBDIR]
	error_check_good env1 [is_valid_env $env1] TRUE
	error_check_good env2 [is_valid_env $env2] TRUE

	# Open a database in each environment.
	puts "\tTxn$tnum.b: Open a database in each environment."
	set db1 [berkdb_open_noerr \
	    -env $env1 -create -auto_commit -btree $testfile]
	set db2 [berkdb_open_noerr \
	    -env $env2 -create -auto_commit -btree $testfile]

	# Create txns in both environments.
	puts "\tTxn$tnum.c: Start a transaction in each environment."
	set txn1 [$env1 txn]
	set txn2 [$env2 txn]
	error_check_good txn1_begin [is_valid_txn $txn1 $env1] TRUE
	error_check_good txn2_begin [is_valid_txn $txn2 $env2] TRUE

	# First do the puts in the correct envs, so we have something
	# for the gets and deletes.
	error_check_good txn1_env1 [$db1 put -txn $txn1 $key $data] 0
	error_check_good txn2_env2 [$db2 put -txn $txn2 $key $data] 0

	puts "\tTxn$tnum.d: Execute db put in wrong environment."
	set errormsg "from different environments"
	catch {$db1 put -txn $txn2 $key $data} res
	error_check_good put_env1txn2 [is_substr $res $errormsg] 1
	catch {$db2 put -txn $txn1 $key $data} res
	error_check_good put_env2txn1 [is_substr $res $errormsg] 1

	puts "\tTxn$tnum.e: Execute db get in wrong environment."
	catch {$db1 get -txn $txn2 $key} res
	error_check_good get_env1txn2 [is_substr $res $errormsg] 1
	catch {$db2 get -txn $txn1 $key} res
	error_check_good get_env2txn1 [is_substr $res $errormsg] 1

	puts "\tTxn$tnum.f: Execute db del in wrong environment."
	catch {$db1 del -txn $txn2 $key} res
	error_check_good get_env1txn2 [is_substr $res $errormsg] 1
	catch {$db2 del -txn $txn1 $key} res
	error_check_good get_env2txn1 [is_substr $res $errormsg] 1

	# Clean up.
	error_check_good txn1_commit [$txn1 commit] 0
	error_check_good txn2_commit [$txn2 commit] 0
	error_check_good db1_close [$db1 close] 0
	error_check_good db2_close [$db2 close] 0
	error_check_good env1_close [$env1 close] 0
	error_check_good env2_close [$env2 close] 0
}

