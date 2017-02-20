# See the file LICENSE for redistribution information.
#
# Copyright (c) 2007-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	repmgr010
# TEST	Acknowledgement policy and timeout test. 
# TEST
# TEST	Verify that "quorum" acknowledgement policy succeeds with fewer than 
# TEST	nsites running. Verify that "all" acknowledgement policy results in 
# TEST	ack failures with fewer than nsites running.
# TEST
# TEST	Run for btree only because access method shouldn't matter.
# TEST
proc repmgr010 { method { niter 100 } { tnum "010" } args } {

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

	puts "Repmgr$tnum ($method): repmgr ack policy and timeout test."
	repmgr010_sub $method $niter $tnum $args
}

proc repmgr010_sub { method niter tnum largs } {
	global testdir
	global rep_verbose
	global verbose_type
	set nsites 3

	set small_iter [expr $niter / 10]

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	env_cleanup $testdir
	set ports [available_ports $nsites]

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR2

	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $clientdir2

	# Use different connection retry timeout values to handle any
	# collisions from starting sites at the same time by retrying
	# at different times.

	puts "\tRepmgr$tnum.a: Start master, two clients, ack policy quorum."
	# Open a master.
	set ma_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx MASTER -home $masterdir -txn -rep -thread"
	set masterenv [eval $ma_envcmd]
	$masterenv repmgr -ack quorum -nsites $nsites \
	    -timeout {conn_retry 20000000} \
	    -local [list localhost [lindex $ports 0]] \
	    -start master

	# Open first client
	set cl_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx CLIENT -home $clientdir -txn -rep -thread"
	set clientenv [eval $cl_envcmd]
	$clientenv repmgr -ack quorum -nsites $nsites \
	    -timeout {conn_retry 10000000} \
	    -local [list localhost [lindex $ports 1]] \
	    -remote [list localhost [lindex $ports 0]] \
	    -remote [list localhost [lindex $ports 2]] \
	    -start client
	await_startup_done $clientenv

	# Open second client
	set cl2_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx CLIENT2 -home $clientdir2 -txn -rep -thread"
	set clientenv2 [eval $cl2_envcmd]
	$clientenv2 repmgr -ack quorum -nsites $nsites \
	    -timeout {conn_retry 5000000} \
	    -local [list localhost [lindex $ports 2]] \
	    -remote [list localhost [lindex $ports 0]] \
	    -remote [list localhost [lindex $ports 1]] \
	    -start client
	await_startup_done $clientenv2

	puts "\tRepmgr$tnum.b: Run first set of transactions at master."
	set start 0
	eval rep_test $method $masterenv NULL $niter $start 0 0 $largs
	incr start $niter

	#
	# Special verification needed for quorum ack policy. Wait
	# longer than ack timeout (default 1 second) then check for 
	# ack failures (perm_failed events). Quorum only guarantees
	# that transactions replicated to one site or the other, so
	# test for this condition instead of both sites.
	#
	puts "\tRepmgr$tnum.c: Verify both client databases, no ack failures."
	tclsleep 5
	error_check_good quorum_perm_failed1 \
	    [stat_field $masterenv repmgr_stat "Acknowledgement failures"] 0
	catch {rep_verify\
	    $masterdir $masterenv $clientdir $clientenv 1 1 1} ver1
	catch {rep_verify\
	    $masterdir $masterenv $clientdir2 $clientenv2 1 1 1} ver2
	error_check_good onesite [expr [string length $ver1] == 0 || \
	    [string length $ver2] == 0] 1

	puts "\tRepmgr$tnum.d: Shut down first client."
	error_check_good client_close [$clientenv close] 0

	puts "\tRepmgr$tnum.e: Run second set of transactions at master."
	eval rep_test $method $masterenv NULL $small_iter $start 0 0 $largs
	incr start $niter

	puts "\tRepmgr$tnum.f: Verify client database, no ack failures."
	tclsleep 5
	error_check_good quorum_perm_failed2 \
	    [stat_field $masterenv repmgr_stat "Acknowledgement failures"] 0
	rep_verify $masterdir $masterenv $clientdir2 $clientenv2 1 1 1

	puts "\tRepmgr$tnum.g: Adjust all sites to ack policy all."
	# Reopen first client with ack policy all
	set cl_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx CLIENT -home $clientdir -txn -rep -thread"
	# Open -recover to clear env region, including startup_done value.
	set clientenv [eval $cl_envcmd -recover]
	$clientenv repmgr -ack all -nsites $nsites \
	    -timeout {conn_retry 10000000} \
	    -local [list localhost [lindex $ports 1]] \
	    -remote [list localhost [lindex $ports 0]] \
	    -remote [list localhost [lindex $ports 2]] \
	    -start client
	await_startup_done $clientenv

	# Adjust other sites to ack policy all
	$masterenv repmgr -ack all
	$clientenv2 repmgr -ack all

	puts "\tRepmgr$tnum.h: Shut down first client."
	error_check_good client_close [$clientenv close] 0
	set init_perm_failed \
	    [stat_field $masterenv repmgr_stat "Acknowledgement failures"]

	#
	# Use of -ack all guarantees replication complete before repmgr send
	# function returns and rep_test finishes.
	#
	puts "\tRepmgr$tnum.i: Run third set of transactions at master."
	eval rep_test $method $masterenv NULL $small_iter $start 0 0 $largs

	puts "\tRepmgr$tnum.j: Verify client database, some ack failures."
	rep_verify $masterdir $masterenv $clientdir2 $clientenv2 1 1 1
	error_check_good all_perm_failed [expr \
	    [stat_field $masterenv repmgr_stat "Acknowledgement failures"] \
	    > $init_perm_failed] 1

	error_check_good client2_close [$clientenv2 close] 0
	error_check_good masterenv_close [$masterenv close] 0
}
