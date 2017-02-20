# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	env006
# TEST	Make sure that all the utilities exist and run.
# TEST	Test that db_load -r options don't blow up.
proc env006 { } {
	source ./include.tcl

	puts "Env006: Run underlying utilities."

	set rlist {
	{ "db_archive"			"Env006.a"}
	{ "db_checkpoint"		"Env006.b"}
	{ "db_deadlock"			"Env006.c"}
	{ "db_dump"			"Env006.d"}
	{ "db_load"			"Env006.e"}
	{ "db_printlog"			"Env006.f"}
	{ "db_recover"			"Env006.g"}
	{ "db_stat"			"Env006.h"}
	{ "db_upgrade"			"Env006.h"}
	{ "db_verify"			"Env006.h"}
	}
	foreach pair $rlist {
		set cmd [lindex $pair 0]
		set msg [lindex $pair 1]

		puts "\t$msg: $cmd"

		set stat [catch {exec $util_path/$cmd -?} ret]
		error_check_good $cmd $stat 1

		#
		# Check for "usage", but only check "sage" so that
		# we can handle either Usage or usage.
		#
		error_check_good $cmd.err [is_substr $ret sage] 1
	}

	env_cleanup $testdir
	set env [eval berkdb_env -create -home $testdir -txn]
	error_check_good env_open [is_valid_env $env] TRUE

	set sub SUBDB
	foreach case { noenv env } {
		if { $case == "env" } {
			set envargs " -env $env "
			set homeargs " -h $testdir "
			set testfile env006.db
		} else {
			set envargs ""
			set homeargs ""
			set testfile $testdir/env006.db
		}

		puts "\tEnv006.i: Testing db_load -r with $case."
		set db [eval berkdb_open -create $envargs -btree $testfile]
		error_check_good db_open [is_valid_db $db] TRUE
		error_check_good db_close [$db close] 0

		set ret [eval \
		    exec $util_path/db_load -r lsn $homeargs $testfile]
		error_check_good db_load_r_lsn $ret ""
		set ret [eval \
		    exec $util_path/db_load -r fileid $homeargs $testfile]
		error_check_good db_load_r_fileid $ret ""

		error_check_good db_remove \
		    [eval {berkdb dbremove} $envargs $testfile] 0

		puts "\tEnv006.j: Testing db_load -r with $case and subdbs."
		set db [eval berkdb_open -create $envargs -btree $testfile $sub]
		error_check_good db_open [is_valid_db $db] TRUE
		error_check_good db_close [$db close] 0

		set ret [eval \
		    exec {$util_path/db_load} -r lsn $homeargs $testfile]
		error_check_good db_load_r_lsn $ret ""
		set ret [eval \
		    exec {$util_path/db_load} -r fileid $homeargs $testfile]
		error_check_good db_load_r_fileid $ret ""

		error_check_good \
		    db_remove [eval {berkdb dbremove} $envargs $testfile] 0
	}
	error_check_good env_close [$env close] 0
}
