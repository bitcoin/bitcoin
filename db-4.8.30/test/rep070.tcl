# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep070
# TEST	Test of startup_done condition with idle master.
# TEST
# TEST	Join a client to an existing master, and verify that
# TEST	the client detects startup_done even if the master
# TEST	does not execute any new transactions.
#
proc rep070 { method { niter 200 } { tnum "070" } args } {

	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory

	# Run for btree and queue only.
	if { $checking_valid_methods } {
		set test_methods {}
		foreach method $valid_methods {
			if { [is_btree $method] == 1 ||\
			    [is_queue $method] == 1 } {
				lappend test_methods $method
			}
		}
		return $test_methods
	}
	if { [is_btree $method] != 1 && [is_queue $method] != 1 } {
		puts "Skipping rep070 for method $method."
		return
	}

	# This test does not cover any new ground with in-memory 
	# databases.
	if { $databases_in_memory } {
		puts "Skipping Rep$tnum for in-memory databases."
		return 
	}

	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	set args [convert_args $method $args]
	set logsets [create_logsets 2]

	# Run the body of the test with and without recovery,
	# and with and without cleaning.  Skip recovery with in-memory
	# logging - it doesn't make sense.
	#
	foreach r $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Skipping rep$tnum for -recover\
				    with in-memory logs."
				continue
			}
			puts "Rep$tnum ($method $r $args): Test of\
			    internal initialization and startup_done $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep070_sub $method $niter $tnum $l $r $args
		}
	}
}

proc rep070_sub { method niter tnum logset recargs largs } {
	source ./include.tcl
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

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR

	file mkdir $masterdir
	file mkdir $clientdir

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]

	# In-memory logs cannot be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs \
	    $m_logargs $verbargs -errpfx MASTER $repmemargs \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs -rep_master]

	# Put some data into the database
	puts "\tRep$tnum.a: Run rep_test in master env."
	set start 0
	eval rep_test $method $masterenv NULL $niter $start $start 0 $largs

	# Open a client
	puts "\tRep$tnum.b: Open client."
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs \
	    $c_logargs $verbargs -errpfx CLIENT $repmemargs \
	    -home $clientdir -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd $recargs -rep_client]

	set envlist "{$masterenv 1} {$clientenv 2}"
	rep070_verify_startup_done $clientenv $envlist

	# Close and re-open the client.  What happens next depends on whether
	# we used -recover.
	#
	$clientenv close
	set clientenv [eval $cl_envcmd $recargs -rep_client]
	set envlist "{$masterenv 1} {$clientenv 2}"
	if { $recargs == "-recover" } {
		rep070_verify_startup_done $clientenv $envlist
	} else {
		error_check_good \
		    startup_still_done [rep070_startup_done $clientenv] 1
	}

	rep_verify $masterdir $masterenv $clientdir $clientenv 1

	check_log_location $masterenv
	check_log_location $clientenv

	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	replclose $testdir/MSGQUEUEDIR
}

# Verify that startup_done starts off false, then turns to true at some point,
# and thereafter never reverts to false.
#
proc rep070_verify_startup_done { clientenv envlist } {
	# Initially we should not yet have startup_done.
	set got_startup_done [rep070_startup_done $clientenv]
	error_check_good startup_not_done_yet $got_startup_done 0

	# Bring the client online little by little.
	#
	while { [proc_msgs_once $envlist] > 0 } {
		set done [rep070_startup_done $clientenv]

		# At some point, startup_done gets turned on.  Make sure it
		# never gets turned off after that.
		#
		if { $got_startup_done } {
			# We've seen startup_done previously.
			error_check_good no_rescind $done 1
		} else {
			set got_startup_done $done
		}
	}
	error_check_good startup_done $got_startup_done 1
}

proc rep070_startup_done { env } {
	stat_field $env rep_stat "Startup complete"
}
