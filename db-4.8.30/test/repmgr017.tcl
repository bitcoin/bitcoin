# See the file LICENSE for redistribution information.
#
# Copyright (c) 2007-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	repmgr017
# TEST	repmgr in-memory cache overflow test. 
# TEST
# TEST	Start an appointed master site and one client, putting databases,
# TEST	environment regions, logs and replication files in-memory. Set 
# TEST	very small cachesize and run enough transactions to overflow cache. 
# TEST	Shut down and restart master and client, giving master a larger cache. 
# TEST	Run and verify a small number of transactions.
# TEST
# TEST	Run for btree only because access method shouldn't matter.
# TEST
proc repmgr017 { method { niter 1000 } { tnum "017" } args } {

	source ./include.tcl

	if { $is_freebsd_test == 1 } {
		puts "Skipping replication manager test on FreeBSD platform."
		return
	}

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win9x platform."
		return
	}

	# Skip for all methods except btree.
	if { $checking_valid_methods } {
		return btree
	}
	if { [is_btree $method] == 0 } {
		puts "Repmgr$tnum: skipping for non-btree method $method."
		return
	}

	set args [convert_args $method $args]

	puts \
    "Repmgr$tnum ($method): repmgr in-memory cache overflow test."
	repmgr017_sub $method $niter $tnum $args
}

proc repmgr017_sub { method niter tnum largs } {
	global rep_verbose 
	global verbose_type 
	global databases_in_memory

	# Force databases in-memory for this test but preserve original
	# value to restore later so that other tests aren't affected.
	set restore_dbinmem $databases_in_memory
	set databases_in_memory 1

	# No need for test directories because test is entirely in-memory.

	set nsites 2
	set ports [available_ports $nsites]
	set omethod [convert_method $method]

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	# In-memory logs cannot be used with -txn nosync.
	set logargs [adjust_logargs "in-memory"]
	set txnargs [adjust_txnargs "in-memory"]

	# Use different connection retry timeout values to handle any
	# collisions from starting sites at the same time by retrying
	# at different times.

	# Open a master with a very small cache.
	puts "\tRepmgr$tnum.a: Start a master with a very small cache."
	set cacheargs "-cachesize {0 32768 1}"
	set ma_envcmd "berkdb_env_noerr -create $logargs $txnargs $verbargs \
	   -errpfx MASTER -rep -thread -rep_inmem_files -private $cacheargs"
	set masterenv [eval $ma_envcmd]
	$masterenv repmgr -ack all -nsites $nsites \
	    -timeout {conn_retry 20000000} \
	    -local [list localhost [lindex $ports 0]] \
	    -start master

	# Open a client
	puts "\tRepmgr$tnum.b: Start a client."
	set cl_envcmd "berkdb_env_noerr -create $logargs $txnargs $verbargs \
	    -errpfx CLIENT -rep -thread -rep_inmem_files -private"
	set clientenv [eval $cl_envcmd]
	$clientenv repmgr -ack all -nsites $nsites \
	    -timeout {conn_retry 10000000} \
	    -local [list localhost [lindex $ports 1]] \
	    -remote [list localhost [lindex $ports 0]] \
	    -start client
	await_startup_done $clientenv

	puts "\tRepmgr$tnum.c: Run master transactions and verify full cache."
	set dbname { "" "test.db" }
	set mdb [eval "berkdb_open_noerr -create $omethod -auto_commit \
	    -env $masterenv $largs $dbname"]
	set stat [catch {
	    rep_test $method $masterenv $mdb $niter 0 0 0 $largs } ret ]
	error_check_good broke $stat 1
	error_check_good nomem \
	    [is_substr $ret "not enough memory"] 1

	puts "\tRepmgr$tnum.d: Close master and client."
	error_check_good mdb_close [$mdb close] 0
	error_check_good client_close [$clientenv close] 0
	# Master close should return invalid argument.
	catch { $masterenv close } ret2
	error_check_good needrec [is_substr $ret2 "invalid argument"] 1

	puts "\tRepmgr$tnum.e: Restart master (with larger cache) and client."
	# Recovery is a no-op with everything in-memory, but specify it
	# anyway after closing the master environment with an error.
	set cacheargs ""
	set masterenv [eval $ma_envcmd -recover]
	$masterenv repmgr -ack all -nsites $nsites \
	    -timeout {conn_retry 20000000} \
	    -local [list localhost [lindex $ports 0]] \
	    -start master

	# Open -recover to clear env region, including startup_done value.
	set clientenv [eval $cl_envcmd -recover]
	$clientenv repmgr -ack all -nsites $nsites \
	    -timeout {conn_retry 10000000} \
	    -local [list localhost [lindex $ports 1]] \
	    -remote [list localhost [lindex $ports 0]] \
	    -start client
	await_startup_done $clientenv

	puts "\tRepmgr$tnum.f: Perform small number of transactions on master."
	set numtxns 10
	set mdb [eval "berkdb_open_noerr -create $omethod -auto_commit \
	    -env $masterenv $largs $dbname"]
	set t [$masterenv txn]
	for { set i 1 } { $i <= $numtxns } { incr i } {
		error_check_good db_put \
		    [eval $mdb put -txn $t $i [chop_data $method data$i]] 0
	}
	error_check_good txn_commit [$t commit] 0

	puts "\tRepmgr$tnum.g: Verify transactions on client."
	set cdb [eval "berkdb_open_noerr -create -mode 0644 $omethod \
	    -env $clientenv $largs $dbname"]
	error_check_good reptest_db [is_valid_db $cdb] TRUE
	for { set i 1 } { $i <= $numtxns } { incr i } {
		set ret [lindex [$cdb get $i] 0]
		error_check_good cdb_get $ret [list $i \
		    [pad_data $method data$i]]
	}

	# If the test had erroneously created replication files, they would
	# be in the current working directory. Verify that this didn't happen.
	puts "\tRepmgr$tnum.h: Verify no replication files on disk."
	no_rep_files_on_disk "."

	# Restore original databases_in_memory value.
	set databases_in_memory $restore_dbinmem

	error_check_good cdb_close [$cdb close] 0
	error_check_good mdb_close [$mdb close] 0
	error_check_good client_close [$clientenv close] 0
	error_check_good master_close [$masterenv close] 0
}
