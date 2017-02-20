# See the file LICENSE for redistribution information.
#
# Copyright (c) 2003-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	txn011
# TEST	Test durable and non-durable txns.
# TEST	Test a mixed env (with both durable and non-durable
# TEST	dbs), then a purely non-durable env.  Make sure commit
# TEST	and abort work, and that only the log records we
# TEST	expect are written.
# TEST	Test that we can't get a durable handle on an open ND
# TEST	database, or vice versa.  Test that all subdb's
# TEST	must be of the same type (D or ND).
proc txn011 { {ntxns 100} } {
	source ./include.tcl
	global util_path

	foreach envtype { "" "-private" } {
		puts "Txn011: Non-durable txns ($envtype)."
		env_cleanup $testdir

		puts "\tTxn011.a: Persistent env recovery with -log_inmemory"
		set lbuf [expr 8 * [expr 1024 * 1024]]
		set env_cmd "berkdb_env -create \
		    -home $testdir -txn -log_inmemory -log_buffer $lbuf"
		set ndenv [eval $env_cmd $envtype]
		set db [berkdb_open -create -auto_commit \
		    -btree -env $ndenv -notdurable test.db]
		check_log_records $testdir
		error_check_good db_close [$db close] 0
		error_check_good ndenv_close [$ndenv close] 0

		# Run recovery with -e to retain environment.
		set stat [catch {exec $util_path/db_recover -e -h $testdir} ret]
		error_check_good db_printlog $stat 0

		# Rejoin env and make sure that the db is still there.
		set ndenv [berkdb_env -home $testdir]
		set db [berkdb_open -auto_commit -env $ndenv test.db]
		error_check_good db_close [$db close] 0
		error_check_good ndenv_close [$ndenv close] 0
		env_cleanup $testdir

		# Start with a new env for the next test.
		set ndenv [eval $env_cmd]
		error_check_good env_open [is_valid_env $ndenv] TRUE

		# Open/create the database.
		set testfile notdurable.db
		set db [eval berkdb_open -create \
		    -auto_commit -env $ndenv -notdurable -btree $testfile]
		error_check_good dbopen [is_valid_db $db] TRUE

		puts "\tTxn011.b: Abort txns in in-memory logging env."
		txn011_runtxns $ntxns $db $ndenv abort
		# Make sure there is nothing in the db.
		txn011_check_empty $db $ndenv

		puts "\tTxn011.c: Commit txns in in-memory logging env."
		txn011_runtxns $ntxns $db $ndenv commit

		# Make sure we haven't written any inappropriate log records
		check_log_records $testdir

		# Clean up non-durable env tests.
		error_check_good db_close [$db close] 0
		error_check_good ndenv_close [$ndenv close] 0
		env_cleanup $testdir

		puts "\tTxn011.d: Set up mixed durable/non-durable test."
		# Open/create the mixed environment
		set mixed_env_cmd "berkdb_env_noerr -create \
			-home $testdir -txn -log_inmemory -log_buffer $lbuf"
		set env [eval $mixed_env_cmd]
		error_check_good env_open [is_valid_env $env] TRUE
		check_log_records $testdir

		# Open/create the non-durable database
		set nondurfile nondurable.db
		set ndb [berkdb_open_noerr -create\
		    -auto_commit -env $env -btree -notdurable $nondurfile]
		error_check_good dbopen [is_valid_db $ndb] TRUE
		check_log_records $testdir

		puts "\tTxn011.e: Abort txns in non-durable db."
		txn011_runtxns $ntxns $ndb $env abort
		# Make sure there is nothing in the db.
		txn011_check_empty $ndb $env
		check_log_records $testdir

		puts "\tTxn011.f: Commit txns in non-durable db."
		txn011_runtxns $ntxns $ndb $env commit
		check_log_records $testdir

		# Open/create the durable database
		set durfile durable.db
		set ddb [eval berkdb_open_noerr \
		   -create -auto_commit -env $env -btree $durfile]
		error_check_good dbopen [is_valid_db $ddb] TRUE

		# Try to get a not-durable handle on the durable db.
		puts "\tTxn011.g: Try to get a not-durable handle on\
		    an open durable db."
		set errormsg "Cannot open DURABLE and NOT DURABLE handles"
		catch {berkdb_open_noerr \
		    -auto_commit -env $env -notdurable $durfile} res
		error_check_good handle_error1 [is_substr $res $errormsg] 1
		error_check_good ddb_close [$ddb close] 0

		# Try to get a not-durable handle when reopening the durable 
		# db (this should work).
		set db [berkdb_open_noerr \
		    -auto_commit -env $env -notdurable $durfile]  
		error_check_good db_reopen [is_valid_db $db] TRUE
		error_check_good db_close [$db close] 0

		# Now reopen as durable for the remainder of the test.
		set ddb [berkdb_open_noerr \
		    -auto_commit -env $env -btree $durfile]
		error_check_good dbopen [is_valid_db $ddb] TRUE

		puts "\tTxn011.h: Abort txns in durable db."
		# Add items to db in several txns but abort every one.
		txn011_runtxns $ntxns $ddb $env abort
		# Make sure there is nothing in the db.
		txn011_check_empty $ddb $env

		puts "\tTxn011.i: Commit txns in durable db."
		txn011_runtxns $ntxns $ddb $env commit

		puts "\tTxn011.j: Subdbs must all be durable or all not durable."
		# Ask for -notdurable on durable db/subdb
		set sdb1 [eval berkdb_open_noerr -create -auto_commit \
		    -env $env -btree testfile1.db subdb1]
		catch {set sdb2 [eval berkdb_open_noerr -create -auto_commit \
		    -env $env -btree -notdurable testfile1.db subdb2]} res
		error_check_good same_type_subdb1 [is_substr $res $errormsg] 1
		error_check_good sdb1_close [$sdb1 close] 0

		# Ask for durable on notdurable db/subdb
		set sdb3 [eval berkdb_open_noerr -create -auto_commit \
		    -env $env -btree -notdurable testfile2.db subdb3]
		catch {set sdb4 [eval berkdb_open_noerr -create -auto_commit \
		    -env $env -btree testfile2.db subdb4]} res
		error_check_good same_type_subdb2 [is_substr $res $errormsg] 1
		error_check_good sdb3_close [$sdb3 close] 0

		puts "\tTxn011.k: Try to get a durable handle on a\
		    not-durable db."
		# Try to get a durable handle on a not-durable database,
		# while open.  This should fail, but getting a durable handle
		# when re-opening should work. 
		catch {berkdb_open_noerr -auto_commit -env $env $nondurfile} res
		error_check_good handle_error [is_substr $res $errormsg] 1
		error_check_good ndb_close [$ndb close] 0

		set ndb [berkdb_open_noerr -auto_commit -env $env $nondurfile]
		error_check_good ndb_reopen [is_valid_db $ndb] TRUE
		error_check_good ndb_close [$ndb close] 0

		# Clean up mixed env.
		error_check_good ddb_close [$ddb close] 0
		error_check_good env_close [$env close] 0
	}
}

proc txn011_runtxns { ntxns db env end } {
	source ./include.tcl

	set did [open $dict]
	set i 0
	while { [gets $did str] != -1 && $i < $ntxns } {
		set txn [$env txn]
		error_check_good txn_begin [is_valid_txn $txn $env] TRUE

		error_check_good db_put_txn [$db put -txn $txn $i $str] 0
		error_check_good txn_$end [$txn $end] 0
		incr i
	}
	close $did
}

# Verify that a database is empty
proc txn011_check_empty { db env } {
	# Start a transaction
	set t [$env txn]
	error_check_good txn [is_valid_txn $t $env] TRUE
	set txn "-txn $t"

	# If a cursor get -first returns nothing, the db is empty.
	set dbc [eval {$db cursor} $txn]
	error_check_good db_cursor [is_substr $dbc $db] 1
	set ret [$dbc get -first]
	error_check_good get_on_empty [string length $ret] 0
	error_check_good dbc_close [$dbc close] 0

	# End transaction
	error_check_good txn [$t commit] 0
}

# Some log records are still produced when we run create in a
# non-durable db in a regular env.  Just make sure we don't see
# any unexpected types.
proc check_log_records { dir } {
	global util_path

	set tmpfile $dir/printlog.out
	set stat [catch {exec $util_path/db_printlog -h $dir > $tmpfile} ret]
	error_check_good db_printlog $stat 0

	set f [open $tmpfile r]
	while { [gets $f record] >= 0 } {
		set r [regexp {\[[^\]]*\]\[[^\]]*\]([^\:]*)\:} $record whl name]
		if { $r == 1 && [string match *_debug $name] != 1 && \
		    [string match __txn_regop $name] != 1 && \
		    [string match __txn_child $name] != 1 } {
			puts "FAIL: unexpected log record $name found"
		}
	}
	close $f
	fileremove $tmpfile
}
