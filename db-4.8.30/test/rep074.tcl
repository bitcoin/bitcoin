# See the file LICENSE for redistribution information.
#
# Copyright (c) 2007-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep074
# TEST	Verify replication withstands send errors processing requests.
# TEST
# TEST	Run for btree only because access method shouldn't matter.
# TEST
proc rep074 { method { niter 20 } { tnum "074" } args } {

	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win9x platform."
		return
	}

	# Skip for all methods except btree.
	if { $checking_valid_methods } {
		return btree
	}
	if { [is_btree $method] == 0 } {
		puts "Rep$tnum: skipping for non-btree method $method."
		return
	}

	set args [convert_args $method $args]
	set logsets [create_logsets 2]

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

	foreach l $logsets {
		puts "Rep$tnum ($method): Test of send errors processing\
		    requests $msg $msg2."
		puts "Rep$tnum: Master logs are [lindex $l 0]"
		puts "Rep$tnum: Client logs are [lindex $l 1]"
		rep074_sub $method $niter $tnum $l $args
	}
}

proc rep074_sub { method niter tnum logset largs } {
	global testdir
	global rep074_failure_count
	global repfiles_in_memory
	global rep_verbose
	global verbose_type

	set rep074_failure_count -1

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

	# In-memory logs require a large log buffer, and cannot
	# be used with -txn nosync.  Adjust the args for master
	# and client.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $verbargs -errpfx MASTER \
	    -home $masterdir $m_logargs $m_txnargs $repmemargs \
	    -rep_transport \[list 1 rep074_replsend\]"
	set masterenv [eval $ma_envcmd -rep_master]

	# Create some new records, so that the master will have something
	# substantial to say when asked for LOG_REQ.
	#
	puts "\tRep$tnum.a: Running rep_test in replicated env."
	eval rep_test $method $masterenv NULL $niter 0 0 0 $largs

	# Open a client
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $verbargs -errpfx CLIENT \
	    -home $clientdir $c_logargs $c_txnargs $repmemargs \
	    -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd -rep_client]
	set envlist "{$masterenv 1} {$clientenv 2}"

	# Bring the client online by processing the startup messages.  This will
	# cause the client to send a request to the master.
	#
	# In the first cycle, the client gets NEWMASTER and sends an UPDATE_REQ.
	# In the second cycle, the master answers the UPDATE_REQ with an UPDATE,
	# and the client sends a PAGE_REQ.  Third, once we've gotten pages, we
	# send a LOG_REQ.
	#
	# 1. NEWCLIENT -> NEWMASTER -> UPDATE_REQ
	# 2.              UPDATE -> PAGE_REQ
	# 3.              PAGE -> LOG_REQ
	#
	puts "\tRep$tnum.b: NEWMASTER -> UPDATE_REQ"
	proc_msgs_once $envlist
	puts "\tRep$tnum.c: UPDATE -> PAGE_REQ"
	proc_msgs_once $envlist
	puts "\tRep$tnum.d: PAGE -> LOG_REQ"
	proc_msgs_once $envlist

	# Force a sending error at the master while processing the LOG_REQ.
	# We should ignore it, and return success to rep_process_message
	#
	puts "\tRep$tnum.e: Simulate a send error."
	set rep074_failure_count [expr $niter / 2]
	proc_msgs_once $envlist NONE errorp

	puts "\tRep$tnum.f: Check for good return from rep_process_msg."
	error_check_good rep_resilient $errorp 0

	# Since we interrupted the flow with the simulated error, we don't have
	# the log records we need yet.
	#
	error_check_bad startupdone \
	    [stat_field $clientenv rep_stat "Startup complete"] 1

	#
	# Run some more new txns at the master, so that the client eventually
	# decides to request the remainder of the LOG_REQ response that it's
	# missing.  Pause for a second to make sure we reach the lower 
	# threshold for re-request on fast machines.  We need to force a
	# checkpoint because we need to create a gap, and then pause to
	# reach the rerequest threshold.
	#
	set rep074_failure_count -1
	$masterenv txn_checkpoint -force
	process_msgs $envlist
	tclsleep 1
	eval rep_test $method $masterenv NULL $niter 0 0 0 $largs
	process_msgs $envlist

	error_check_good startupdone \
	    [stat_field $clientenv rep_stat "Startup complete"] 1

	$masterenv close
	$clientenv close
	replclose $testdir/MSGQUEUEDIR
}

# Failure count < 0 turns off any special failure simulation processing.
# When the count is > 0, it means we should process that many messages normally,
# before invoking a failure.
#
proc rep074_replsend { control rec fromid toid flags lsn } {
	global rep074_failure_count

	if { $rep074_failure_count < 0 } {
		return [replsend $control $rec $fromid $toid $flags $lsn]
	}

	if { $rep074_failure_count > 0 } {
		incr rep074_failure_count -1
		return [replsend $control $rec $fromid $toid $flags $lsn]
	}

	# Return an arbitrary non-zero value to indicate an error.
	return 1
}
