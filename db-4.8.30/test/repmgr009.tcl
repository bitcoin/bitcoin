# See the file LICENSE for redistribution information.
#
# Copyright (c) 2007-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	repmgr009
# TEST	repmgr API error test.
# TEST
# TEST	Try a variety of repmgr calls that result in errors. Also
# TEST	try combinations of repmgr and base replication API calls
# TEST	that result in errors.
# TEST
# TEST	Run for btree only because access method shouldn't matter.
# TEST
proc repmgr009 { method { niter 10 } { tnum "009" } args } {

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

	puts "Repmgr$tnum ($method): repmgr API error test."
	repmgr009_sub $method $niter $tnum $args
}

proc repmgr009_sub { method niter tnum largs } {
	global testdir
	global rep_verbose
	global verbose_type
	set nsites 2

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	env_cleanup $testdir
	set ports [available_ports [expr $nsites * 5]]

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	set masterdir2 $testdir/MASTERDIR2
	set clientdir $testdir/CLIENTDIR

	file mkdir $masterdir
	file mkdir $masterdir2
	file mkdir $clientdir

	# Use different connection retry timeout values to handle any
	# collisions from starting sites at the same time by retrying
	# at different times.

	puts "\tRepmgr$tnum.a: Set up environment without repmgr."
	set ma_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx MASTER -home $masterdir -txn -rep -thread"
	set masterenv [eval $ma_envcmd]
	error_check_good masterenv_close [$masterenv close] 0

	puts "\tRepmgr$tnum.b: Call repmgr without open master (error)."
	catch {$masterenv repmgr -ack all -nsites $nsites \
	    -timeout {conn_retry 20000000} \
	    -local [list localhost [lindex $ports 0]] \
	    -start master} res
	error_check_good errchk [is_substr $res "invalid command"] 1

	puts "\tRepmgr$tnum.c: Call repmgr_stat without open master (error)."
	catch {[stat_field $masterenv repmgr_stat "Connections dropped"]} res
	error_check_good errchk [is_substr $res "invalid command"] 1

	puts "\tRepmgr$tnum.d: Start a master with repmgr."
	repladd 1
	set masterenv [eval $ma_envcmd]
	$masterenv repmgr -ack all -nsites $nsites \
	    -timeout {conn_retry 20000000} \
	    -local [list localhost [lindex $ports 0]] \
	    -start master

	puts "\tRepmgr$tnum.e: Start repmgr with no local sites (error)."
	set cl_envcmd "berkdb_env_noerr -create $verbargs \
	    -home $clientdir -txn -rep -thread"
	set clientenv [eval $cl_envcmd]
	catch {$clientenv repmgr -ack all -nsites $nsites \
	    -timeout {conn_retry 10000000} \
	    -remote [list localhost [lindex $ports 7]] \
	    -start client} res
	error_check_good errchk [is_substr $res \
	    "set_local_site must be called before repmgr_start"] 1
	error_check_good client_close [$clientenv close] 0

	puts "\tRepmgr$tnum.f: Start repmgr with two local sites (error)."
	set clientenv [eval $cl_envcmd]
	catch {$clientenv repmgr -ack all -nsites $nsites \
	    -timeout {conn_retry 10000000} \
	    -local [list localhost [lindex $ports 8]] \
	    -local [list localhost [lindex $ports 9]] \
	    -start client} res
	error_check_good errchk [string match "*already*set*" $res] 1
	error_check_good client_close [$clientenv close] 0

	puts "\tRepmgr$tnum.g: Start a client."
	repladd 2
	set clientenv [eval $cl_envcmd -recover]
	$clientenv repmgr -ack all -nsites $nsites \
	    -timeout {conn_retry 10000000} \
	    -local [list localhost [lindex $ports 1]] \
	    -remote [list localhost [lindex $ports 0]] \
	    -start client
	await_startup_done $clientenv

	puts "\tRepmgr$tnum.h: Start repmgr a second time (error)."
	catch {$clientenv repmgr -ack all -nsites $nsites \
	    -timeout {conn_retry 10000000} \
	    -local [list localhost [lindex $ports 1]] \
	    -remote [list localhost [lindex $ports 0]] \
	    -start client} res
	error_check_good errchk [is_substr $res "must be called before"] 1

	puts "\tRepmgr$tnum.i: Call rep_start after starting repmgr (error)."
	catch {set clientenv [eval $cl_envcmd -rep_client -rep_transport \
	    \[list 2 replsend\]]} res
	error_check_good errchk [is_substr $res \
	    "type mismatch for a replication process"] 1

	puts "\tRepmgr$tnum.j: Call rep_process_message (error)."
	set envlist "{$masterenv 1} {$clientenv 2}"
	catch {$clientenv rep_process_message 0 0 0} res
	error_check_good errchk [is_substr $res \
	    "cannot call from Replication Manager application"] 1

	#
	# Use of -ack all guarantees replication complete before repmgr send
	# function returns and rep_test finishes.
	#
	puts "\tRepmgr$tnum.k: Run some transactions at master."
	eval rep_test $method $masterenv NULL $niter $niter 0 0 $largs

	puts "\tRepmgr$tnum.l: Call rep_elect (error)."
	catch {$clientenv rep_elect 2 2 2 5000000} res
	error_check_good errchk [is_substr $res \
	    "cannot call from Replication Manager application"] 1

	puts "\tRepmgr$tnum.m: Verifying client database contents."
	rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 1

	error_check_good client_close [$clientenv close] 0
	error_check_good masterenv_close [$masterenv close] 0

	puts "\tRepmgr$tnum.n: Start a master with base API rep_start."
	set ma_envcmd2 "berkdb_env_noerr -create $verbargs \
	    -home $masterdir2 -errpfx MASTER -txn -thread -rep_master \
	    -rep_transport \[list 1 replsend\]"
	set masterenv2 [eval $ma_envcmd2]

	puts "\tRepmgr$tnum.o: Call repmgr after rep_start (error)."
	catch {$masterenv2 repmgr -ack all -nsites $nsites \
	    -local [list localhost [lindex $ports 0]] \
	    -start master} res
	# Internal repmgr calls return EINVAL after hitting
	# base API application test.
	error_check_good errchk [is_substr $res "invalid argument"] 1

	error_check_good masterenv_close [$masterenv2 close] 0
	replclose $testdir/MSGQUEUEDIR
}
