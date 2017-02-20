# See the file LICENSE for redistribution information.
#
# Copyright (c) 2002-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST  rep016
# TEST	Replication election test with varying required nvotes.
# TEST
# TEST	Run a modified version of test001 in a replicated master environment;
# TEST  hold an election among a group of clients to make sure they select
# TEST  the master with varying required participants.

proc rep016 { method args } {
	global errorInfo
	global databases_in_memory
	global repfiles_in_memory

	source ./include.tcl
	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}
	set tnum "016"

	# Skip for all methods except btree.
	if { $checking_valid_methods } {
		set test_methods { btree }
		return $test_methods
	}
	if { [is_btree $method] == 0 } {
		puts "Rep$tnum: Skipping for method $method."
		return
	}

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

	# Run the body of the test with and without recovery.
	foreach r $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Rep$tnum: Skipping\
				    for in-memory logs with -recover."
				continue
			}
			puts "Rep$tnum ($method $r): Replication\
			    elections with varying nvotes $msg $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			for { set i 0 } { $i < $nclients } { incr i } {
				puts "Rep$tnum: Client $i logs are\
				    [lindex $l [expr $i + 1]]"
			}
			rep016_sub $method $nclients $tnum $l $r $args
		}
	}
}

proc rep016_sub { method nclients tnum logset recargs largs } {
	source ./include.tcl
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

	set niter 5
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
	set env_cmd(M) "berkdb_env_noerr -create -log_max 1000000 \
	    -event rep_event $repmemargs \
	    -home $masterdir $m_txnargs $m_logargs -rep_master $verbargs \
	    -errpfx MASTER -rep_transport \[list 1 replsend\]"
	set masterenv [eval $env_cmd(M) $recargs]
	lappend envlist "$masterenv 1"

	# Open the clients.
	# Don't set -errfile now -- wait until the error catching
	# portion of the test is complete.
	for { set i 0 } { $i < $nclients } { incr i } {
		set envid [expr $i + 2]
		repladd $envid
		set env_cmd($i) "berkdb_env_noerr -create -home $clientdir($i) \
		    -event rep_event $repmemargs \
		    $c_txnargs($i) $c_logargs($i) -rep_client $verbargs \
		    -rep_transport \[list $envid replsend\]"
		set clientenv($i) [eval $env_cmd($i) $recargs]
		lappend envlist "$clientenv($i) $envid"
	}
	# Bring the clients online by processing the startup messages.
	process_msgs $envlist

	# Run a modified test001 in the master.
	puts "\tRep$tnum.a: Running rep_test in replicated env."
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
	
	error_check_good masterenv_close [$masterenv close] 0
	set envlist [lreplace $envlist 0 0]

	puts "\tRep$tnum.b: Error values for rep_elect"
	#
	# Do all the error catching in client0.  We do not need to call
	# start_election here to fork a process because we never get
	# far enough to send/receive any messages.  We just want to
	# check the error message.
	#
	# !!!
	# We cannot set -errpfx or -errfile or anything in the
	# env_cmd above.  Otherwise the correct output won't be set
	# in 'ret' below and the test will fail.
	#
	# First check negative nvotes.
	#
	set nsites [expr $nclients + 1]
	set priority 2
	set timeout 5000000
	#
	# Setting nsites to 0 acts as a signal for rep_elect to use
	# the configured nsites, but since we haven't set that yet,
	# this should still fail.  TODO: need another test verifying
	# the proper operation when we *have* configured nsites.
	#
	set nsites 0
	set nvotes 2
	set res [catch {$clientenv(0) rep_elect $nsites $nvotes $priority \
	    $timeout} ret]
	error_check_bad catch $res 0
	error_check_good ret [is_substr $ret "is larger than nsites"] 1

	#
	# Check nvotes > nsites.
	#
	set nsites $nclients
	set nvotes [expr $nsites + 1]
	set res [catch {$clientenv(0) rep_elect $nsites $nvotes $priority \
	    $timeout} ret]
	error_check_bad catch $res 0
	error_check_good ret [is_substr $ret "is larger than nsites"] 1

	for { set i 0 } { $i < $nclients } { incr i } {
		replclear [expr $i + 2]
		#
		# This test doesn't use the testing hooks, so
		# initialize err_cmd and crash appropriately.
		#
		set err_cmd($i) "none"
		set crash($i) 0
		#
		# Initialize the array pri.  We'll set it to
		# appropriate values when the winner is determined.
 		#
		set pri($i) 0
		#
		if { $rep_verbose == 1 } {
			$clientenv($i) errpfx CLIENT.$i
			$clientenv($i) verbose $verbose_type on
			$clientenv($i) errfile /dev/stderr
			set env_cmd($i) [concat $env_cmd($i) \
			    "-errpfx CLIENT.$i -errfile /dev/stderr "]
		}
	}
	set m "Rep$tnum.c"
	puts "\t$m: Check single master/client can elect itself"
	#
	# 2 sites: 1 master, 1 client.  Allow lone client to elect itself.
	# Adjust client env list to reflect the single client.
	#
	set oldenvlist $envlist
	set envlist [lreplace $envlist 1 end]
	set nsites 2
	set nvotes 1
	set orig_ncl $nclients
	set nclients 1
	set elector 0
	set winner 0
	setpriority pri $nclients $winner
	run_election env_cmd envlist err_cmd pri crash\
	    $qdir $m $elector $nsites $nvotes $nclients $winner 1 $dbname

	#
	# Now run with all clients.  Client0 should always get elected
	# because it became master and should have a bigger LSN.
	#
	set nclients $orig_ncl
	set envlist [lreplace $oldenvlist 0 0 [lindex $envlist 0]]

	set m "Rep$tnum.d"
	puts "\t$m: Elect with 100% client participation"
	set nsites $nclients
	set nvotes $nclients
	set winner [rep016_selectwinner $nsites $nvotes $nclients]
	setpriority pri $nclients $winner
	run_election env_cmd envlist err_cmd pri crash\
	    $qdir $m $elector $nsites $nvotes $nclients $winner 1 $dbname

	#
	# Elect with varying levels of participation.  Start with nsites
	# as nclients+1 (simulating a down master) and require nclients,
	# and fewer (by 1) until we get down to 2 clients.
	#
	set m "Rep$tnum.e"
	puts "\t$m: Elect with varying participation"
	set nsites [expr $nclients + 1]
	set count 0
	for {set n $nclients} {$n > 1} {incr n -1} {
		set m "Rep$tnum.e.$count"
		set winner [rep016_selectwinner $nsites $n $n]
		setpriority pri $nclients $winner
		run_election env_cmd envlist err_cmd pri crash\
		    $qdir $m $elector $nsites $n $n $winner 1 $dbname
		incr count
	}

	foreach pair $envlist {
		set cenv [lindex $pair 0]
		error_check_good cenv_close [$cenv close] 0
	}
	replclose $testdir/MSGQUEUEDIR
}

proc rep016_selectwinner { nsites nvotes nclients } {
	#
	# Special case:  When we test with 100% participation, we expect
	# client 0 to always win because it has a bigger LSN than the
	# rest due to earlier part of the test.  This special case is
	# kinda gross.
	#
	if { $nsites != $nvotes } {
		set win [berkdb random_int 0 [expr $nclients - 1]]
	} else {
		set win 0
	}
	return $win
}
