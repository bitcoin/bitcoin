# See the file LICENSE for redistribution information.
#
# Copyright (c) 2007-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	repmgr011
# TEST	repmgr two site strict majority test.
# TEST
# TEST	Start an appointed master and one client with 2 site strict
# TEST	majority set. Shut down the master site, wait and verify that
# TEST	the client site was not elected master. Start up master site
# TEST	and verify that transactions are processed as expected.
# TEST
# TEST	Run for btree only because access method shouldn't matter.
# TEST
proc repmgr011 { method { niter 100 } { tnum "011" } args } {

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

	puts "Repmgr$tnum ($method): repmgr two site strict majority test."
	repmgr011_sub $method $niter $tnum $args
}

proc repmgr011_sub { method niter tnum largs } {
	global testdir
	global rep_verbose
	global verbose_type
	set nsites 2

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	env_cleanup $testdir
	set ports [available_ports $nsites]

	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR2

	file mkdir $clientdir
	file mkdir $clientdir2

	# Use different connection retry timeout values to handle any
	# collisions from starting sites at the same time by retrying
	# at different times.

	# Open first client as master and set 2site_strict.
	puts "\tRepmgr$tnum.a: Start first client as master."
	set cl_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx CLIENT -home $clientdir -txn -rep -thread"
	set clientenv [eval $cl_envcmd]
	$clientenv repmgr -ack all -nsites $nsites \
	    -timeout {conn_retry 20000000} \
	    -local [list localhost [lindex $ports 0]] \
	    -start master
	error_check_good c1strict [$clientenv rep_config {mgr2sitestrict on}] 0

	# Open second client and set 2site_strict.
	puts "\tRepmgr$tnum.b: Start second client."
	set cl2_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx CLIENT2 -home $clientdir2 -txn -rep -thread"
	set clientenv2 [eval $cl2_envcmd]
	$clientenv2 repmgr -ack all -nsites $nsites \
	    -timeout {conn_retry 10000000} \
	    -local [list localhost [lindex $ports 1]] \
	    -remote [list localhost [lindex $ports 0]] \
	    -start client
	await_startup_done $clientenv2
	error_check_good c2strict [$clientenv2 rep_config \
	    {mgr2sitestrict on}] 0

	#
	# Use of -ack all guarantees replication complete before repmgr send
	# function returns and rep_test finishes.
	#
	puts "\tRepmgr$tnum.c: Run first set of transactions at master."
	eval rep_test $method $clientenv NULL $niter 0 0 0 $largs

	puts "\tRepmgr$tnum.d: Verifying client database contents."
	rep_verify $clientdir $clientenv $clientdir2 $clientenv2 1 1 1

	puts "\tRepmgr$tnum.e: Shut down first client (current master)."
	error_check_good client_close [$clientenv close] 0

	puts "\tRepmgr$tnum.f: Wait, then verify no master."
	tclsleep 20
	error_check_bad c2_master [stat_field $clientenv2 rep_stat "Master"] 1

	puts "\tRepmgr$tnum.g: Restart first client as master"
	set clientenv [eval $cl_envcmd]
	$clientenv repmgr -ack all -nsites $nsites \
	    -timeout {conn_retry 20000000} \
	    -local [list localhost [lindex $ports 0]] \
	    -remote [list localhost [lindex $ports 1]] \
	    -start master
	await_expected_master $clientenv

	puts "\tRepmgr$tnum.h: Run second set of transactions at master."
	eval rep_test $method $clientenv NULL $niter $niter 0 0 $largs

	puts "\tRepmgr$tnum.i: Verifying client database contents."
	rep_verify $clientdir $clientenv $clientdir2 $clientenv2 1 1 1

	error_check_good client2_close [$clientenv2 close] 0
	error_check_good client_close [$clientenv close] 0
}
