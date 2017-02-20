# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rpc006
# TEST	Test RPC server and multiple operations to server.
# TEST  Make sure the server doesn't deadlock itself, but
# TEST  returns DEADLOCK to the client.
proc rpc006 { } {
	global __debug_on
	global __debug_print
	global errorInfo
	global rpc_svc
	source ./include.tcl

	puts "Rpc006: RPC server + multiple operations"
	puts "Rpc006: Using $rpc_svc"
	cleanup $testdir NULL
	set dpid [rpc_server_start]
	puts "\tRpc006.a: Started server, pid $dpid"

	#
	# Wrap the test in a catch statement so we can still kill
	# the rpc server even if the test fails.
	#
	set status [catch {
		tclsleep 2
		remote_cleanup $rpc_server $rpc_testdir $testdir

		puts "\tRpc006.b: Creating environment"

		set testfile "rpc006.db"
		set home [file tail $rpc_testdir]

		set env [eval {berkdb_env -create -mode 0644 -home $home \
		    -server $rpc_server -txn}]
		error_check_good lock_env:open [is_valid_env $env] TRUE

		#
		# NOTE: the type of database doesn't matter, just use btree.
		set db [eval {berkdb_open -auto_commit -create -btree \
		    -mode 0644} -env $env $testfile]
		error_check_good dbopen [is_valid_db $db] TRUE

		puts "\tRpc006.c: Create competing transactions"
		set txn1 [$env txn]
		set txn2 [$env txn]
		error_check_good txn [is_valid_txn $txn1 $env] TRUE
		error_check_good txn [is_valid_txn $txn2 $env] TRUE
		set key1 "key1"
		set key2 "key2"
		set data1 "data1"
		set data2 "data2"

		puts "\tRpc006.d: Put with both txns to same page. Deadlock"
		set ret [$db put -txn $txn1 $key1 $data1]
		error_check_good db_put $ret 0
		set res [catch {$db put -txn $txn2 $key2 $data2} ret]
		error_check_good db_put2 $res 1
		error_check_good db_putret [is_substr $ret DB_LOCK_DEADLOCK] 1
		error_check_good txn_commit [$txn1 commit] 0

		puts "\tRpc006.e: Retry after commit."
		set res [catch {$db put -txn $txn2 $key2 $data2} ret]
		error_check_good db_put2 $res 0
		error_check_good db_putret $ret 0
		error_check_good txn_commit [$txn2 commit] 0
		error_check_good db_close [$db close] 0
		error_check_good env_close [$env close] 0
	} res]
	if { $status != 0 } {
		puts $res
	}
	tclkill $dpid
}
