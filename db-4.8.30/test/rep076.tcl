# See the file LICENSE for redistribution information.
#
# Copyright (c) 2007-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep076
# TEST	Replication elections - what happens if elected client
# TEST  does not become master?
# TEST
# TEST	Set up a master and 3 clients.  Take down master, run election.
# TEST	The elected client will ignore the fact that it's been elected,
# TEST	so we still have 2 clients.
# TEST
# TEST	Run another election, a regular election that allows the winner
# TEST	to become master, and make sure it goes okay.  We do this both
# TEST	for the client that ignored its election and for the other client.
# TEST
# TEST	This simulates what would happen if, say, we had a temporary
# TEST	network partition and lost the winner.
#
proc rep076 { method args } {
	source ./include.tcl

	global mixed_mode_logging
	global databases_in_memory
	global repfiles_in_memory

	set tnum "076"
	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}

	# Run for btree only.
	if { $checking_valid_methods } {
		set test_methods { btree }
		return $test_methods
	}
	if { [is_btree $method] == 0 } {
		puts "Rep$tnum: Skipping for method $method."
		return
	}

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

	set nclients 3
	set logsets [create_logsets [expr $nclients + 1]]
	set winsets { { 1 1 } { 1 2 } }
	foreach l $logsets {
		foreach w $winsets {
			puts "Rep$tnum ($method): Replication elections -\
			    elected client ignores election $msg $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			for { set i 0 } { $i < $nclients } { incr i } {
				puts "Rep$tnum: Client $i logs are\
				    [lindex $l [expr $i + 1]]"
			}
			rep076_sub $method $nclients $tnum $l $w $args
		}
	}
}

proc rep076_sub { method nclients tnum logset winset largs } {
	source ./include.tcl
	global machids
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
	set env_cmd(M) "berkdb_env -create -log_max 1000000 $verbargs \
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
		    -event rep_event $repmemargs \
		    -home $clientdir($i) $c_txnargs($i) $c_logargs($i) \
		    -rep_client -rep_transport \[list $envid replsend\]"
		set clientenv($i) [eval $env_cmd($i)]
		error_check_good \
		    client_env($i) [is_valid_env $clientenv($i)] TRUE
		lappend envlist "$clientenv($i) $envid"
	}

	# Bring the clients online by processing the startup messages.
	process_msgs $envlist

	# Run a modified test001 in the master.
	puts "\tRep$tnum.a: Running rep_test in replicated env."
	set niter 10
	eval rep_test $method $masterenv NULL $niter 0 0 0 $largs
	process_msgs $envlist

	# Close master.
	error_check_good masterenv_close [$masterenv close] 0
	set envlist [lreplace $envlist 0 0]

	# Winner1 is set up to win the first election, winner2
	# the second.
	set m "Rep$tnum.b"
	set winner1 [lindex $winset 0]
	set winner2 [lindex $winset 1]
	set elector 1
	set nsites $nclients
	set nvotes $nclients
	setpriority pri $nclients $winner1
	if { $databases_in_memory } {
		set dbname { "" "test.db" }
	} else { 
		set dbname "test.db"
	} 

	foreach pair $envlist {
		set i [expr [lindex $pair 1] - 2]
		replclear [expr $i + 2]
		set err_cmd($i) "none"
		set crash($i) 0
		if { $rep_verbose == 1 } {
			$clientenv($i) errpfx CLIENT$i
			$clientenv($i) verbose $verbose_type on
			$clientenv($i) errfile /dev/stderr
			set env_cmd($i) [concat $env_cmd($i) \
			    "-errpfx CLIENT$i -errfile /dev/stderr"]
		}
	}


	# Run election where winner will ignore its election and
	# not be made master.
	puts "\tRep$tnum: First winner ignores its election."
	run_election env_cmd envlist err_cmd pri crash $qdir $m\
	    $elector $nsites $nvotes $nclients $winner1 0 $dbname 1

	# Run second election where winner accepts its election and
	# is made master.
	puts "\tRep$tnum: Second winner accepts its election."
	setpriority pri $nclients $winner2
	run_election env_cmd envlist err_cmd pri crash $qdir $m\
	    $elector $nsites $nvotes $nclients $winner2 0 $dbname

	# Clean up.
	foreach pair $envlist {
		set cenv [lindex $pair 0]
		error_check_good cenv_close [$cenv close] 0
	}

	replclose $testdir/MSGQUEUEDIR
}

