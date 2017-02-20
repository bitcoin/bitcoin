# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rpc005
# TEST	Test RPC server handle ID sharing
proc rpc005 { } {
	global __debug_on
	global __debug_print
	global errorInfo
	global rpc_svc
	global is_hp_test
	source ./include.tcl

	puts "Rpc005: RPC server handle sharing"
	puts "Rpc005: Using $rpc_svc"
	set dpid [rpc_server_start]
	puts "\tRpc005.a: Started server, pid $dpid"

	#
	# Wrap the test in a catch statement so we can still kill
	# the rpc server even if the test fails.
	#
	set status [catch {
		tclsleep 2
		remote_cleanup $rpc_server $rpc_testdir $testdir
		puts "\tRpc005.b: Creating environment"

		set testfile "rpc005.db"
		set testfile1 "rpc005a.db"
		set subdb1 "subdb1"
		set subdb2 "subdb2"
		set home [file tail $rpc_testdir]

		set env [eval {berkdb_env -create -mode 0644 -home $home \
		    -server $rpc_server -txn}]
		error_check_good lock_env:open [is_valid_env $env] TRUE

		# You can't open two handles on the same env in
		# HP-UX, so skip this piece.
		if { $is_hp_test == 1 } {
			puts "\tRpc005.c: Skipping for HP-UX."
		} else {
			puts "\tRpc005.c: Compare identical and different \
			    configured envs"
			set env_ident [eval {berkdb_env -home $home \
			    -server $rpc_server -txn}]
			error_check_good \
			    lock_env:open [is_valid_env $env_ident] TRUE

			set env_diff [eval {berkdb_env -home $home \
			    -server $rpc_server -txn nosync}]
			error_check_good \
			    lock_env:open [is_valid_env $env_diff] TRUE

			error_check_good \
			    ident:id [$env rpcid] [$env_ident rpcid]
			error_check_bad \
			    diff:id [$env rpcid] [$env_diff rpcid]

			error_check_good envclose [$env_diff close] 0
			error_check_good envclose [$env_ident close] 0
		}

		puts "\tRpc005.d: Opening a database"
		set db [eval {berkdb_open -auto_commit -create -btree \
		    -mode 0644} -env $env $testfile]
		error_check_good dbopen [is_valid_db $db] TRUE

		puts "\tRpc005.e: Compare identical and different \
		    configured dbs"
		set db_ident [eval {berkdb_open -btree} -env $env $testfile]
		error_check_good dbopen [is_valid_db $db_ident] TRUE

		set db_diff [eval {berkdb_open -btree} -env $env -rdonly \
		    $testfile]
		error_check_good dbopen [is_valid_db $db_diff] TRUE

		set db_diff2 [eval {berkdb_open -btree} -env $env -rdonly \
		    $testfile]
		error_check_good dbopen [is_valid_db $db_diff2] TRUE

		error_check_good ident:id [$db rpcid] [$db_ident rpcid]
		error_check_bad diff:id [$db rpcid] [$db_diff rpcid]
		error_check_good ident2:id [$db_diff rpcid] [$db_diff2 rpcid]

		error_check_good db_close [$db_ident close] 0
		error_check_good db_close [$db_diff close] 0
		error_check_good db_close [$db_diff2 close] 0
		error_check_good db_close [$db close] 0

		puts "\tRpc005.f: Compare with a database and subdatabases"
		set db [eval {berkdb_open -auto_commit -create -btree \
		    -mode 0644} -env $env $testfile1 $subdb1]
		error_check_good dbopen [is_valid_db $db] TRUE
		set dbid [$db rpcid]

		set db2 [eval {berkdb_open -auto_commit -create -btree \
		    -mode 0644} -env $env $testfile1 $subdb2]
		error_check_good dbopen [is_valid_db $db2] TRUE
		set db2id [$db2 rpcid]
		error_check_bad 2subdb:id $dbid $db2id

		set db_ident [eval {berkdb_open -btree} -env $env \
		    $testfile1 $subdb1]
		error_check_good dbopen [is_valid_db $db_ident] TRUE
		set identid [$db_ident rpcid]

		set db_ident2 [eval {berkdb_open -btree} -env $env \
		    $testfile1 $subdb2]
		error_check_good dbopen [is_valid_db $db_ident2] TRUE
		set ident2id [$db_ident2 rpcid]

		set db_diff1 [eval {berkdb_open -btree} -env $env -rdonly \
		    $testfile1 $subdb1]
		error_check_good dbopen [is_valid_db $db_diff1] TRUE
		set diff1id [$db_diff1 rpcid]

		set db_diff2 [eval {berkdb_open -btree} -env $env -rdonly \
		    $testfile1 $subdb2]
		error_check_good dbopen [is_valid_db $db_diff2] TRUE
		set diff2id [$db_diff2 rpcid]

		set db_diff [eval {berkdb_open -unknown} -env $env -rdonly \
		    $testfile1]
		error_check_good dbopen [is_valid_db $db_diff] TRUE
		set diffid [$db_diff rpcid]

		set db_diff2a [eval {berkdb_open -btree} -env $env -rdonly \
		    $testfile1 $subdb2]
		error_check_good dbopen [is_valid_db $db_diff2a] TRUE
		set diff2aid [$db_diff2a rpcid]

		error_check_good ident:id $dbid $identid
		error_check_good ident2:id $db2id $ident2id
		error_check_bad diff:id $dbid $diffid
		error_check_bad diff2:id $db2id $diffid
		error_check_bad diff3:id $diff2id $diffid
		error_check_bad diff4:id $diff1id $diffid
		error_check_good diff2a:id $diff2id $diff2aid

		error_check_good db_close [$db_ident close] 0
		error_check_good db_close [$db_ident2 close] 0
		error_check_good db_close [$db_diff close] 0
		error_check_good db_close [$db_diff1 close] 0
		error_check_good db_close [$db_diff2 close] 0
		error_check_good db_close [$db_diff2a close] 0
		error_check_good db_close [$db2 close] 0
		error_check_good db_close [$db close] 0
		error_check_good env_close [$env close] 0
	} res]
	if { $status != 0 } {
		puts $res
	}
	tclkill $dpid
}
