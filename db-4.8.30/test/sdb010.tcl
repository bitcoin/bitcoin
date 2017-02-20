# See the file LICENSE for redistribution information.
#
# Copyright (c) 2000-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	sdb010
# TEST	Test DB->remove() method and DB->truncate() for subdbs
proc sdb010 { method args } {
	global errorCode
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	puts "Subdb010: Test of DB->remove() and DB->truncate"

	if { [is_queue $method] == 1 } {
		puts "\tSubdb010: Skipping for method $method."
		return
	}

	set txnenv 0
	set envargs ""
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are not given an env, create one.
	if { $eindex == -1 } {
		set env [berkdb_env -create -home $testdir -mode 0644]
		error_check_good env_open [is_valid_env $env] TRUE
	} else {
		incr eindex
		set env [lindex $args $eindex]
	}
	set testfile subdb010.db
	set envargs " -env $env "
	set txnenv [is_txnenv $env]
	if { $txnenv == 1 } {
		append args " -auto_commit "
		append envargs " -auto_commit "
	}
	set testdir [get_home $env]
	set tfpath $testdir/$testfile

	cleanup $testdir $env

	set txn ""
	set testdb DATABASE
	set testdb2 DATABASE2

	set db [eval {berkdb_open -create -mode 0644} $omethod \
	    $args $envargs $testfile $testdb]
	error_check_good db_open [is_valid_db $db] TRUE
	error_check_good db_close [$db close] 0

	puts "\tSubdb010.a: Test of DB->remove()"
	error_check_good file_exists_before [file exists $tfpath] 1
	error_check_good db_remove [eval {berkdb dbremove} $envargs \
	    $testfile $testdb] 0

	# File should still exist.
	error_check_good file_exists_after [file exists $tfpath] 1

	# But database should not.
	set ret [catch {eval berkdb_open $omethod \
	    $args $envargs $testfile $testdb} res]
	error_check_bad open_failed ret 0
	error_check_good open_failed_ret [is_substr $errorCode ENOENT] 1

	puts "\tSubdb010.b: Setup for DB->truncate()"
	# The nature of the key and data are unimportant; use numeric key
	# so record-based methods don't need special treatment.
	set key1 1
	set key2 2
	set data1 [pad_data $method data1]
	set data2 [pad_data $method data2]

	set db [eval {berkdb_open -create -mode 0644} $omethod \
	    $args $envargs {$testfile $testdb}]
	error_check_good db_open [is_valid_db $db] TRUE
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	error_check_good dbput [eval {$db put} $txn {$key1 $data1}] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}

	set db2 [eval {berkdb_open -create -mode 0644} $omethod \
	    $args $envargs $testfile $testdb2]
	error_check_good db_open [is_valid_db $db2] TRUE
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	error_check_good dbput [eval {$db2 put} $txn {$key2 $data2}] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}

	error_check_good db_close [$db close] 0
	error_check_good db_close [$db2 close] 0

	puts "\tSubdb010.c: truncate"
	#
	# Return value should be 1, the count of how many items were
	# destroyed when we truncated.
	set db [eval {berkdb_open -create -mode 0644} $omethod \
	    $args $envargs $testfile $testdb]
	error_check_good db_open [is_valid_db $db] TRUE
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	error_check_good trunc_subdb [eval {$db truncate} $txn] 1
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0

	puts "\tSubdb010.d: check"
	set db [eval {berkdb_open} $args $envargs {$testfile $testdb}]
	error_check_good db_open [is_valid_db $db] TRUE
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good db_cursor [is_valid_cursor $dbc $db] TRUE
	set kd [$dbc get -first]
	error_check_good trunc_dbcget [llength $kd] 0
	error_check_good dbcclose [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}

	set db2 [eval {berkdb_open} $args $envargs {$testfile $testdb2}]
	error_check_good db_open [is_valid_db $db2] TRUE
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db2 cursor} $txn]
	error_check_good db_cursor [is_valid_cursor $dbc $db2] TRUE
	set kd [$dbc get -first]
	error_check_bad notrunc_dbcget1 [llength $kd] 0
	set db2kd [list [list $key2 $data2]]
	error_check_good key2 $kd $db2kd
	set kd [$dbc get -next]
	error_check_good notrunc_dbget2 [llength $kd] 0
	error_check_good dbcclose [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}

	error_check_good db_close [$db close] 0
	error_check_good db_close [$db2 close] 0

	# If we created our env, close it.
	if { $eindex == -1 } {
		error_check_good env_close [$env close] 0
	}
}
