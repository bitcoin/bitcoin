# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST  rep001
# TEST	Replication rename and forced-upgrade test.
# TEST
# TEST	Run rep_test in a replicated master environment.
# TEST	Verify that the database on the client is correct.
# TEST	Next, remove the database, close the master, upgrade the
# TEST	client, reopen the master, and make sure the new master can
# TEST	correctly run rep_test and propagate it in the other direction.

proc rep001 { method { niter 1000 } { tnum "001" } args } {
	global passwd
	global has_crypto
	global databases_in_memory
	global repfiles_in_memory

	source ./include.tcl
	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win9x platform."
		return
	}

	if { $checking_valid_methods } {
		return "ALL"
	}

	# It's possible to run this test with in-memory databases.
	set msg "with named databases"
	if { $databases_in_memory } {
		set msg "with in-memory named databases"
		if { [is_queueext $method] == 1 } {
			puts "Skipping rep$tnum for method $method"
			return
		}
	}

	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	# Run tests with and without recovery.  If we're doing testing
	# of in-memory logging, skip the combination of recovery
	# and in-memory logging -- it doesn't make sense.
	set logsets [create_logsets 2]
	set saved_args $args

	foreach recopt $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $recopt == "-recover" && $logindex != -1 } {
				puts "Skipping test with -recover for in-memory logs."
				continue
			}
			set envargs ""
			set args $saved_args
			puts -nonewline "Rep$tnum: Replication sanity test "
			puts "($method $recopt) $msg $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep001_sub $method $niter $tnum $envargs $l $recopt $args

			# Skip encrypted tests if not supported.
			if { $has_crypto == 0 || $databases_in_memory } {
				continue
			}

			# Run the same tests with security.  In-memory
			# databases don't work with encryption.
			append envargs " -encryptaes $passwd "
			append args " -encrypt "
			puts "Rep$tnum: Replication and security sanity test\
			    ($method $recopt)."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep001_sub $method \
			    $niter $tnum $envargs $l $recopt $args
		}
	}
}

proc rep001_sub { method niter tnum envargs logset recargs largs } {
	source ./include.tcl
	global testdir
	global encrypt
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

	set verify_subset \
	    [expr { $m_logtype == "in-memory" || $c_logtype == "in-memory" }]

	# In-memory logs require a large log buffer, and cannot
	# be used with -txn nosync.  Adjust the args for master
	# and client.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Open a master.
	repladd 1
	set env_cmd(M) "berkdb_env_noerr -create $repmemargs \
	    -log_max 1000000 $envargs $m_logargs $recargs $verbargs \
	    -home $masterdir -errpfx MASTER $m_txnargs -rep_master \
	    -rep_transport \[list 1 replsend\]"
	set masterenv [eval $env_cmd(M)]

	# Open a client
	repladd 2
	set env_cmd(C) "berkdb_env_noerr -create $repmemargs \
	    -log_max 1000000 $envargs $c_logargs $recargs $verbargs \
	    -home $clientdir -errpfx CLIENT $c_txnargs -rep_client \
	    -rep_transport \[list 2 replsend\]"
	set clientenv [eval $env_cmd(C)]

	# Bring the client online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	# Clobber replication's 30-second anti-archive timer, which will have
	# been started by client sync-up internal init, so that we can do a
	# db_remove in a moment.
	#
	$masterenv test force noarchive_timeout

	# Run rep_test in the master (and update client).
	puts "\tRep$tnum.a:\
	    Running rep_test in replicated env ($envargs $recargs)."
	eval rep_test $method $masterenv NULL $niter 0 0 0 $largs
	process_msgs $envlist

	puts "\tRep$tnum.b: Verifying client database contents."
	if { $databases_in_memory } {
		set dbname { "" "test.db" }
	} else {
		set dbname "test.db"
	}

	rep_verify $masterdir $masterenv \
	    $clientdir $clientenv $verify_subset 1 1

	# Remove the file (and update client).
	puts "\tRep$tnum.c: Remove the file on the master and close master."
	error_check_good remove \
	    [eval {$masterenv dbremove} -auto_commit $dbname] 0
	error_check_good masterenv_close [$masterenv close] 0
	process_msgs $envlist

	puts "\tRep$tnum.d: Upgrade client."
	set newmasterenv $clientenv
	error_check_good upgrade_client [$newmasterenv rep_start -master] 0

	# Run rep_test in the new master
	puts "\tRep$tnum.e: Running rep_test in new master."
	eval rep_test $method $newmasterenv NULL $niter 0 0 0 $largs
	set envlist "{$newmasterenv 2}"
	process_msgs $envlist

	puts "\tRep$tnum.f: Reopen old master as client and catch up."
	# Throttle master so it can't send everything at once
	$newmasterenv rep_limit 0 [expr 64 * 1024]
	set newclientenv [eval {berkdb_env_noerr -create -recover} \
	    $envargs $m_logargs $m_txnargs -errpfx NEWCLIENT $verbargs $repmemargs \
	    {-home $masterdir -rep_client -rep_transport [list 1 replsend]}]
	set envlist "{$newclientenv 1} {$newmasterenv 2}"
	process_msgs $envlist

	# If we're running with a low number of iterations, we might
	# not have had to throttle the data transmission; skip the check.
	if { $niter > 200 } {
		set nthrottles \
		    [stat_field $newmasterenv rep_stat "Transmission limited"]
		error_check_bad nthrottles $nthrottles -1
		error_check_bad nthrottles $nthrottles 0
	}

	# Run a modified rep_test in the new master (and update client).
	puts "\tRep$tnum.g: Running rep_test in new master."
	eval rep_test $method \
	    $newmasterenv NULL $niter $niter $niter 0 $largs
	process_msgs $envlist

	# Verify the database in the client dir.
	puts "\tRep$tnum.h: Verifying new client database contents."

	rep_verify $masterdir $newmasterenv \
	    $clientdir $newclientenv $verify_subset 1 1

	error_check_good newmasterenv_close [$newmasterenv close] 0
	error_check_good newclientenv_close [$newclientenv close] 0

	if { [lsearch $envargs "-encrypta*"] !=-1 } {
		set encrypt 1
	}
	error_check_good verify \
	    [verify_dir $clientdir "\tRep$tnum.k: " 0 0 1] 0 

	replclose $testdir/MSGQUEUEDIR
}
