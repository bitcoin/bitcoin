# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST  rep022
# TEST	Replication elections - test election generation numbers
# TEST	during simulated network partition.
# TEST
proc rep022 { method args } {

	source ./include.tcl
	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}
	global rand_init
	global mixed_mode_logging
	global databases_in_memory
	global repfiles_in_memory

	set tnum "022"

	# Run for btree only.
	if { $checking_valid_methods } {
		set test_methods { btree }
		return $test_methods
	}
	if { [is_btree $method] == 0 } {
		puts "Rep$tnum: Skipping for method $method."
		return
	}

	if { $mixed_mode_logging > 0 } {
		puts "Rep$tnum: Skipping for mixed-mode logging."
		return
	}

	if { $databases_in_memory > 0 } { 
		puts "Rep$tnum: Skipping for in-memory databases."
		return
	}

	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	error_check_good set_random_seed [berkdb srand $rand_init] 0
	set nclients 5
	set logsets [create_logsets [expr $nclients + 1]]
	foreach l $logsets {
		puts "Rep$tnum ($method): Election generation test\
		    with simulated network partition $msg2."
		puts "Rep$tnum: Master logs are [lindex $l 0]"
		for { set i 0 } { $i < $nclients } { incr i } {
			puts "Rep$tnum: Client $i logs are\
			    [lindex $l [expr $i + 1]]"
		}
		rep022_sub $method $nclients $tnum $l $args
	}
}

proc rep022_sub { method nclients tnum logset largs } {
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

	set qdir $testdir/MSGQUEUEDIR
	replsetup $qdir

	set masterdir $testdir/MASTERDIR
	file mkdir $masterdir
	set m_logtype [lindex $logset 0]
	set m_logargs [adjust_logargs $m_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]

	for { set i 0 } { $i < $nclients } { incr i } {
		set clientdir($i) $testdir/CLIENTDIR.$i
		file mkdir $clientdir($i)
		set c_logtype($i) [lindex $logset [expr $i + 1]]
		set c_logargs($i) [adjust_logargs $c_logtype($i)]
		set c_txnargs($i) [adjust_txnargs $c_logtype($i)]
	}

	# Open a master.
	set envlist {}
	repladd 1
	set env_cmd(M) "berkdb_env_noerr -create -log_max 1000000 $verbargs \
	    -event rep_event $repmemargs \
	    -home $masterdir $m_txnargs $m_logargs -rep_master \
	    -errpfx MASTER -rep_transport \[list 1 replsend\]"
	set masterenv [eval $env_cmd(M)]
	lappend envlist "$masterenv 1"

	# Open the clients.
	for { set i 0 } { $i < $nclients } { incr i } {
		set envid [expr $i + 2]
		repladd $envid
		set env_cmd($i) "berkdb_env_noerr -create $verbargs \
		    -errpfx CLIENT.$i -event rep_event $repmemargs \
		    -home $clientdir($i) $c_txnargs($i) $c_logargs($i) \
		    -rep_client -rep_transport \[list $envid replsend\]"
		set clientenv($i) [eval $env_cmd($i)]
		lappend envlist "$clientenv($i) $envid"
	}

	# Bring the clients online by processing the startup messages.
	process_msgs $envlist

	# Run a modified test001 in the master.
	puts "\tRep$tnum.a: Running rep_test in replicated env."
	set niter 10
	eval rep_test $method $masterenv NULL $niter 0 0 0 $largs
	process_msgs $envlist
	error_check_good masterenv_close [$masterenv close] 0
	set envlist [lreplace $envlist 0 0]

	foreach pair $envlist {
		set i [expr [lindex $pair 1] - 2]
		replclear [expr $i + 2]
		set err_cmd($i) "none"
		set pri($i) 10
		set crash($i) 0
		if { $rep_verbose == 1 } {
			$clientenv($i) errpfx CLIENT$i
			$clientenv($i) verbose $verbose_type on
			$clientenv($i) errfile /dev/stderr
			set env_cmd($i) [concat $env_cmd($i) \
			    "-errpfx CLIENT$i -errfile /dev/stderr"]
		}
	}

	set msg "Rep$tnum.b"
	puts "\t$msg: Run election for clients 0,1,2."
	#
	# Run an election with clients 0, 1, and 2.
	# Make client 0 be the winner, and let it stay master.
	#
	set origlist $envlist
	set orignclients $nclients
	set envlist [lrange $origlist 0 2]
	set nclients 3
	set nsites 3
	set nvotes 3
	set winner 0
	setpriority pri $nclients $winner
	set elector [berkdb random_int 0 [expr $nclients - 1]]
	run_election env_cmd envlist err_cmd pri crash \
	    $qdir $msg $elector $nsites $nvotes $nclients $winner 0 test.db

	set msg "Rep$tnum.c"
	puts "\t$msg: Close and reopen client 2 with recovery."
	#
	# Now close and reopen 2 with recovery.  Update the
	# list of all client envs with the new information.
	#
	replclear 5
	replclear 6
	error_check_good flush [$clientenv(2) log_flush] 0
	error_check_good clientenv_close(2) [$clientenv(2) close] 0
	set clientenv(2) [eval $env_cmd(2) -recover]
	set origlist [lreplace $origlist 2 2 "$clientenv(2) 4"]

	# Get last LSN for client 2.
	set logc [$clientenv(2) log_cursor]
	error_check_good logc \
	    [is_valid_logc $logc $clientenv(2)] TRUE
	set lastlsn2 [lindex [lindex [$logc get -last] 0] 1]
	error_check_good close_cursor [$logc close] 0

	set msg "Rep$tnum.d"
	puts "\t$msg: Close and reopen client 4 with recovery."
	#
	# This forces the last LSN for client 4 up to the last
	# LSN for client 2 so client 4 can be elected.
	#
	set lastlsn4 0
	while { $lastlsn4 < $lastlsn2 } {
        	error_check_good clientenv_close(4) [$clientenv(4) close] 0
		set clientenv(4) [eval $env_cmd(4) -recover]
		error_check_good flush [$clientenv(4) log_flush] 0
		set origlist [lreplace $origlist 4 4 "$clientenv(4) 6"]
		set logc [$clientenv(4) log_cursor]
		error_check_good logc \
		    [is_valid_logc $logc $clientenv(4)] TRUE
		set lastlsn4 [lindex [lindex [$logc get -last] 0] 1]
 		error_check_good close_cursor [$logc close] 0
	}

	set msg "Rep$tnum.e"
	puts "\t$msg: Run election for clients 2,3,4."
	#
	# Run an election with clients 2, 3, 4.
	# Make last client be the winner, and let it stay master.
	# Need to process messages before running election so
	# that clients 2 and 4 update to the right gen with
	# client 3.
	#
	set envlist [lrange $origlist 2 4]
	process_msgs $envlist
	foreach pair $envlist {
		set i [expr [lindex $pair 1] - 2]
		set clientenv($i) [lindex $pair 0]
		set egen($i) [stat_field \
		    $clientenv($i) rep_stat "Election generation number"]
	}
	set winner 4
	setpriority pri $nclients $winner 2
	set elector [berkdb random_int 2 4]
	run_election env_cmd envlist err_cmd pri crash \
	    $qdir $msg $elector $nsites $nvotes $nclients $winner 0 test.db

	# Note egens for all the clients.
	set envlist $origlist
	foreach pair $envlist {
		set i [expr [lindex $pair 1] - 2]
		set clientenv($i) [lindex $pair 0]
		set egen($i) [stat_field \
		    $clientenv($i) rep_stat "Election generation number"]
	}

	# Have client 4 (currently a master) run an operation.
	eval rep_test $method $clientenv(4) NULL $niter 0 0 0 $largs

	# Check that clients 0 and 4 get DUPMASTER messages and
	# restart them as clients.
	#
	puts "\tRep$tnum.f: Check for DUPMASTER"
	set envlist0 [lrange $envlist 0 0]
	process_msgs $envlist0 0 dup err
	error_check_good is_dupmaster0 [lindex $dup 0] 1
	error_check_good downgrade0 [$clientenv(0) rep_start -client] 0

	set envlist4 [lrange $envlist 4 4]
	process_msgs $envlist4 0 dup err
	error_check_good is_dupmaster4 [lindex $dup 0] 1
	error_check_good downgrade4 [$clientenv(4) rep_start -client] 0

	# All DUPMASTER messages are now gone.
	# We might get residual errors however because client 4
	# responded as a master to client 0 and then became a
	# client immediately.  Therefore client 4 might get some
	# "master-only" records and return EINVAL.  We want to
	# ignore those and process records until calm is restored.
	set err 1
	while { $err == 1 } {
		process_msgs $envlist 0 dup err
		error_check_good no_dupmaster $dup 0
	}

	# Check LSNs before new election.
	foreach pair $envlist {
		set i [expr [lindex $pair 1] - 2]
		set logc [$clientenv($i) log_cursor]
		error_check_good logc \
		    [is_valid_logc $logc $clientenv($i)] TRUE
		set lastlsn [lindex [lindex [$logc get -last] 0] 1]
		error_check_good cursor_close [$logc close] 0
	}

	set msg "Rep$tnum.g"
	puts "\t$msg: Run election for all clients after DUPMASTER."

	# Call a new election with all participants.  Make 4 the
	# winner, since it should have a high enough LSN to win.
	set nclients $orignclients
	set nsites $nclients
	set nvotes $nclients
	set winner 4
	setpriority pri $nclients $winner
	set elector [berkdb random_int 0 [expr $nclients - 1]]
	run_election env_cmd envlist err_cmd pri crash \
	    $qdir $msg $elector $nsites $nvotes $nclients $winner 0 test.db

	# Pull out new egens.
	foreach pair $envlist {
		set i [expr [lindex $pair 1] - 2]
		set clientenv($i) [lindex $pair 0]
		set newegen($i) [stat_field \
		    $clientenv($i) rep_stat "Election generation number"]
	}

	# Egen numbers should all be the same now, and all greater than
	# they were before the election.
	set currentegen $newegen(0)
	for { set i 0 } { $i < $nclients } { incr i } {
		set egen_diff [expr $newegen($i) - $egen($i)]
		error_check_good egen_increased [expr $egen_diff > 0] 1
		error_check_good newegens_match $currentegen $newegen($i)
	}

	# Clean up.
	foreach pair $envlist {
		set cenv [lindex $pair 0]
		error_check_good cenv_close [$cenv close] 0
	}

	replclose $testdir/MSGQUEUEDIR
}


