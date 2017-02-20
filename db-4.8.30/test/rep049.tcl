# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep049
# TEST	Replication and delay syncing clients - basic test.
# TEST
# TEST	Open and start up a master and two clients.  Turn on delay sync
# TEST	in the delayed client.  Change master, add data and process messages.
# TEST	Verify delayed client does not match.  Make additional changes and
# TEST	update the delayted client.  Verify all match.
# TEST  Add in a fresh delayed client to test delay of ALL_REQ.
# TEST	Process startup messages and verify freshc client has no database.
# TEST	Sync and verify fresh client matches.
#
proc rep049 { method { niter 10 } { tnum "049" } args } {
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
	set logsets [create_logsets 4]

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
			puts "Rep$tnum: Swap Client logs are [lindex $l 1]"
			puts "Rep$tnum: Delay Client logs are [lindex $l 2]"
			puts "Rep$tnum: Fresh Client logs are [lindex $l 3]"
			rep049_sub $method $niter $tnum $l $r $args
		}
	}
}

proc rep049_sub { method niter tnum logset recargs largs } {
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
	set delaycldir $testdir/DELAYCLDIR
	set freshcldir $testdir/FRESHCLDIR
	file mkdir $env1dir
	file mkdir $env2dir
	file mkdir $delaycldir
	file mkdir $freshcldir

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]
	set dc_logtype [lindex $logset 2]
	set fc_logtype [lindex $logset 3]

	# In-memory logs require a large log buffer, and cannot
	# be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set dc_logargs [adjust_logargs $dc_logtype]
	set fc_logargs [adjust_logargs $fc_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]
	set dc_txnargs [adjust_txnargs $dc_logtype]
	set fc_txnargs [adjust_txnargs $fc_logtype]

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs $verbargs \
	    $repmemargs \
	    $m_logargs -errpfx ENV1 -cachesize {0 4194304 3} \
	    -home $env1dir -rep_transport \[list 1 replsend\]"
	set env1 [eval $ma_envcmd $recargs -rep_master]
	error_check_good master_env [is_valid_env $env1] TRUE
	$env1 rep_limit 0 0

	# Open two clients
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs $verbargs \
	    $repmemargs $c_logargs -errpfx ENV2 \
	    -home $env2dir -rep_transport \[list 2 replsend\]"
	set env2 [eval $cl_envcmd $recargs -rep_client]
	error_check_good client_env [is_valid_env $env2] TRUE
	$env2 rep_limit 0 0

	repladd 3
	set dc_envcmd "berkdb_env_noerr -create $dc_txnargs \
	    $repmemargs $verbargs $dc_logargs -errpfx ENV3 \
	    -home $delaycldir -rep_transport \[list 3 replsend\]"
	set dcenv [eval $dc_envcmd $recargs -rep_client]
	error_check_good client2_env [is_valid_env $dcenv] TRUE
	$dcenv rep_limit 0 0

	#
	# !!!
	# We're not using this client yet, but put its command up here.
	# We'll do the repladd and execute this env command later.
	#
	set fc_envcmd "berkdb_env_noerr -create $fc_txnargs \
	    $repmemargs $fc_logargs -errpfx ENV4 $verbargs \
	    -home $freshcldir -rep_transport \[list 4 replsend\]"

	# Bring the clients online by processing the startup messages.
	set envlist "{$env1 1} {$env2 2} {$dcenv 3}"
	process_msgs $envlist

	puts "\tRep$tnum.a: Run rep_test in master env."
	set start 0
	eval rep_test $method $env1 NULL $niter $start $start 0 $largs

	process_msgs $envlist

	puts "\tRep$tnum.b: Set delayed sync on client.  Basic test."
	error_check_good set_delay [$dcenv rep_config {delayclient on}] 0
	#
	# Call sync when we're not delayed.  Verify it just returns and
	# that no messages are generated anywhere.
	#
	error_check_good sync1 [$dcenv rep_sync] 0
	set nproced [proc_msgs_once $envlist NONE err]
	error_check_good nproced $nproced 0

	puts "\tRep$tnum.c: Swap master/client"
	error_check_good downgrade [$env1 rep_start -client] 0
	error_check_good upgrade [$env2 rep_start -master] 0

	process_msgs $envlist

	puts "\tRep$tnum.d: Run rep_test in new master env"
	set start $niter
	eval rep_test $method $env2 NULL $niter $start $start 0 $largs
	process_msgs $envlist
	#
	# Delayed client should be different. Former master should be synced.
	#
	rep_verify $env2dir $env2 $env1dir $env1 1 1 1
	rep_verify $env2dir $env2 $delaycldir $dcenv 0 0 0

	puts "\tRep$tnum.e: Sync delayed client"
	error_check_good rep_sync [$dcenv rep_sync] 0
	process_msgs $envlist
	#
	# Delayed client should be the same now.
	#
	rep_verify $env2dir $env2 $delaycldir $dcenv 0 1 1

	puts "\tRep$tnum.f: Run rep_test after sync-up in new master env"
	set start [expr $start + $niter]
	eval rep_test $method $env2 NULL $niter $start $start 0 $largs
	process_msgs $envlist
	#
	# Delayed client be caught up and running fine.
	#
	rep_verify $env2dir $env2 $delaycldir $dcenv 0 1 1

	puts "\tRep$tnum.g: Add in a fresh delayed client"
	repladd 4
	set fcenv [eval $fc_envcmd $recargs -rep_client]
	error_check_good client3_env [is_valid_env $fcenv] TRUE
	error_check_good set_delay [$fcenv rep_config {delayclient on}] 0

	set envlist "{$env1 1} {$env2 2} {$dcenv 3} {$fcenv 4}"
	process_msgs $envlist

	# Verify that after processing the startup messages, the
	# new client has no database and unmatched logs.
	set dbname "test.db"
	error_check_bad clientdb [file exists $freshcldir/$dbname] 1
	rep_verify $env2dir $env2 $freshcldir $fcenv 0 0 1 NULL

	puts "\tRep$tnum.h: Sync delayed client"
	error_check_good rep_sync [$fcenv rep_sync] 0
	process_msgs $envlist
	#
	# Delayed client should be the same now.
	#
	rep_verify $env2dir $env2 $freshcldir $fcenv 0 1 1

	puts "\tRep$tnum.i: Closing"
	error_check_good env1_close [$env1 close] 0
	error_check_good env2_close [$env2 close] 0
	error_check_good dc_close [$dcenv close] 0
	error_check_good fc_close [$fcenv close] 0
	replclose $testdir/MSGQUEUEDIR
	set testdir $orig_tdir
	return
}
