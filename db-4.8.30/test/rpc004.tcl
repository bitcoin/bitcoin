# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rpc004
# TEST	Test RPC server and security
proc rpc004 { } {
	global __debug_on
	global __debug_print
	global errorInfo
	global passwd
	global has_crypto
	global rpc_svc
	source ./include.tcl

	puts "Rpc004: RPC server + security"
	puts "Rpc004: Using $rpc_svc"
	# Skip test if release does not support encryption.
	if { $has_crypto == 0 } {
		puts "Skipping test rpc004 for non-crypto release."
		return
	}

	cleanup $testdir NULL
	set dpid [rpc_server_start 1]
	puts "\tRpc004.a: Started server, pid $dpid"

	#
	# Wrap the test in a catch statement so we can still kill
	# the rpc server even if the test fails.
	#
	set status [catch {
		tclsleep 2
		remote_cleanup $rpc_server $rpc_testdir $testdir

		puts "\tRpc004.b: Creating environment"

		set testfile "rpc004.db"
		set testfile1 "rpc004a.db"
		set home [file tail $rpc_testdir]

		set env [eval {berkdb_env -create -mode 0644 -home $home \
		    -server $rpc_server -encryptaes $passwd -txn}]
		error_check_good lock_env:open [is_valid_env $env] TRUE

		puts "\tRpc004.c: Opening a non-encrypted database"
		#
		# NOTE: the type of database doesn't matter, just use btree.
		set db [eval {berkdb_open -auto_commit -create -btree \
		    -mode 0644} -env $env $testfile]
		error_check_good dbopen [is_valid_db $db] TRUE

		puts "\tRpc004.d: Opening an encrypted database"
		set db1 [eval {berkdb_open -auto_commit -create -btree \
		    -mode 0644} -env $env -encrypt $testfile1]
		error_check_good dbopen [is_valid_db $db1] TRUE

		set txn [$env txn]
		error_check_good txn [is_valid_txn $txn $env] TRUE
		puts "\tRpc004.e: Put/get on both databases"
		set key "key"
		set data "data"

		set ret [$db put -txn $txn $key $data]
		error_check_good db_put $ret 0
		set ret [$db get -txn $txn $key]
		error_check_good db_get $ret [list [list $key $data]]
		set ret [$db1 put -txn $txn $key $data]
		error_check_good db1_put $ret 0
		set ret [$db1 get -txn $txn $key]
		error_check_good db1_get $ret [list [list $key $data]]

		error_check_good txn_commit [$txn commit] 0
		error_check_good db_close [$db close] 0
		error_check_good db1_close [$db1 close] 0
		error_check_good env_close [$env close] 0

		# Cleanup our environment because it's encrypted
		remote_cleanup $rpc_server $rpc_testdir $testdir
	} res]
	if { $status != 0 } {
		puts $res
	}
	tclkill $dpid
}
