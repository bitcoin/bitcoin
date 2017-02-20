# See the file LICENSE for redistribution information.
#
# Copyright (c) 2007-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep072
# TEST  Verify that internal init does not leak resources from
# TEST  the locking subsystem.

proc rep072 { method { niter 200 } { tnum "072" } args } {
	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win9x platform."
		return
	}

	# Run for btree and queue methods only.
	if { $checking_valid_methods } {
		set test_methods {}
		foreach method $valid_methods {
			if { [is_btree $method] == 1 || \
			    [is_queue $method] == 1 } {
				lappend test_methods $method
			}
		}
		return $test_methods
	}
	if { [is_btree $method] == 0 && [is_queue $method] == 0 } {
		puts "Rep$tnum: skipping for non-btree, non-queue method."
		return
	}

	set args [convert_args $method $args]
	set limit 3
	set check true

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
		puts "Rep$tnum ($method): Confirm internal init does not\
		    leak locks $msg $msg2."
		puts "Rep$tnum: Master logs are [lindex $l 0]"
		puts "Rep$tnum: Client logs are [lindex $l 1]"
		rep072_sub $method $niter $tnum $l $limit $check $args
	}
}

proc rep072_sub {method {niter 200} {tnum 072} logset \
    {limit 3} {check true} largs} {
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
	# be used with -txn nosync.  Adjust the args for master
	# and client.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Log size is small so we quickly create more than one.
	# The documentation says that the log file must be at least
	# four times the size of the in-memory log buffer.
	set pagesize 4096
	append largs " -pagesize $pagesize "
	set log_max [expr $pagesize * 8]

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $verbargs $repmemargs \
	    $m_logargs $m_txnargs -log_max $log_max -errpfx MASTER \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd -rep_master]
	$masterenv rep_limit 0 0

	# Open a client
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $verbargs $repmemargs \
	    $c_logargs $c_txnargs -log_max $log_max -errpfx CLIENT \
	    -home $clientdir -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd -rep_client]
	$clientenv rep_limit 0 0

	# Bring the client online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	# Clobber replication's 30-second anti-archive timer, which will have
	# been started by client sync-up internal init, so that we can do a
	# log_archive in a moment.
	#
	$masterenv test force noarchive_timeout

	# $limit is the number of internal init cycles we want to try
	for {set count 1} {$count <= $limit} {incr count} {
		puts "\tRep$tnum.a: Try internal init cycle number $count"

		# Run rep_test in the master.
		puts "\tRep$tnum.b: Running rep_test in replicated env."
		eval rep_test $method $masterenv NULL $niter 0 0 0 $largs
		process_msgs $envlist

		puts "\tRep$tnum.c: Leave client alive, but isolated."

		if { $c_logtype != "in-memory" } {
			set res [exec $util_path/db_archive -l -h $clientdir]
		}
		set last_client_log [get_logfile $clientenv last]

		set stop 0
		while { $stop == 0 } {
			# Run rep_test in the master (don't update client).
			puts "\tRep$tnum.d: Running rep_test in replicated env."
			eval rep_test \
			    $method $masterenv NULL $niter 0 0 0 $largs
			#
			# Clear messages for client.  We want that site
			# to get far behind.
			#
			replclear 2
			if { $m_logtype != "in-memory" } {
				puts "\tRep$tnum.e: Run db_archive on master."
				exec $util_path/db_archive -d -h $masterdir
				set res [exec $util_path/db_archive -l \
				    -h $masterdir]
			}
			set first_master_log [get_logfile $masterenv first]
			if { $first_master_log > $last_client_log } {
				set stop 1
			}
		}

		#
		# Run rep_test one more time, this time letting client see
		# messages.  This will induce client to ask master for missing
		# log records, leading to internal init.
		#
		puts "\tRep$tnum.f: Running rep_test in replicated env."
		set entries 10
		eval rep_test $method \
		    $masterenv NULL $entries $niter 0 0 $largs
		process_msgs $envlist

		set n_lockers [stat_field \
		    $clientenv lock_stat "Current number of lockers"]
		puts "\tRep$tnum.f: num lockers: $n_lockers"
		if {$count == 1} {
			set expected_lockers $n_lockers
		} elseif {[string is true $check]} {
			error_check_good leaking? $n_lockers $expected_lockers
		}

		if {$count < $limit} {
			# Wait for replication "no-archive" timeout to expire
			#
			puts "\tRep$tnum.g: Sleep for 32 seconds"
			tclsleep 32
		}
	}

	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	replclose $testdir/MSGQUEUEDIR
}
