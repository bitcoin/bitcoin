# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST  rep025
# TEST  Test of DB_REP_JOIN_FAILURE.
# TEST
# TEST  One master, one client.
# TEST  Generate several log files.
# TEST  Remove old master log files.
# TEST  Delete client files and restart client.
# TEST  Put one more record to the master.  At the next
# TEST  processing of messages, the client should get JOIN_FAILURE.
# TEST  Recover with a hot failover.
#
proc rep025 { method { niter 200 } { tnum "025" } args } {
	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory

	# Run for all access methods.
	if { $checking_valid_methods } {
		return "ALL"
	}

	set args [convert_args $method $args]

	# This test needs to set its own pagesize.
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Rep$tnum: skipping for specific pagesizes"
		return
	}

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

	# Run the body of the test with and without recovery,
	# and with and without cleaning.  Skip recovery with in-memory
	# logging - it doesn't make sense.
	foreach r $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Skipping rep$tnum for -recover\
				    with in-memory logs."
				continue
			}
			puts "Rep$tnum ($method $r): Test of manual\
			    initialization and join failure $msg $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep025_sub $method $niter $tnum $l $r $args
		}
	}
}

proc rep025_sub { method niter tnum logset recargs largs } {
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

	# Log size is small so we quickly create more than one.
	# The documentation says that the log file must be at least
	# four times the size of the in-memory log buffer.
	set pagesize 4096
	append largs " -pagesize $pagesize "
	set log_max [expr $pagesize * 8]

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]

	# In-memory logs cannot be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs $repmemargs \
	    $m_logargs -log_max $log_max $verbargs -errpfx MASTER \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs -rep_master]

	# Open a client
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs $repmemargs \
	    $c_logargs -log_max $log_max $verbargs -errpfx CLIENT \
	    -home $clientdir -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd $recargs -rep_client]

	# Bring the clients online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	# Clobber replication's 30-second anti-archive timer, which will have
	# been started by client sync-up internal init, so that we can do a
	# log_archive in a moment.
	#
	$masterenv test force noarchive_timeout

	# Run a modified test001 in the master (and update client).
	puts "\tRep$tnum.a: Running rep_test in replicated env."
	set start 0
	eval rep_test $method $masterenv NULL $niter $start $start 0 $largs
	incr start $niter
	process_msgs $envlist

	# Find out what exists on the client.  We need to loop until
	# the first master log file > last client log file.
	puts "\tRep$tnum.b: Close client."
	if { $c_logtype != "in-memory" } {
		set res [eval exec $util_path/db_archive -l -h $clientdir]
	}
	set last_client_log [get_logfile $clientenv last]
	error_check_good client_close [$clientenv close] 0

	set stop 0
	while { $stop == 0 } {
		# Run rep_test in the master (don't update client).
		puts "\tRep$tnum.c: Running rep_test in replicated env."
	 	eval rep_test \
		    $method $masterenv NULL $niter $start $start 0 $largs
		incr start $niter
		replclear 2

		puts "\tRep$tnum.d: Run db_archive on master."
		if { $m_logtype != "in-memory"} {
			set res [eval exec $util_path/db_archive -d -h $masterdir]
		}
		set first_master_log [get_logfile $masterenv first]
		if { $first_master_log > $last_client_log } {
			set stop 1
		}
	}

	puts "\tRep$tnum.e: Clean client and reopen."
	env_cleanup $clientdir
	set clientenv [eval $cl_envcmd $recargs -rep_client]
	error_check_good client_env [is_valid_env $clientenv] TRUE
	set envlist "{$masterenv 1} {$clientenv 2}"

	# Set initialization to manual.
	$clientenv rep_config {noautoinit on}
	process_msgs $envlist 0 NONE err
	error_check_good error_on_right_env [lindex $err 0] $clientenv
	error_check_good right_error [is_substr $err DB_REP_JOIN_FAILURE] 1

	# Add records to the master and update client.
	puts "\tRep$tnum.f: Update master; client should return error."
	#
	# Force a log record to create a gap to force rerequest.
	#
	$masterenv txn_checkpoint -force
	process_msgs $envlist 0 NONE err
	tclsleep 1
	set entries 100
	eval rep_test $method $masterenv NULL $entries $start $start 0 $largs
	incr start $entries
	process_msgs $envlist 0 NONE err
	error_check_good error_on_right_env [lindex $err 0] $clientenv
	error_check_good right_error [is_substr $err DB_REP_JOIN_FAILURE] 1

	# If the master logs and the databases are on-disk, copy from master 
	# to client and restart with recovery.  If the logs or databases are 
	# in-memory, we'll have to re-enable internal initialization and 
	# restart the client.
	if { $m_logtype == "on-disk" && $databases_in_memory == 0 } {
		puts "\tRep$tnum.g: Hot failover and catastrophic recovery."
		error_check_good client_close [$clientenv close] 0
		env_cleanup $clientdir
		set files [glob $masterdir/log.* $masterdir/*.db]
		foreach f $files {
			set filename [file tail $f]
			file copy -force $f $clientdir/$filename
		}
		set clientenv [eval $cl_envcmd -recover_fatal -rep_client]
	} else {
		puts "\tRep$tnum.g: Restart client forcing internal init."
		set clientenv [eval $cl_envcmd -rep_client]
		$clientenv rep_config {noautoinit off}
	}
	error_check_good client_env [is_valid_env $clientenv] TRUE
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist 0 NONE err
	error_check_good no_errors1 $err 0

	# Adding another entry should not flush out an error.
	eval rep_test $method $masterenv NULL $entries $start $start 0 $largs
	process_msgs $envlist 0 NONE err
	error_check_good no_errors2 $err 0

	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	replclose $testdir/MSGQUEUEDIR
}
