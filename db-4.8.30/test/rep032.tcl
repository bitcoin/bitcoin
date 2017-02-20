# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep032
# TEST	Test of log gap processing.
# TEST
# TEST	One master, one client.
# TEST	Run rep_test.
# TEST	Run rep_test without sending messages to client.
# TEST  Make sure client missing the messages catches up properly.
#
proc rep032 { method { niter 200 } { tnum "032" } args } {

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

	# Run the body of the test with and without recovery.
	set opts { "" "bulk" }
	foreach r $test_recopts {
		foreach b $opts {
			foreach l $logsets {
				set logindex [lsearch -exact $l "in-memory"]
				if { $r == "-recover" && $logindex != -1 } {
					puts "Rep$tnum: Skipping\
					    for in-memory logs with -recover."
					continue
				}
				puts "Rep$tnum ($method $r $b $args):\
				    Test of log gap processing $msg $msg2."
				puts "Rep$tnum: Master logs are [lindex $l 0]"
				puts "Rep$tnum: Client logs are [lindex $l 1]"
				rep032_sub $method $niter $tnum $l $r $b $args
			}
		}
	}
}

proc rep032_sub { method niter tnum logset recargs opts largs } {
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

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR

	file mkdir $masterdir
	file mkdir $clientdir

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]

	# In-memory logs require a large log buffer, and cannot
	# be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs $repmemargs \
	    $m_logargs $verbargs -home $masterdir -errpfx MASTER \
	    -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs -rep_master]
	if { $opts == "bulk" } {
		error_check_good bulk [$masterenv rep_config {bulk on}] 0
	}

	# Open a client
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs $repmemargs \
	    $c_logargs $verbargs -home $clientdir -errpfx CLIENT \
	     -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd $recargs -rep_client]

	# Bring the client online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	# Run rep_test in the master (and update client).
	puts "\tRep$tnum.a: Running rep_test in replicated env."
	set start 0
	eval rep_test $method $masterenv NULL $niter $start $start 0 $largs
	incr start $niter
	process_msgs $envlist

	puts "\tRep$tnum.b: Check client processed everything properly."
	set queued [stat_field $clientenv rep_stat "Maximum log records queued"]
	set request1 [stat_field $clientenv rep_stat "Log records requested"]
	error_check_good queued $queued 0

	# Run rep_test in the master (don't update client).
	# First run with dropping all client messages via replclear.
	puts "\tRep$tnum.c: Running rep_test dropping client msgs."
	eval rep_test $method $masterenv NULL $niter $start $start 0 $largs
	incr start $niter
	replclear 2
	process_msgs $envlist

	#
	# Need new operations to force log gap processing to
	# request missing pieces.
	#
	puts "\tRep$tnum.d: Running rep_test again replicated."
	#
	# Force a checkpoint to cause a gap to force rerequest.
	#
	$masterenv txn_checkpoint -force
	process_msgs $envlist
	tclsleep 1
	eval rep_test $method $masterenv NULL $niter $start $start 0 $largs
	incr start $niter
	process_msgs $envlist

	puts "\tRep$tnum.e: Check we re-requested and had a backlog."
	set queued [stat_field $clientenv rep_stat "Maximum log records queued"]
	set request2 [stat_field $clientenv rep_stat "Log records requested"]
	error_check_bad queued $queued 0
	error_check_bad request $request1 $request2

	puts "\tRep$tnum.f: Verify logs and databases"
	#
	# If doing bulk testing, turn it off now so that it forces us
	# to flush anything currently in the bulk buffer.  We need to
	# do this because rep_test might have aborted a transaction on
	# its last iteration and those log records would still be in
	# the bulk buffer causing the log comparison to fail.
	#
	if { $opts == "bulk" } {
		puts "\tRep$tnum.f.1: Turn off bulk transfers."
		error_check_good bulk [$masterenv rep_config {bulk off}] 0
		process_msgs $envlist 0 NONE err
	}

	rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 1

        set bulkxfer [stat_field $masterenv rep_stat "Bulk buffer transfers"]
	if { $opts == "bulk" } {
		error_check_bad bulkxferon $bulkxfer 0
	} else {
		error_check_good bulkxferoff $bulkxfer 0
	}

	check_log_location $masterenv
	check_log_location $clientenv

	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	replclose $testdir/MSGQUEUEDIR
}
