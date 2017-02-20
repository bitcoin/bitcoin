# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test063
# TEST	Test of the DB_RDONLY flag to DB->open
# TEST	Attempt to both DB->put and DBC->c_put into a database
# TEST	that has been opened DB_RDONLY, and check for failure.
proc test063 { method args } {
	global errorCode
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]
	set tnum "063"

	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
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

	set key "key"
	set data "data"
	set key2 "another_key"
	set data2 "more_data"

	set gflags ""
	set txn ""

	if { [is_record_based $method] == 1 } {
	    set key "1"
	    set key2 "2"
	    append gflags " -recno"
	}

	puts "Test$tnum: $method ($args) DB_RDONLY test."

	# Create a test database.
	puts "\tTest$tnum.a: Creating test database."
	set db [eval {berkdb_open_noerr -create -mode 0644} \
	    $omethod $args $testfile]
	error_check_good db_create [is_valid_db $db] TRUE

	# Put and get an item so it's nonempty.
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set ret [eval {$db put} $txn {$key [chop_data $method $data]}]
	error_check_good initial_put $ret 0

	set dbt [eval {$db get} $txn $gflags {$key}]
	error_check_good initial_get $dbt \
	    [list [list $key [pad_data $method $data]]]

	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0

	if { $eindex == -1 } {
		# Confirm that database is writable.  If we are
		# using an env (that may be remote on a server)
		# we cannot do this check.
		error_check_good writable [file writable $testfile] 1
	}

	puts "\tTest$tnum.b: Re-opening DB_RDONLY and attempting to put."

	# Now open it read-only and make sure we can get but not put.
	set db [eval {berkdb_open_noerr -rdonly} $args {$testfile}]
	error_check_good db_open [is_valid_db $db] TRUE

	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbt [eval {$db get} $txn $gflags {$key}]
	error_check_good db_get $dbt \
	    [list [list $key [pad_data $method $data]]]

	set ret [catch {eval {$db put} $txn \
	    {$key2 [chop_data $method $data]}} res]
	error_check_good put_failed $ret 1
	error_check_good db_put_rdonly [is_substr $errorCode "EACCES"] 1
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}

	set errorCode "NONE"

	puts "\tTest$tnum.c: Attempting cursor put."

	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good cursor_create [is_valid_cursor $dbc $db] TRUE

	error_check_good cursor_set [$dbc get -first] $dbt
	set ret [catch {eval {$dbc put} -current $data} res]
	error_check_good c_put_failed $ret 1
	error_check_good dbc_put_rdonly [is_substr $errorCode "EACCES"] 1

	set dbt [eval {$db get} $gflags {$key2}]
	error_check_good db_get_key2 $dbt ""

	puts "\tTest$tnum.d: Attempting ordinary delete."

	set errorCode "NONE"
	set ret [catch {eval {$db del} $txn {$key}} 1]
	error_check_good del_failed $ret 1
	error_check_good db_del_rdonly [is_substr $errorCode "EACCES"] 1

	set dbt [eval {$db get} $txn $gflags {$key}]
	error_check_good db_get_key $dbt \
	    [list [list $key [pad_data $method $data]]]

	puts "\tTest$tnum.e: Attempting cursor delete."
	# Just set the cursor to the beginning;  we don't care what's there...
	# yet.
	set dbt2 [$dbc get -first]
	error_check_good db_get_first_key $dbt2 $dbt
	set errorCode "NONE"
	set ret [catch {$dbc del} res]
	error_check_good c_del_failed $ret 1
	error_check_good dbc_del_rdonly [is_substr $errorCode "EACCES"] 1

	set dbt2 [$dbc get -current]
	error_check_good db_get_key $dbt2 $dbt

	puts "\tTest$tnum.f: Close, reopen db;  verify unchanged."

	error_check_good dbc_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0

	set db [eval {berkdb_open} $omethod $args $testfile]
	error_check_good db_reopen [is_valid_db $db] TRUE

	set dbc [$db cursor]
	error_check_good cursor_create [is_valid_cursor $dbc $db] TRUE

	error_check_good first_there [$dbc get -first] \
	    [list [list $key [pad_data $method $data]]]
	error_check_good nomore_there [$dbc get -next] ""

	error_check_good dbc_close [$dbc close] 0
	error_check_good db_close [$db close] 0
}
