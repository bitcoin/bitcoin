# See the file LICENSE for redistribution information.
#
# Copyright (c) 2007-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	repmgr013
# TEST	Site list test. 
# TEST
# TEST	Configure a master and two clients where one client is a peer of 
# TEST	the other and verify resulting site lists.
# TEST
# TEST	Run for btree only because access method shouldn't matter.
# TEST
proc repmgr013 { method { niter 100 } { tnum "013" } args } {

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

	puts "Repmgr$tnum ($method): repmgr site list test."
	repmgr013_sub $method $niter $tnum $args
}

proc repmgr013_sub { method niter tnum largs } {
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

	puts "\tRepmgr$tnum.a: Start a master."
	set ma_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx MASTER -home $masterdir -txn -rep -thread"
	set masterenv [eval $ma_envcmd]
	$masterenv repmgr -ack all -nsites $nsites \
	    -timeout {conn_retry 20000000} \
	    -local [list localhost [lindex $ports 0]] \
	    -start master

	puts "\tRepmgr$tnum.b: Start first client."
	set cl_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx CLIENT -home $clientdir -txn -rep -thread"
	set clientenv [eval $cl_envcmd]
	$clientenv repmgr -ack all -nsites $nsites \
	    -timeout {conn_retry 10000000} \
	    -local [list localhost [lindex $ports 1]] \
	    -remote [list localhost [lindex $ports 0]] \
	    -remote [list localhost [lindex $ports 2]] \
	    -start client
	await_startup_done $clientenv

	puts "\tRepmgr$tnum.c: Start second client as peer of first."
	set cl2_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx CLIENT2 -home $clientdir2 -txn -rep -thread"
	set clientenv2 [eval $cl2_envcmd]
	$clientenv2 repmgr -ack all -nsites $nsites \
	    -timeout {conn_retry 5000000} \
	    -local [list localhost [lindex $ports 2]] \
	    -remote [list localhost [lindex $ports 0]] \
	    -remote [list localhost [lindex $ports 1] peer] \
	    -start client
	await_startup_done $clientenv2

	puts "\tRepmgr$tnum.d: Get repmgr site lists and verify."
	verify_sitelist [$masterenv repmgr_site_list]
	verify_sitelist [$clientenv repmgr_site_list]
	verify_sitelist [$clientenv2 repmgr_site_list]

	error_check_good client2_close [$clientenv2 close] 0
	error_check_good client_close [$clientenv close] 0
	error_check_good masterenv_close [$masterenv close] 0
}

proc verify_sitelist { sitelist } {
	# Make sure there are 2 other sites.
	error_check_good lenchk [llength $sitelist] 2

	# Make sure eid, port and status are integers, host is
	# expected string value.
	foreach tuple $sitelist {
		error_check_good eidchk [string is integer -strict \
		    [lindex $tuple 0]] 1
		error_check_good hostchk [lindex $tuple 1] "localhost"
		error_check_good eidchk [string is integer -strict \
		    [lindex $tuple 2]] 1
		error_check_bad statchk [lindex $tuple 3] unknown
	}
}
