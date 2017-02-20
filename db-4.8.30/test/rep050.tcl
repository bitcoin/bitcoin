# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep050
# TEST	Replication and delay syncing clients - change master test.
# TEST
# TEST	Open and start up master and 4 clients.  Turn on delay for 3 clients.
# TEST	Switch masters, add data and verify delayed clients are out of date.
# TEST	Make additional changes to master.  And change masters again.
# TEST	Sync/update delayed client and verify.  The 4th client is a brand
# TEST	new delayed client added in to test the non-verify path.
# TEST
# TEST	Then test two different things:
# TEST	1. Swap master again while clients are still delayed.
# TEST	2. Swap master again while sync is proceeding for one client.
#
proc rep050 { method { niter 10 } { tnum "050" } args } {
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
	set logsets [create_logsets 5]

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

	# Run the body of the test with and without recovery.
	foreach r $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Rep$tnum: Skipping\
				    for in-memory logs with -recover."
				continue
			}
			puts "Rep$tnum ($r): Replication\
			    and ($method) delayed sync-up $msg $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client 0 logs are [lindex $l 1]"
			puts "Rep$tnum: Delay Client 1 logs are [lindex $l 2]"
			puts "Rep$tnum: Delay Client 2 logs are [lindex $l 3]"
			puts "Rep$tnum: Delay Client 3 logs are [lindex $l 4]"
			rep050_sub $method $niter $tnum $l $r $args
		}
	}
}

proc rep050_sub { method niter tnum logset recargs largs } {
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

	set env1dir $testdir/MASTERDIR
	set env2dir $testdir/CLIENTDIR
	set delaycldir1 $testdir/DELAYCLDIR.1
	set delaycldir2 $testdir/DELAYCLDIR.2
	set delaycldir3 $testdir/DELAYCLDIR.3
	file mkdir $env1dir
	file mkdir $env2dir
	file mkdir $delaycldir1
	file mkdir $delaycldir2
	file mkdir $delaycldir3

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]
	set dc1_logtype [lindex $logset 2]
	set dc2_logtype [lindex $logset 3]
	set dc3_logtype [lindex $logset 4]

	# In-memory logs require a large log buffer, and cannot
	# be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set dc1_logargs [adjust_logargs $dc1_logtype]
	set dc2_logargs [adjust_logargs $dc2_logtype]
	set dc3_logargs [adjust_logargs $dc3_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]
	set dc1_txnargs [adjust_txnargs $dc1_logtype]
	set dc2_txnargs [adjust_txnargs $dc2_logtype]
	set dc3_txnargs [adjust_txnargs $dc3_logtype]

	#
	# XXX rep050 delayed sync-up but change master:
	# while client is delayed.
	# while client is in the middle of delayed sync.

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs \
	    $m_logargs -errpfx ENV1 $verbargs $repmemargs \
	    -home $env1dir -rep_transport \[list 1 replsend\]"
	set env1 [eval $ma_envcmd $recargs -rep_master]
	$env1 rep_limit 0 0

	# Open two clients
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs \
	    $c_logargs -errpfx ENV2 $verbargs $repmemargs \
	    -cachesize {0 2097152 2} \
	    -home $env2dir -rep_transport \[list 2 replsend\]"
	set env2 [eval $cl_envcmd $recargs -rep_client]
	$env2 rep_limit 0 0

	repladd 3
	set dc1_envcmd "berkdb_env_noerr -create $dc1_txnargs \
	    $dc1_logargs -errpfx ENV3 $verbargs $repmemargs \
	    -home $delaycldir1 -rep_transport \[list 3 replsend\]"
	set dc1env [eval $dc1_envcmd $recargs -rep_client]
	$dc1env rep_limit 0 0

	repladd 4
	set dc2_envcmd "berkdb_env_noerr -create $dc2_txnargs \
	    $dc2_logargs -errpfx ENV4 $verbargs $repmemargs \
	    -home $delaycldir2 -rep_transport \[list 4 replsend\]"
	set dc2env [eval $dc2_envcmd $recargs -rep_client]
	$dc2env rep_limit 0 0

	repladd 5
	set dc3_envcmd "berkdb_env_noerr -create $dc3_txnargs \
	    $dc3_logargs -errpfx ENV5 $verbargs $repmemargs \
	    -home $delaycldir3 -rep_transport \[list 5 replsend\]"

	# Bring the clients online by processing the startup messages.
	# !!!
	# NOTE: We set up dc3_envcmd but we do not open the env now.
	# Therefore dc3env is not part of the envlist.  However, since
	# we did the repladd broadcast messages will be sent to it,
	# but we will replclear before we start the env.
	#
	set envlist "{$env1 1} {$env2 2} {$dc1env 3} {$dc2env 4}"
	process_msgs $envlist

	puts "\tRep$tnum.a: Run rep_test in master env."
	set start 0
	eval rep_test $method $env1 NULL $niter $start $start 0 $largs

	process_msgs $envlist

	puts "\tRep$tnum.b: Set delayed sync on clients 2 and 3"
	error_check_good set_delay [$dc1env rep_config {delayclient on}] 0
	error_check_good set_delay [$dc2env rep_config {delayclient on}] 0

	set oplist { "delayed" "syncing" }

	set masterenv $env1
	set mid 1
	set mdir $env1dir
	set clientenv $env2
	set cid 2
	set cdir $env2dir
	foreach op $oplist {
		# Swap all the info we need.
		set tmp $masterenv
		set masterenv $clientenv
		set clientenv $tmp

		set tmp $mdir
		set mdir $cdir
		set cdir $mdir

		set tmp $mid
		set mid $cid
		set cid $tmp

		puts "\tRep$tnum.c: Swap master/client ($op)"
		error_check_good downgrade [$clientenv rep_start -client] 0
		error_check_good upgrade [$masterenv rep_start -master] 0
		process_msgs $envlist

		#
		# !!!
		# At this point, clients 2 and 3 should have DELAY set.
		# We should # probably add a field to rep_stat
		# to indicate that and test that here..
		#
		puts "\tRep$tnum.d: Run rep_test in new master env"
		set start [expr $start + $niter]
		eval rep_test $method $env2 NULL $niter $start $start 0 $largs
		process_msgs $envlist

		#
		# Delayed clients should be different.
		# Former master should by synced.
		#
		rep_verify $mdir $masterenv $cdir $clientenv 0 1 1
		rep_verify $mdir $masterenv $delaycldir1 $dc1env 0 0 0
		rep_verify $mdir $masterenv $delaycldir2 $dc2env 0 0 0

		#
		# Run rep_test again, but don't process on former master.
		# This makes the master/client different from each other.
		#
		puts "\tRep$tnum.e: Run rep_test in new master env only"
		set start [expr $start + $niter]
		eval rep_test \
		    $method $masterenv NULL $niter $start $start 0 $largs
		replclear $cid
		replclear 3
		replclear 4
		replclear 5

		puts "\tRep$tnum.f: Start 4th, clean delayed client."
		set dc3env [eval $dc3_envcmd $recargs -rep_client]
		error_check_good client4_env [is_valid_env $dc3env] TRUE
		$dc3env rep_limit 0 0
		error_check_good set_delay [$dc3env rep_config \
		    {delayclient on}] 0
		set envlist "{$env1 1} {$env2 2} {$dc1env 3} \
		    {$dc2env 4} {$dc3env 5}"
		process_msgs $envlist

		#
		# Now we have a master at point 1, a former master,
		# now client at point 2, and two delayed clients at point 3.
		# If 'delayed' swap masters now, while the clients are
		# in the delayed state but not syncing yet.
		# If 'syncing', first call rep_sync, and begin syncing the
		# clients, then swap masters in the middle of that process.
		#
		set nextlet "g"
		if { $op == "delayed" } {
			# Swap all the info we need.
			set tmp $masterenv
			set masterenv $clientenv
			set clientenv $tmp

			set tmp $mdir
			set mdir $cdir
			set cdir $mdir

			set tmp $mid
			set mid $cid
			set cid $tmp

			puts "\tRep$tnum.g: Swap master/client while delayed"
			set nextlet "h"
			error_check_good downgrade \
			    [$clientenv rep_start -client] 0
			error_check_good upgrade \
			    [$masterenv rep_start -master] 0
			process_msgs $envlist
		}
		puts "\tRep$tnum.$nextlet: Run rep_test and sync delayed client"
		set start [expr $start + $niter]
		eval rep_test $method $masterenv NULL $niter $start $start 0 $largs
		process_msgs $envlist
		error_check_good rep_sync [$dc1env rep_sync] 0
		error_check_good rep_sync [$dc3env rep_sync] 0
		if { $op == "syncing" } {
			#
			# Process messages twice to get us into syncing,
			# but not enough to complete it.  Then swap.
			#
			set nproced [proc_msgs_once $envlist NONE err]
			set nproced [proc_msgs_once $envlist NONE err]

			# Swap all the info we need.
			set tmp $masterenv
			set masterenv $clientenv
			set clientenv $tmp

			set tmp $mdir
			set mdir $cdir
			set cdir $mdir

			set tmp $mid
			set mid $cid
			set cid $tmp

			puts "\tRep$tnum.h: Swap master/client while syncing"
			error_check_good downgrade \
			    [$clientenv rep_start -client] 0
			error_check_good upgrade \
			    [$masterenv rep_start -master] 0
		}
		#
		# Now process all messages and verify.
		#
		puts "\tRep$tnum.i: Process all messages and verify."
		process_msgs $envlist

		#
		# If we swapped during the last syncing, we need to call
		# rep_sync again because the master changed again.
		#
		if { $op == "syncing" } {
			error_check_good rep_sync [$dc1env rep_sync] 0
			error_check_good rep_sync [$dc3env rep_sync] 0
			process_msgs $envlist
		}

		#
		# Delayed client should be the same now.
		#
		rep_verify $mdir $masterenv $delaycldir1 $dc1env 0 1 1
		rep_verify $mdir $masterenv $delaycldir3 $dc3env 0 1 1
		rep_verify $mdir $masterenv $delaycldir2 $dc2env 0 0 0
		error_check_good dc3_close [$dc3env close] 0
		env_cleanup $delaycldir3
		set envlist "{$env1 1} {$env2 2} {$dc1env 3} {$dc2env 4}"

	}
	puts "\tRep$tnum.j: Sync up 2nd delayed client and verify."
	error_check_good rep_sync [$dc2env rep_sync] 0
	process_msgs $envlist
	rep_verify $mdir $masterenv $delaycldir2 $dc2env 0 1 1

	puts "\tRep$tnum.k: Closing"
	error_check_good env1_close [$env1 close] 0
	error_check_good env2_close [$env2 close] 0
	error_check_good dc1_close [$dc1env close] 0
	error_check_good dc2_close [$dc2env close] 0
	replclose $testdir/MSGQUEUEDIR
	set testdir $orig_tdir
	return
}
