# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	env005
# TEST	Test that using subsystems without initializing them correctly
# TEST	returns an error.  Cannot test mpool, because it is assumed in
# TEST	the Tcl code.
proc env005 { } {
	source ./include.tcl

	puts "Env005: Uninitialized env subsystems test."

	env_cleanup $testdir
	puts "\tEnv005.a: Creating env with no subsystems."
	set e [berkdb_env_noerr -create -home $testdir]
	error_check_good dbenv [is_valid_env $e] TRUE
	set db [berkdb_open -create -btree $testdir/env005.db]
	error_check_good dbopen [is_valid_db $db] TRUE

	set rlist {
	{ "lock_detect"			"Env005.b0"}
	{ "lock_get read 1 1"		"Env005.b1"}
	{ "lock_id"			"Env005.b2"}
	{ "lock_stat"			"Env005.b3"}
	{ "lock_timeout 100"		"Env005.b4"}
	{ "log_archive"			"Env005.c0"}
	{ "log_cursor"			"Env005.c1"}
	{ "log_file {1 1}"		"Env005.c2"}
	{ "log_flush"			"Env005.c3"}
	{ "log_put record"		"Env005.c4"}
	{ "log_stat"			"Env005.c5"}
	{ "txn"				"Env005.d0"}
	{ "txn_checkpoint"		"Env005.d1"}
	{ "txn_stat"			"Env005.d2"}
	{ "txn_timeout 100"		"Env005.d3"}
	}

	foreach pair $rlist {
		set cmd [lindex $pair 0]
		set msg [lindex $pair 1]
		puts "\t$msg: $cmd"
		set stat [catch {eval $e $cmd} ret]
		error_check_good $cmd $stat 1
		error_check_good $cmd.err [is_substr $ret invalid] 1
	}
	error_check_good dbclose [$db close] 0
	error_check_good envclose [$e close] 0
}
