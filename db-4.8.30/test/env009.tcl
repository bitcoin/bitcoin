# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	env009
# TEST	Test calls to all the various stat functions.  We have several
# TEST	sprinkled throughout the test suite, but this will ensure that
# TEST	we run all of them at least once.
proc env009 { } {
	source ./include.tcl

	puts "Env009: Various stat functions test."

	env_cleanup $testdir
	puts "\tEnv009.a: Setting up env and a database."

	set e [berkdb_env -create -home $testdir -txn]
	error_check_good dbenv [is_valid_env $e] TRUE
	set dbbt [berkdb_open -create -btree $testdir/env009bt.db]
	error_check_good dbopen [is_valid_db $dbbt] TRUE
	set dbh [berkdb_open -create -hash $testdir/env009h.db]
	error_check_good dbopen [is_valid_db $dbh] TRUE
	set dbq [berkdb_open -create -queue $testdir/env009q.db]
	error_check_good dbopen [is_valid_db $dbq] TRUE

	puts "\tEnv009.b: Setting up replication master and client envs."
	replsetup $testdir/MSGQUEUEDIR
	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	file mkdir $masterdir
	file mkdir $clientdir

	repladd 1
	set repenv(M) [berkdb_env -create -home $masterdir \
	    -txn -rep_master -rep_transport [list 1 replsend]]
	repladd 2
	set repenv(C) [berkdb_env -create -home $clientdir \
	    -txn -rep_client -rep_transport [list 2 replsend]]

	set rlist {
	{ "lock_stat" "Maximum locks" "Env009.c" $e }
	{ "log_stat" "Magic" "Env009.d" "$e" }
	{ "mpool_stat" "Number of caches" "Env009.e" "$e"}
	{ "txn_stat" "Maximum txns" "Env009.f" "$e" }
	{ "rep_stat" "{Environment ID} 1" "Env009.g (Master)" "$repenv(M)"}
	{ "rep_stat" "{Environment ID} 2" "Env009.h (Client)" "$repenv(C)"}
	}

	foreach set $rlist {
		set cmd [lindex $set 0]
		set str [lindex $set 1]
		set msg [lindex $set 2]
		set env [lindex $set 3]
		puts "\t$msg: $cmd"
		set ret [eval $env $cmd]
		error_check_good $cmd [is_substr $ret $str] 1
	}

	puts "\tEnv009.i: btree stats"
	set ret [$dbbt stat]
	error_check_good $cmd [is_substr $ret "Leaf pages"] 1

	puts "\tEnv009.j: hash stats"
	set ret [$dbh stat]
	error_check_good $cmd [is_substr $ret "Buckets"] 1

	puts "\tEnv009.k: queue stats"
	set ret [$dbq stat]
	error_check_good $cmd [is_substr $ret "Extent size"] 1

	# Clean up.
	error_check_good dbclose [$dbbt close] 0
	error_check_good dbclose [$dbh close] 0
	error_check_good dbclose [$dbq close] 0
	error_check_good masterenvclose [$repenv(M) close] 0
	error_check_good clientenvclose [$repenv(C) close] 0
	replclose $testdir/MSGQUEUEDIR
	error_check_good envclose [$e close] 0
}
