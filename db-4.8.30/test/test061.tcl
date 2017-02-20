# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test061
# TEST	Test of txn abort and commit for in-memory databases.
# TEST	a) Put + abort: verify absence of data
# TEST	b) Put + commit: verify presence of data
# TEST	c) Overwrite + abort: verify that data is unchanged
# TEST	d) Overwrite + commit: verify that data has changed
# TEST	e) Delete + abort: verify that data is still present
# TEST	f) Delete + commit: verify that data has been deleted
proc test061 { method args } {
	global alphabet
	global encrypt
	global errorCode
	global passwd
	source ./include.tcl

	#
	# If we are using an env, then skip this test.  It needs its own.
	set eindex [lsearch -exact $args "-env"]
	if { $eindex != -1 } {
		incr eindex
		set env [lindex $args $eindex]
		puts "Test061 skipping for env $env"
		return
	}
	if { [is_partitioned $args] == 1 } {
		puts "Test061 skipping for partitioned $method"
		return
	}
	set args [convert_args $method $args]
	set omethod [convert_method $method]
	if { [is_queueext $method] == 1} {
		puts "Test061 skipping for method $method"
		return
	}
	puts "Test061: Transaction abort and commit test for in-memory data."
	puts "Test061: $method $args"

	set encargs ""
	set args [split_encargs $args encargs]
	set pageargs ""
	split_pageargs $args pageargs

	set key "key"
	set data "data"
	set otherdata "otherdata"
	set txn ""
	set flags ""
	set gflags ""

	if { [is_record_based $method] == 1} {
		set key 1
		set gflags " -recno"
	}

	puts "\tTest061: Create environment and $method database."
	env_cleanup $testdir

	# create environment
	set eflags "-create -txn $encargs -home $testdir"
	set dbenv [eval {berkdb_env} $eflags $pageargs ]
	error_check_good dbenv [is_valid_env $dbenv] TRUE

	# db open -- no file specified, in-memory database
	set flags "-auto_commit -create $args $omethod"
	set db [eval {berkdb_open -env} $dbenv $flags]
	error_check_good dbopen [is_valid_db $db] TRUE

	# Here we go with the six test cases.  Since we need to verify
	# a different thing each time, and since we can't just reuse
	# the same data if we're to test overwrite, we just
	# plow through rather than writing some impenetrable loop code;
	# each of the cases is only a few lines long, anyway.

	puts "\tTest061.a: put/abort"

	# txn_begin
	set txn [$dbenv txn]
	error_check_good txn_begin [is_valid_txn $txn $dbenv] TRUE

	# put a key
	set ret [eval {$db put} -txn $txn {$key [chop_data $method $data]}]
	error_check_good db_put $ret 0

	# check for existence
	set ret [eval {$db get} -txn $txn $gflags {$key}]
	error_check_good get $ret [list [list $key [pad_data $method $data]]]

	# abort
	error_check_good txn_abort [$txn abort] 0

	# check for *non*-existence
	set ret [eval {$db get} $gflags {$key}]
	error_check_good get $ret {}

	puts "\tTest061.b: put/commit"

	# txn_begin
	set txn [$dbenv txn]
	error_check_good txn_begin [is_valid_txn $txn $dbenv] TRUE

	# put a key
	set ret [eval {$db put} -txn $txn {$key [chop_data $method $data]}]
	error_check_good db_put $ret 0

	# check for existence
	set ret [eval {$db get} -txn $txn $gflags {$key}]
	error_check_good get $ret [list [list $key [pad_data $method $data]]]

	# commit
	error_check_good txn_commit [$txn commit] 0

	# check again for existence
	set ret [eval {$db get} $gflags {$key}]
	error_check_good get $ret [list [list $key [pad_data $method $data]]]

	puts "\tTest061.c: overwrite/abort"

	# txn_begin
	set txn [$dbenv txn]
	error_check_good txn_begin [is_valid_txn $txn $dbenv] TRUE

	# overwrite {key,data} with {key,otherdata}
	set ret [eval {$db put} -txn $txn {$key [chop_data $method $otherdata]}]
	error_check_good db_put $ret 0

	# check for existence
	set ret [eval {$db get} -txn $txn $gflags {$key}]
	error_check_good get $ret \
	    [list [list $key [pad_data $method $otherdata]]]

	# abort
	error_check_good txn_abort [$txn abort] 0

	# check that data is unchanged ($data not $otherdata)
	set ret [eval {$db get} $gflags {$key}]
	error_check_good get $ret [list [list $key [pad_data $method $data]]]

	puts "\tTest061.d: overwrite/commit"

	# txn_begin
	set txn [$dbenv txn]
	error_check_good txn_begin [is_valid_txn $txn $dbenv] TRUE

	# overwrite {key,data} with {key,otherdata}
	set ret [eval {$db put} -txn $txn {$key [chop_data $method $otherdata]}]
	error_check_good db_put $ret 0

	# check for existence
	set ret [eval {$db get} -txn $txn $gflags {$key}]
	error_check_good get $ret \
	    [list [list $key [pad_data $method $otherdata]]]

	# commit
	error_check_good txn_commit [$txn commit] 0

	# check that data has changed ($otherdata not $data)
	set ret [eval {$db get} $gflags {$key}]
	error_check_good get $ret \
	    [list [list $key [pad_data $method $otherdata]]]

	puts "\tTest061.e: delete/abort"

	# txn_begin
	set txn [$dbenv txn]
	error_check_good txn_begin [is_valid_txn $txn $dbenv] TRUE

	# delete
	set ret [eval {$db del} -txn $txn {$key}]
	error_check_good db_put $ret 0

	# check for nonexistence
	set ret [eval {$db get} -txn $txn $gflags {$key}]
	error_check_good get $ret {}

	# abort
	error_check_good txn_abort [$txn abort] 0

	# check for existence
	set ret [eval {$db get} $gflags {$key}]
	error_check_good get $ret \
	    [list [list $key [pad_data $method $otherdata]]]

	puts "\tTest061.f: delete/commit"

	# txn_begin
	set txn [$dbenv txn]
	error_check_good txn_begin [is_valid_txn $txn $dbenv] TRUE

	# put a key
	set ret [eval {$db del} -txn $txn {$key}]
	error_check_good db_put $ret 0

	# check for nonexistence
	set ret [eval {$db get} -txn $txn $gflags {$key}]
	error_check_good get $ret {}

	# commit
	error_check_good txn_commit [$txn commit] 0

	# check for continued nonexistence
	set ret [eval {$db get} $gflags {$key}]
	error_check_good get $ret {}

	# We're done; clean up.
	error_check_good db_close [eval {$db close}] 0
	error_check_good env_close [eval {$dbenv close}] 0

	# Now run db_recover and ensure that it runs cleanly.
	set utilflag ""
	if { $encrypt != 0 } {
		set utilflag "-P $passwd"
	}
	puts "\tTest061.g: Running db_recover -h"
	set ret [catch {eval {exec} $util_path/db_recover -h $testdir \
	    $utilflag} res]
	if { $ret != 0 } {
		puts "FAIL: db_recover outputted $res"
	}
	error_check_good db_recover $ret 0

	puts "\tTest061.h: Running db_recover -c -h"
	set ret [catch {eval {exec} $util_path/db_recover -c -h $testdir \
	    $utilflag} res]
	error_check_good db_recover-c $ret 0
}
