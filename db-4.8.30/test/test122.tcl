# See the file LICENSE for redistribution information.
#
# Copyright (c) 2006-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test122
# TEST	Tests of multi-version concurrency control.
# TEST
# TEST	MVCC and databases that turn multi-version on and off.

proc test122 { method {tnum "122"} args } {
	source ./include.tcl

	# This test needs its own env.
	set eindex [lsearch -exact $args "-env"]
	if { $eindex != -1 } {
		incr eindex
		set env [lindex $args $eindex]
		puts "Test$tnum skipping for env $env"
		return
	}

	# MVCC is not allowed with queue methods.
	if { [is_queue $method] == 1 } {
		puts "Test$tnum skipping for method $method"
		return
	}

	puts "\tTest$tnum ($method): Turning MVCC on and off."

	set args [convert_args $method $args]
	set omethod [convert_method $method]
	set encargs ""
	set args [split_encargs $args encargs]
	set pageargs ""
	split_pageargs $args pageargs
	set filename "test.db"

	# Create transactional env.  Don't specify -multiversion to
	# the env, because we need to turn it on and off.
	env_cleanup $testdir

	puts "\tTest$tnum.a: Creating txn env."
	set cacheargs " -cachesize {0 524288 1} "
	set env [eval {berkdb_env}\
	    -create $cacheargs -txn $pageargs $encargs -home $testdir]
	error_check_good env_open [is_valid_env $env] TRUE

	# Open database.
	puts "\tTest$tnum.b: Creating -multiversion db."
	set db [eval {berkdb_open} -multiversion \
	    -create -auto_commit -env $env $omethod $args $filename]
	error_check_good db_open [is_valid_db $db] TRUE

	# Put some data.  The tcl interface automatically does it
	# transactionally.
	set niter 100
	for { set i 1 } { $i < $niter } { incr i } {
		set key $i
		set data DATA.$i
		error_check_good db_put [eval {$db put} $key $data] 0
	}

	# Open a read-only handle and also a txn -snapshot handle.
	puts "\tTest$tnum.c: Open read-only handle and txn -snapshot handle."
	set t [$env txn -snapshot]
	set txn "-txn $t"
	set snapshotdb [eval {berkdb_open} \
	    $txn -env $env $omethod $args $filename]
	error_check_good snapshotdb [is_valid_db $snapshotdb] TRUE
 	set readonlydb [eval {berkdb_open} \
	    -auto_commit -env $env $omethod $args $filename]
	error_check_good readonlydb [is_valid_db $readonlydb] TRUE


	# Overwrite all the data.  The read-only handle will see the
	# new data and the -snapshot handle will see the old data.
	puts "\tTest$tnum.d: Overwrite data."
	for { set i 1 } { $i < $niter } { incr i } {
		set key $i
		set data NEWDATA.$i
		error_check_good db_put [eval {$db put} $key $data] 0
	}

	puts "\tTest$tnum.e: Check data through handles."
	for { set i 1 } { $i < $niter } { incr i } {
		set r_ret [eval {$readonlydb get} $i]
		set s_ret [eval {$snapshotdb get} $txn $i]
		set r_key [lindex [lindex $r_ret 0] 0]
		set r_data [lindex [lindex $r_ret 0] 1]
		set s_key [lindex [lindex $s_ret 0] 0]
		set s_data [lindex [lindex $s_ret 0] 1]
	}

	error_check_good t_commit [$t commit] 0

	# Clean up.
	error_check_good db_close [$db close] 0
	error_check_good snapshotdb_close [$snapshotdb close] 0
	error_check_good readonlydb_close [$readonlydb close] 0
	error_check_good env_close [$env close] 0
}
