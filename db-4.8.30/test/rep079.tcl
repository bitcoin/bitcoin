# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST  rep079
# TEST	Replication leases and invalid usage.
# TEST	
# TEST	Open a client without leases.  Attempt to set leases after rep_start.
# TEST	Attempt to declare as master without election.
# TEST	Run an election with an nsites parameter value.
# TEST	Elect a master with leases.  Put some data and send to clients.
# TEST	Cleanly shutdown master env.  Restart without
# TEST	recovery and verify leases are expired and refreshed.
# TEST	Add a new client without leases to a group using leases.
#
proc rep079 { method { tnum "079" } args } {
	source ./include.tcl
	global repfiles_in_memory

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win9x platform."
		return
	}

	# Valid for all access methods, but there is no difference
	# running it with one method over any other.  Just use btree.
	if { $checking_valid_methods } {
		set test_methods { btree }
		return $test_methods
	}
	if { [is_btree $method] == 0 } {
		puts "Rep$tnum: Skipping for method $method."
		return 
	}
	
	set args [convert_args $method $args]
	set logsets [create_logsets 4]

	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	foreach l $logsets {
		#
        	# Skip the case where the master is in-memory and at least
        	# one of the clients is on-disk.  If the master is in-memory,
        	# the wrong site gets elected because on-disk envs write a log
        	# record when they create the env and in-memory ones do not
		# and the test wants to control which env gets elected.
		#
		set master_logs [lindex $l 0]
		if { $master_logs == "in-memory" } {
			set client_logs [lsearch -exact $l "on-disk"]
			if { $client_logs != -1 } {
				puts "Skipping for in-memory master\
				    with on-disk client."
				continue
			}
		}
		puts "Rep$tnum: Replication leases and invalid usage $msg2."
		puts "Rep$tnum: Master logs are [lindex $l 0]"
		puts "Rep$tnum: Client logs are [lindex $l 1]"
		puts "Rep$tnum: Client 2 logs are [lindex $l 2]"
		puts "Rep$tnum: Client 3 logs are [lindex $l 3]"
		rep079_sub $method $tnum $l $args
	}
}

proc rep079_sub { method tnum logset largs } {
	global testdir
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

	set qdir $testdir/MSGQUEUEDIR
	replsetup $qdir

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR2
	set clientdir3 $testdir/CLIENTDIR3

	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $clientdir2
	file mkdir $clientdir3

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]
	set c2_logtype [lindex $logset 2]
	set c3_logtype [lindex $logset 3]

	# In-memory logs cannot be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set c2_logargs [adjust_logargs $c2_logtype]
	set c3_logargs [adjust_logargs $c3_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]
	set c2_txnargs [adjust_txnargs $c2_logtype]
	set c3_txnargs [adjust_txnargs $c3_logtype]

	# Set leases for 4 sites, 1 second timeout, 1% clock skew
	# [NOTE: We are not adding in client3 until later so don't
	# set it in nvotes.]
	set nsites 4
	set nvotes 3
	set lease_to 1000000
	set lease_tosec [expr $lease_to / 1000000]
	set clock_fast 101
	set clock_slow 100

	repladd 2
	#
	# Use a command without setting errpfx, errfile or verbose
	# so that error messages can be caught correctly.
	#
	set envcmd_err "berkdb_env_noerr -create $m_txnargs $m_logargs \
	    $repmemargs -home $masterdir -rep_transport \[list 2 replsend\]"

	#
	# This is the real env command, but we won't use it
	# quite yet.
	set envcmd(0) "berkdb_env -create $m_txnargs $m_logargs \
	    $repmemargs $verbargs -errpfx MASTER -home $masterdir \
	    -event rep_event \
	    -rep_transport \[list 2 replsend\]"

	#
	# Leases must be configured before rep_start is called.
	# Open a repl env without leases.  Try to configure leases
	# after the open has already called rep_start.  Open as a client.
	#
	puts "\tRep$tnum.a: Try to configure leases after rep_start."
	set noleaseenv [eval $envcmd_err -rep_client]
	set stat [catch {$noleaseenv rep_lease \
	    [list $nsites $lease_to $clock_fast $clock_slow]} lease]
	error_check_bad stat $stat 0
	error_check_good menverror [is_substr $lease "timeout must be set"] 1
	error_check_good close [$noleaseenv close] 0
	env_cleanup $masterdir

	#
	# If leases are being used, elections must be used.  A site
	# cannot simply upgrade itself to master.  Test that we cannot
	# open as a client and then upgrade ourself to a master just
	# by calling rep_start.
	#
	set upgenv [eval $envcmd_err -rep_client \
	    -rep_lease \[list $nsites $lease_to $clock_fast $clock_slow\]]
	puts "\tRep$tnum.b: Try to upgrade a client without election."
        set stat [catch {$upgenv rep_start -master} ret]
	error_check_bad upg_stat $stat 0
	error_check_good upg_str [is_substr $ret "Cannot become master"] 1
	error_check_good close [$upgenv close] 0
	env_cleanup $masterdir

	#
	# Now test inconsistencies dealing with having a group that
	# is using lease up and running.  For instance, if leases are
	# configured, the 'nsites' arg to rep_elect must be 0, etc.
	#
	# Open the master.  Must open as a client and get elected.
	#
	set err_cmd(0) "none"
	set crash(0) 0
	set pri(0) 100
	set masterenv [eval $envcmd(0) -rep_client \
	    -rep_lease \[list $nsites $lease_to $clock_fast $clock_slow\]]
	error_check_good master_env [is_valid_env $masterenv] TRUE

	# Open two clients.
	repladd 3
	set err_cmd(1) "none"
	set crash(1) 0
	set pri(1) 10
	set envcmd(1) "berkdb_env -create $c_txnargs $c_logargs \
	    $repmemargs $verbargs -errpfx CLIENT -home $clientdir \
	    -event rep_event \
	    -rep_lease \[list $nsites $lease_to $clock_fast $clock_slow\] \
	    -rep_client -rep_transport \[list 3 replsend\]"
	set clientenv [eval $envcmd(1)]
	error_check_good client_env [is_valid_env $clientenv] TRUE

	repladd 4
	set err_cmd(2) "none"
	set crash(2) 0
	set pri(2) 10
	set envcmd(2) "berkdb_env_noerr -create $c2_txnargs $c2_logargs \
	    $repmemargs -home $clientdir2 -event rep_event \
	    -rep_lease \[list $nsites $lease_to $clock_fast $clock_slow\] \
	    -rep_client -rep_transport \[list 4 replsend\]"
	set clientenv2 [eval $envcmd(2)]
	error_check_good client_env [is_valid_env $clientenv2] TRUE

	# Bring the clients online by processing the startup messages.
	set envlist "{$masterenv 2} {$clientenv 3} {$clientenv2 4}"
	process_msgs $envlist

	#
	# Send a non-zero nsites value for an election.  That is an error.
	#
	puts "\tRep$tnum.c: Try to run election with leases and nsites value."
	#
	# !!! We have not set -errpfx or -errfile in envcmd(2) above
	# otherwise the error output won't be set in 'ret' below and
	# the test will fail.  Set it after this piece of the test.
	#
	set timeout 5000000
	set res [catch {$clientenv2 rep_elect $nsites $nvotes $pri(2) \
	    $timeout} ret]
	error_check_bad catch $res 0
	error_check_good ret [is_substr $ret "nsites must be zero"] 1

	#
	# Now we can set verbose args, errpfx, etc.  Set it in the
	# command (for elections) and also manually add it to the
	# current env handle.
	#
	set envcmd(2) "$envcmd(2) $verbargs -errpfx CLIENT2"
	if { $rep_verbose == 1 } {
		$clientenv2 verbose $verbose_type on
		$clientenv2 errpfx CLIENT2
	}

	#
	# This next section will test that a replicated env that is master
	# can cleanly close and then reopen without recovery and retain
	# its master status.
	#
	set msg "Rep$tnum.d"
	set nvotes [expr $nsites - 1]
	set winner 0
	setpriority pri $nsites $winner
	set elector [berkdb random_int 0 2]
	puts "\tRep$tnum.d: Run election for real to get master."
	#
	# Run election for real.  Set nsites to 0 for this command.
	#
	repladd 5
	set err_cmd(3) "none"
	set crash(3) 0
	set pri(3) 0
	run_election envcmd envlist err_cmd pri crash $qdir $msg \
	    $elector 0 $nvotes $nsites $winner 0 NULL

	puts "\tRep$tnum.e: Write a checkpoint."
	#
	# Writing a checkpoint forces a PERM record which will cause
	# the clients to grant us their leases.  Then, while holding
	# the lease grants we can do the next part of the test to
	# close and cleanly reopen while holding leases.
	$masterenv txn_checkpoint -force

	process_msgs $envlist

	puts "\tRep$tnum.f.0: Close master env."
	error_check_good mclose [$masterenv close] 0
	set sleep [expr $lease_tosec + 1]
	puts "\tRep$tnum.f.1: Sleep $sleep secs to expire lease grants."
	tclsleep $sleep
	#
	# We should be able to reopen the master env without running
	# recovery and still retain our mastership. 
	set masterenv [eval $envcmd(0) -rep_master \
	    -rep_lease \[list $nsites $lease_to $clock_fast $clock_slow\]]
	error_check_good master_env [is_valid_env $masterenv] TRUE
	set envlist "{$masterenv 2} {$clientenv 3} {$clientenv2 4}"

	#
	# Verify that if a non-lease site tries to join a group that
	# is using leases, it gets an error.  Configuring leases
	# must be all-or-none across all group members.
	#
	puts "\tRep$tnum.g: Add client3 that does not configure leases."
	replclear 5
	set envcmd(3) "berkdb_env_noerr -create $c3_txnargs $c3_logargs \
	    -home $clientdir3 -event rep_event \
	    $repmemargs $verbargs -errpfx CLIENT3 \
	    -rep_client -rep_transport \[list 5 replsend\]"
	set clientenv3 [eval $envcmd(3)]
	error_check_good client_env [is_valid_env $clientenv3] TRUE

	# Bring the clients online by processing the startup messages.
	set origlist $envlist
	set envlist "{$masterenv 2} {$clientenv 3} \
	    {$clientenv2 4} {$clientenv3 5}"
	process_msgs $envlist 0 NONE err

	puts "\tRep$tnum.g.1: Verify client fatal error."
	error_check_good process_msgs_err [is_substr $err DB_RUNRECOVERY] 1
	#
	# Close to reclaim Tcl resources, but we want to catch/ignore
	# the continuing DB_RUNRECOVERY error.
	#
	catch {$clientenv3 close} ret
	set envlist $origlist

	puts "\tRep$tnum.h: Check expired lease error on txn commit."
	#
	# Leases are already expired, so attempt to commit should fail.  
	# (And this will be the 'before we commit' check that returns
	# an error, not the 'after' check that panics).
	#
	set txn [$masterenv txn]
	set stat [catch {$txn commit} ret]
	error_check_good stat $stat 1
	error_check_good exp [is_substr $ret REP_LEASE_EXPIRED] 1

	error_check_good mclose [$masterenv close] 0
	error_check_good cclose [$clientenv close] 0
	error_check_good c2close [$clientenv2 close] 0
	replclose $testdir/MSGQUEUEDIR
}
