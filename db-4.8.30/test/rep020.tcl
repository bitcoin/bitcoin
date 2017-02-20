# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST  rep020
# TEST	Replication elections - test election generation numbers.
# TEST

proc rep020 { method args } {
	global rand_init
	global databases_in_memory
	global repfiles_in_memory

	source ./include.tcl
	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}
	set tnum "020"

	# Run for btree only.
	if { $checking_valid_methods } {
		set test_methods { btree }
		return $test_methods
	}
	if { [is_btree $method] == 0 } {
		puts "Rep$tnum: Skipping for method $method."
		return
	}

	error_check_good set_random_seed [berkdb srand $rand_init] 0
	set nclients 5
	set logsets [create_logsets [expr $nclients + 1]]

	# Set up for on-disk or in-memory databases.
	set msg "using on-disk databases"
	if { $databases_in_memory } {
		set msg "using named in-memory databases"
		if { [is_queueext $method] } { 
			puts -nonewline "Skipping rep$tnum for method "
			puts "$method with named in-memory databases"
			return
		}
	}

	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	foreach l $logsets {
		puts "Rep$tnum ($method): Election generation test $msg $msg2."
		puts "Rep$tnum: Master logs are [lindex $l 0]"
		for { set i 0 } { $i < $nclients } { incr i } {
			puts "Rep$tnum: Client $i logs are\
			    [lindex $l [expr $i + 1]]"
		}
		rep020_sub $method $nclients $tnum $l $args
	}
}

proc rep020_sub { method nclients tnum logset largs } {
	source ./include.tcl
	global errorInfo
	global databases_in_memory
	global mixed_mode_logging
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
		set env_cmd($i) "berkdb_env_noerr -create -event rep_event \
		    $verbargs -home $clientdir($i) $repmemargs \
		    $c_txnargs($i) $c_logargs($i) \
		    -rep_client -rep_transport \[list $envid replsend\]"
		set clientenv($i) [eval $env_cmd($i)]
		lappend envlist "$clientenv($i) $envid"
	}

	# Run a modified test001 in the master.
	process_msgs $envlist
	puts "\tRep$tnum.a: Running rep_test in replicated env."
	set niter 10
	eval rep_test $method $masterenv NULL $niter 0 0 0 $largs
	process_msgs $envlist

	# Check that databases are in-memory or on-disk as expected.
	if { $databases_in_memory } {
		set dbname { "" "test.db" }
	} else { 
		set dbname "test.db"
	} 
	check_db_location $masterenv
	for { set i 0 } { $i < $nclients } { incr i } { 
		check_db_location $clientenv($i)
	}

	# Close master.
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
	puts "\t$msg: Run elections to increment egen."

	set nelect 2
	set nsites $nclients
	set nvotes $nclients
	for { set j 0 } { $j < $nelect } { incr j } {
		# Pick winner and elector randomly.
		set winner [berkdb random_int 0 [expr $nclients - 1]]
		setpriority pri $nclients $winner
		set elector [berkdb random_int 0 [expr $nclients - 1]]
		run_election env_cmd envlist err_cmd pri crash $qdir \
		    $msg $elector $nsites $nvotes $nclients $winner 1 $dbname
	}
	process_msgs $envlist

	set msg "Rep$tnum.c"
	puts "\t$msg: Updating egen when getting an old vote."

	#
	# Find the last client and save the election generation number.
	# Close the last client and adjust the list of envs to process.
	#
	set i [expr $nclients - 1]
	set last [lindex $envlist end]
	set clientenv($i) [lindex $last 0]
	set egen($i) \
	    [stat_field $clientenv($i) rep_stat "Election generation number"]
	error_check_good clientenv_close($i) [$clientenv($i) close] 0
	set envlist [lreplace $envlist end end]

	# Run a few more elections while the last client is closed.
	# Make sure we don't pick the closed client as the winner,
	# and require votes from one fewer site.
	#
	set orig_nvotes $nvotes
	set orig_nclients $nclients
	set nvotes [expr $orig_nvotes - 1]
	set nclients [expr $orig_nclients - 1]
	for { set j 0 } { $j < $nelect } { incr j } {
		set winner [berkdb random_int 0 [expr $nclients - 1]]
		setpriority pri $nclients $winner
		set elector [berkdb random_int 0 [expr $nclients - 1]]
		run_election env_cmd envlist err_cmd pri crash $qdir \
		    $msg $elector $nsites $nvotes $nclients $winner 1 $dbname
	}
	process_msgs $envlist
	#
	# Verify that the last client's election generation number has
	# changed, and that it matches the other clients.
	#
	set pair [lindex $envlist 0]
	set clenv [lindex $pair 0]
	set clegen [stat_field \
	    $clenv rep_stat "Election generation number"]

	# Reopen last client's env.  Do not run recovery, but do
	# process messages to get the egen updated.
	replclear $envid
	set clientenv($i) [eval $env_cmd($i)]
	lappend envlist "$clientenv($i) $envid"
	error_check_good client_reopen [is_valid_env $clientenv($i)] TRUE
	process_msgs $envlist

	set newegen($i) \
	    [stat_field $clientenv($i) rep_stat "Election generation number"]
	error_check_bad egen_changed $newegen($i) $egen($i)
	error_check_good egen_changed1 $newegen($i) $clegen

	set msg "Rep$tnum.d"
	puts "\t$msg: New client starts election."
	#
	# Run another election, this time called by the last client.
	# This should succeed because the last client has already
	# caught up to the others for egen.
	#
	set winner 2
	set nvotes $orig_nvotes
	set nclients $orig_nclients
	set elector [expr $nclients - 1]
	setpriority pri $nclients $winner
	run_election env_cmd envlist err_cmd pri crash $qdir \
	    $msg $elector $nsites $nvotes $nclients $winner 0 $dbname

	set newegen($i) \
	    [stat_field $clientenv($i) rep_stat "Election generation number"]
	foreach pair $envlist {
		set i [expr [lindex $pair 1] - 2]
		set clientenv($i) [lindex $pair 0]
		set egen($i) [stat_field \
		    $clientenv($i) rep_stat "Election generation number"]
	}
	error_check_good egen_catchup $egen(4) $egen(3)

	# Skip this part of the test for mixed-mode logging,
	# since we can't recover with in-memory logs.
	if { $mixed_mode_logging == 0 } {
		set msg "Rep$tnum.e"
	puts "\t$msg: Election generation set as expected after recovery."
		# Note all client egens.  Close, recover, process messages,
		# and check that egens are unchanged.
		set big_e [big_endian]
		foreach pair $envlist {
			set i [expr [lindex $pair 1] - 2]
			set clientenv($i) [lindex $pair 0]
			# Can only get egen file if repfiles on-disk.
			if { $repfiles_in_memory == 0 } {
				set fid [open $clientdir($i)/__db.rep.egen r]
				fconfigure $fid -translation binary
				set data [read $fid 4]
				if { $big_e } {
					binary scan $data I egen($i)
				} else {
					binary scan $data i egen($i)
				}
				binary scan $data c val
				close $fid
			}
			$clientenv($i) log_flush
			error_check_good \
			    clientenv_close($i) [$clientenv($i) close] 0
			set clientenv($i) [eval $env_cmd($i) -recover]
			set envlist [lreplace \
			    $envlist $i $i "$clientenv($i) [expr $i + 2]"]
		}
		process_msgs $envlist
		foreach pair $envlist {
			set i [expr [lindex $pair 1] - 2]
			set newegen($i) [stat_field $clientenv($i) \
			    rep_stat "Election generation number"]
			if { $repfiles_in_memory == 0 } {
				error_check_good egen_recovery $egen($i) \
				    $newegen($i)
			} else {
				# For rep in-memory, egen expected to start
				# over at 1 after close/reopen environment.
				error_check_good egen_recovery $newegen($i) 1
			}
		}

		# Run an election.  Now the egens should go forward.
		set winner [berkdb random_int 0 [expr $nclients - 1]]
		setpriority pri $nclients $winner
		set elector [berkdb random_int 0 [expr $nclients - 1]]
		run_election env_cmd envlist err_cmd pri crash $qdir \
		    $msg $elector $nsites $nvotes $nclients $winner 1 $dbname

		foreach pair $envlist {
			set i [expr [lindex $pair 1] - 2]
			set clientenv($i) [lindex $pair 0]
			set newegen($i) [stat_field $clientenv($i) \
			    rep_stat "Election generation number"]
			if { $repfiles_in_memory == 0 } {
				error_check_good egen_forward \
				    [expr $newegen($i) > $egen($i)] 1
			} else {
				# For rep in-memory, egen expected to
				# increment to 2 after election.
				error_check_good egen_recovery $newegen($i) 2
			}
		}
	}

	foreach pair $envlist {
		set cenv [lindex $pair 0]
                error_check_good cenv_close [$cenv close] 0
        }

	replclose $testdir/MSGQUEUEDIR
}

