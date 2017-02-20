# See the file LICENSE for redistribution information.
#
# Copyright (c) 2002-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST  rep002
# TEST	Basic replication election test.
# TEST
# TEST	Run a modified version of test001 in a replicated master
# TEST	environment; hold an election among a group of clients to
# TEST	make sure they select a proper master from amongst themselves,
# TEST	in various scenarios.

proc rep002 { method { niter 10 } { nclients 3 } { tnum "002" } args } {

	source ./include.tcl
	global repfiles_in_memory

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}

	# Skip for record-based methods.
	if { $checking_valid_methods } {
		set test_methods {}
		foreach method $valid_methods {
			if { [is_record_based $method] != 1 } {
				lappend test_methods $method
			}
		}
		return $test_methods
	}
	if { [is_record_based $method] == 1 } {
		puts "Rep002: Skipping for method $method."
		return
	}

	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	set logsets [create_logsets [expr $nclients + 1]]

	# Run the body of the test with and without recovery.
	foreach r $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Skipping test with -recover for in-memory logs."
			}
			puts "Rep$tnum ($method $r): Replication election\
			    test with $nclients clients $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			for { set i 0 } { $i < $nclients } { incr i } {
				puts "Rep$tnum: Client $i logs are\
				    [lindex $l [expr $i + 1]]"
			}
			rep002_sub $method $niter $nclients $tnum $l $r $args
		}
	}
}

proc rep002_sub { method niter nclients tnum logset recargs largs } {
	source ./include.tcl
	global repfiles_in_memory
	global elect_timeout elect_serial
	set elect_timeout(default) 5000000

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
	repladd 1
	set env_cmd(M) "berkdb_env_noerr -create -log_max 1000000 \
	    -event rep_event $repmemargs \
	    -home $masterdir $m_logargs -errpfx MASTER $verbargs \
	    $m_txnargs -rep_master -rep_transport \[list 1 replsend\]"
	# In an election test, the -recovery arg must not go
	# in the env_cmd string because that is going to be
	# passed to a child process.
	set masterenv [eval $env_cmd(M) $recargs]

	# Open the clients.
	for { set i 0 } { $i < $nclients } { incr i } {
		set envid [expr $i + 2]
		repladd $envid
		set env_cmd($i) "berkdb_env_noerr -create -home $clientdir($i) \
		    -event rep_event $repmemargs \
		    $c_logargs($i) $c_txnargs($i) -rep_client -errpfx CLIENT$i \
		    $verbargs -rep_transport \[list $envid replsend\]"
		set clientenv($i) [eval $env_cmd($i) $recargs]
	}

	# Loop, processing first the master's messages, then the client's,
	# until both queues are empty.
	set envlist {}
	lappend envlist "$masterenv 1"
	for { set i 0 } { $i < $nclients } { incr i } {
		lappend envlist "$clientenv($i) [expr $i + 2]"
	}
	process_msgs $envlist

	# Run a modified test001 in the master.
	puts "\tRep$tnum.a: Running test001 in replicated env."
	eval test001 $method $niter 0 0 $tnum -env $masterenv $largs
	process_msgs $envlist

	# Verify the database in the client dir.
	for { set i 0 } { $i < $nclients } { incr i } {
		puts "\tRep$tnum.b: Verifying contents of client database $i."
		set testdir [get_home $masterenv]
		set t1 $testdir/t1
		set t2 $testdir/t2
		set t3 $testdir/t3
		open_and_dump_file test$tnum.db $clientenv($i) $testdir/t1 \
	    	    test001.check dump_file_direction "-first" "-next"

		if { [string compare [convert_method $method] -recno] != 0 } {
			filesort $t1 $t3
		}
		error_check_good diff_files($t2,$t3) [filecmp $t2 $t3] 0

		verify_dir $clientdir($i) "\tRep$tnum.c: " 0 0 1
	}

	# Start an election in the first client.
	puts "\tRep$tnum.d: Starting election with existing master."
	# We want to verify that the master declares the election
	# over by fiat, even if everyone uses a lower priority than 20.
	# Loop and process all messages, keeping track of which
	# sites got a HOLDELECTION and checking that the master i.d. is
	# unchanged after the election.

	set origrole [stat_field $masterenv rep_stat "Role"]
	error_check_good originally_master $origrole "master"
	set origgeneration [stat_field $masterenv rep_stat "Generation number"]

	set got_hold_elect(M) 0
	for { set i 0 } { $i < $nclients } { incr i } {
		set got_hold_elect($i) 0
		set elect_pipe($i) INVALID
	}
	set elect_pipe(0) [start_election C0 $qdir $env_cmd(0) \
	    [expr $nclients + 1] $nclients 20 $elect_timeout(default)]

	tclsleep 2

	set got_master 0
	while { 1 } {
		set nproced 0
		set he 0

		incr nproced [replprocessqueue $masterenv 1 0 he]

		if { $he == 1 } {
			incr elect_serial
			set elect_pipe(M) [start_election CM $qdir \
			    $env_cmd(M) [expr $nclients + 1] $nclients \
			    0 $elect_timeout(default)]
			set got_hold_elect(M) 1
		}

		for { set i 0 } { $i < $nclients } { incr i } {
			set he 0
			set envid [expr $i + 2]
			incr nproced \
			    [replprocessqueue $clientenv($i) $envid 0 he]
			if { $he == 1 } {
				# error_check_bad client(0)_in_elect $i 0
				if { $elect_pipe($i) != "INVALID" } {
					close_election $elect_pipe($i)
				}
				incr elect_serial
				set pfx CHILD$i.$elect_serial
				set elect_pipe($i) [start_election $pfx $qdir \
			    	    $env_cmd($i) [expr $nclients + 1] \
				    $nclients 0 \
				    $elect_timeout(default)]
				set got_hold_elect($i) 1
			}
		}

		if { $nproced == 0 } {
			break
		}
	}
	set role [stat_field $masterenv rep_stat "Role"]
	set generation [stat_field $masterenv rep_stat "Generation number"]
	error_check_good master_unchanged $origrole $role
	error_check_good gen_unchanged $origgeneration $generation
	cleanup_elections

	# We need multiple clients to proceed from here.
	if { $nclients < 2 } {
		puts "\tRep$tnum: Skipping for less than two clients."
		error_check_good masterenv_close [$masterenv close] 0
		for { set i 0 } { $i < $nclients } { incr i } {
			error_check_good clientenv_close($i) \
			    [$clientenv($i) close] 0
		}
		return
	}

	# Make sure all the clients are synced up and ready to be good
	# voting citizens.
	error_check_good master_flush [$masterenv rep_flush] 0
	process_msgs $envlist

	# Now hold another election in the first client, this time with
	# a dead master.
	puts "\tRep$tnum.e: Starting election with dead master."
	error_check_good masterenv_close [$masterenv close] 0
	set envlist [lreplace $envlist 0 0]

	set m "Rep$tnum.e"
	# We're not going to be using err_cmd, so initialize to "none".
	# Client #1 has priority 100; everyone else has priority 10.
	for { set i 0 } { $i < $nclients } { incr i } {
		set err_cmd($i) "none"
		set crash($i) 0
		if { $i == 1 } {
			set pri($i) 100
		} else {
			set pri($i) 10
		}
	}
	set nsites $nclients
	set nvotes $nclients
	# The elector calls the first election.  The expected winner
	# is $win.
	set elector 1
	set win 1
	run_election env_cmd envlist err_cmd pri crash $qdir $m \
	    $elector $nsites $nvotes $nclients $win 1 "test$tnum.db"

	# Hold an election with two clients at the same (winning) priority.
	# Make sure that the tie gets broken, and that the third client
	# does not win.
	puts "\tRep$tnum.f: Election with two clients at same priority."
	set m "Rep$tnum.f"
	# Clients 0 and 1 have high, matching priority.
	for { set i 0 } { $i < $nclients } { incr i } {
		if { $i >= 2 } {
			set pri($i) 10
		} else {
			set pri($i) 100
		}
	}

	# Run several elections.
	set elections 5
	for { set i 0 } { $i < $elections } { incr i } {
		#
		# The expected winner is 0 or 1.  Since run_election can only
		# handle one expected winner, catch the result and inspect it.
		#
		set elector 0
		set win 1
		set altwin 0
		if {[catch {eval run_election \
		    env_cmd envlist err_cmd pri crash $qdir $m $elector $nsites \
		    $nvotes $nclients $win 1 "test$tnum.db"} res]} {
			#
			# If the primary winner didn't win, make sure
			# the alternative winner won.  Do all the cleanup
			# for that winner normally done in run_election:
			# open and close the new master, then reopen as a
			# client for the next cycle.
			#
			error_check_good check_winner [is_substr \
			    $res "expected 3, got [expr $altwin + 2]"] 1
			puts "\t$m: Election $i: Alternate winner $altwin won."
			error_check_good make_master \
			    [$clientenv($altwin) rep_start -master] 0

			cleanup_elections
			process_msgs $envlist

			error_check_good newmaster_close \
			    [$clientenv($altwin) close] 0
			set clientenv($altwin) [eval $env_cmd($altwin)]
			error_check_good cl($altwin) \
			    [is_valid_env $clientenv($altwin)] TRUE
			set newelector "$clientenv($altwin) [expr $altwin + 2]"
			set envlist [lreplace $envlist $altwin $altwin $newelector]
		} else {
			puts "\t$m: Election $i: Primary winner $win won."
		}
		process_msgs $envlist
	}

	foreach pair $envlist {
		set cenv [lindex $pair 0]
		error_check_good cenv_close [$cenv close] 0
	}

	replclose $testdir/MSGQUEUEDIR
}
