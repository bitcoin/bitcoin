# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep053
# TEST	Replication and basic client-to-client synchronization.
# TEST
# TEST	Open and start up master and 1 client.
# TEST	Start up a second client later and verify it sync'ed from
# TEST	the original client, not the master.
#
proc rep053 { method { niter 200 } { tnum "053" } args } {
	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}

	# Valid for all access methods.
	if { $checking_valid_methods } {
		return "ALL"
	}

	set args [convert_args $method $args]
	set logsets [create_logsets 3]

	# Set up for on-disk or in-memory databases.
	set msg "using on-disk databases"
	if { $databases_in_memory } {
		set msg "using named in-memory databases"
		if { [is_queueext $method] } { 
			puts -nonewline "Skipping rep$tnum for method "
			puts "$method with named in-memory databases."
			return
		}
	}

	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	# Run the body of the test with and without recovery,
	# and with and without cleaning.  Skip recovery with in-memory
	# logging - it doesn't make sense.
	set throttle { "throttle" "" }
	foreach r $test_recopts {
		foreach t $throttle {
			foreach l $logsets {
				set logindex [lsearch -exact $l "in-memory"]
				if { $r == "-recover" && $logindex != -1 } {
					puts "Skipping rep$tnum for -recover\
					    with in-memory logs."
					continue
				}
				puts "Rep$tnum ($method $r $t): Replication\
				    and client-to-client sync up $msg $msg2."
				puts "Rep$tnum: Master logs are [lindex $l 0]"
				puts "Rep$tnum: Client logs are [lindex $l 1]"
				puts "Rep$tnum: Client2 logs are [lindex $l 2]"
				rep053_sub $method $niter $tnum $l $r $t $args
			}
		}
	}
}

proc rep053_sub { method niter tnum logset recargs throttle largs } {
	global anywhere
	global testdir
	global util_path
	global databases_in_memory
	global repfiles_in_memory
	global rep_verbose
	global verbose_type

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	set repmemargs ""
	if { $repfiles_in_memory } {
		set repmemargs "-rep_inmem_files "
	}

	env_cleanup $testdir
	set orig_tdir $testdir

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	set delaycldir1 $testdir/DELAYCLDIR.1
	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $delaycldir1

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]
	set c2_logtype [lindex $logset 2]

	# In-memory logs cannot be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set c2_logargs [adjust_logargs $c2_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]
	set c2_txnargs [adjust_txnargs $c2_logtype]

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs \
	    $m_logargs -errpfx MASTER $verbargs $repmemargs \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs -rep_master]

	# Open two clients
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs \
	    $c_logargs -errpfx CLIENT $verbargs $repmemargs \
	    -home $clientdir -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd $recargs -rep_client]

	# If throttling is specified, turn it on here.  Throttle the
	# client, since this is a test of client-to-client sync-up.
	if { $throttle == "throttle" } {
		error_check_good \
		    throttle [$clientenv rep_limit 0 [expr 8 * 1024]] 0
	}

	#
	# Set up delayed client command, but don't eval until later.
	# !!! Do NOT put the 'repladd' call here because we don't
	# want this client to already have the backlog of records
	# when it starts.
	#
	set dc1_envcmd "berkdb_env_noerr -create $c2_txnargs \
	    $c2_logargs -errpfx DELAYCL $verbargs $repmemargs \
	    -home $delaycldir1 -rep_transport \[list 3 replsend\]"

	# Bring the client online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	puts "\tRep$tnum.a: Run rep_test in master env."
	set start 0
	eval rep_test $method $masterenv NULL $niter $start $start 0 $largs
	incr start $niter
	process_msgs $envlist

	puts "\tRep$tnum.b: Start new client."
	set anywhere 1
	repladd 3
	set newclient [eval $dc1_envcmd $recargs -rep_client]
	error_check_good client2_env [is_valid_env $newclient] TRUE

	set envlist "{$masterenv 1} {$clientenv 2} {$newclient 3}"
	process_msgs $envlist

	puts "\tRep$tnum.c: Verify sync-up from client."
	set req [stat_field $clientenv rep_stat "Client service requests"]
	set miss [stat_field $clientenv rep_stat "Client service req misses"]
	set rereq [stat_field $newclient rep_stat "Client rerequests"]

	# To complete the internal init, we need a PAGE_REQ and a LOG_REQ.  These
	# requests get served by $clientenv.  Since the end-of-range specified
	# in the LOG_REQ points to the very end of the log (i.e., the LSN given
	# in the NEWMASTER message), the serving client gets NOTFOUND in its log
	# cursor reading loop, and can't tell whether it simply hit the end, or
	# is really missing sufficient log records to fulfill the request.  So
	# it counts a "miss" and generates a rerequest.  When internal init
	# finishes recovery, it sends an ALL_REQ, for a total of 3 requests in
	# the simple case, and more than 3 in the "throttle" case.
	#

	set expected_msgs 3
	if { [is_queue $method] } {
		# Queue database require an extra request
		# to retrieve the meta page.
		incr expected_msgs
	}

	if { $throttle == "throttle" } {
		error_check_good req [expr $req > $expected_msgs] 1
	} else {
		error_check_good min_req [expr $req >= $expected_msgs] 1
		set max_expected_msgs [expr $expected_msgs * 2] 
		error_check_good max_req [expr $req <= $max_expected_msgs] 1
	}
	error_check_good miss=rereq $miss $rereq

	# Check for throttling.
	if { $throttle == "throttle" } {
		set num_throttles \
		    [stat_field $clientenv rep_stat "Transmission limited"]
		error_check_bad client_throttling $num_throttles 0
	}

	rep_verify $masterdir $masterenv $clientdir $clientenv 0 1 1

	# Process messages again in case we are running with debug_rop.
	process_msgs $envlist
	rep_verify $masterdir $masterenv $delaycldir1 $newclient 0 1 1

	puts "\tRep$tnum.d: Run rep_test more in master env and verify."
	set niter 10
	eval rep_test $method $masterenv NULL $niter $start $start 0 $largs
	incr start $niter
	process_msgs $envlist
	rep_verify $masterdir $masterenv $clientdir $clientenv 0 1 1
	process_msgs $envlist
	rep_verify $masterdir $masterenv $delaycldir1 $newclient 0 1 1

	puts "\tRep$tnum.e: Closing"
	error_check_good master_close [$masterenv close] 0
	error_check_good client_close [$clientenv close] 0
	error_check_good dc1_close [$newclient close] 0
	replclose $testdir/MSGQUEUEDIR
	set testdir $orig_tdir
	set anywhere 0
	return
}
