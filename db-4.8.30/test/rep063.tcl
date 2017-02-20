# See the file LICENSE for redistribution information.
#
# Copyright (c) 2002-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST  rep063
# TEST	Replication election test with simulated different versions
# TEST	for each site.  This tests that old sites with real priority
# TEST	trump ELECTABLE sites with zero priority even with greater LSNs.
# TEST	There is a special case in the code for testing that if the
# TEST	priority is <= 10, we simulate mixed versions for elections.
# TEST
# TEST	Run a rep_test in a replicated master environment and close;
# TEST  hold an election among a group of clients to make sure they select
# TEST  the master with varying LSNs and priorities.
#
proc rep063 { method args } {
	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}
	set tnum "063"

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
			puts "$method with named in-memory databases."
			return
		}
	}

	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	# Run the body of the test with and without recovery.
	set recopts { "" "-recover" }
	foreach r $recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Rep$tnum: Skipping\
				    for in-memory logs with -recover."
				continue
			}
			puts "Rep$tnum ($method $r): Replication\
			    elections with varying versions $msg $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			for { set i 0 } { $i < $nclients } { incr i } {
				puts "Rep$tnum: Client $i logs are\
				    [lindex $l [expr $i + 1]]"
			}
			rep063_sub $method $nclients $tnum $l $r $args
		}
	}
}

proc rep063_sub { method nclients tnum logset recargs largs } {
	source ./include.tcl
	global electable_pri
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

	set niter 80

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
	set env_cmd(M) "berkdb_env_noerr -create -log_max 1000000 \
	    -event rep_event $repmemargs \
	    -home $masterdir $m_txnargs $m_logargs -rep_master $verbargs \
	    -errpfx MASTER -rep_transport \[list 1 replsend\]"
	set masterenv [eval $env_cmd(M) $recargs]
	error_check_good master_env [is_valid_env $masterenv] TRUE
	lappend envlist "$masterenv 1"

	# Open the clients.
	for { set i 0 } { $i < $nclients } { incr i } {
		set envid [expr $i + 2]
		repladd $envid
		set env_cmd($i) "berkdb_env_noerr -create -home $clientdir($i) \
		    -event rep_event $repmemargs \
		    $c_txnargs($i) $c_logargs($i) -rep_client \
		    -rep_transport \[list $envid replsend\]"
		set clientenv($i) [eval $env_cmd($i) $recargs]
		error_check_good \
		    client_env($i) [is_valid_env $clientenv($i)] TRUE
		lappend envlist "$clientenv($i) $envid"
	}
	# Bring the clients online by processing the startup messages.
	process_msgs $envlist

	# Run a modified test001 in the master.
	puts "\tRep$tnum.a: Running rep_test in replicated env."
	eval rep_test $method $masterenv NULL $niter 0 0 0 $largs
	process_msgs $envlist

	#
	# We remove some client envs and run rep_test so that we can
	# force some client LSNs to be further ahead/behind than others.
	# When we're done, the LSNs look like this:
	#
	# Client0: ......................
	# Client1: ...........
	# Client2: ...........
	# Client3: ......................
	# Client4: .................................
	#
	# Remove client 1 and 2 from list to process, this guarantees
	# clients 0, 3 and 4 are ahead in LSN.  We use each of these
	# in different parts of the test so guarantee bigger LSNs.
	#
	set orig_env $envlist
	set envlist [lreplace $envlist 3 3]
	set envlist [lreplace $envlist 2 2]
	eval rep_test $method $masterenv NULL $niter 0 0 0 $largs
	process_msgs $envlist
	#
	# Remove client 3 so that client 4 has the biggest LSN of all.
	#
	set eend [llength $envlist]
	set cl3_i [expr $eend - 2]
	set envlist [lreplace $envlist $cl3_i $cl3_i]
	eval rep_test $method $masterenv NULL $niter 0 0 0 $largs
	process_msgs $envlist
	#
	# Put all removed clients back in.
	#
	set envlist $orig_env
	error_check_good masterenv_close [$masterenv close] 0
	set envlist [lreplace $envlist 0 0]

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
			error_check_good pfx [$clientenv($i) errpfx CLIENT$i] 0
			$clientenv($i) verbose $verbose_type on
			set env_cmd($i) [concat $env_cmd($i) \
			    "-errpfx CLIENT$i $verbargs "]
		}
	}
	#
	# Remove clients 3 and 4 from the envlist.  We'll save those for
	# later.
	#
	set cl4 [lindex $envlist 4]
	set envlist [lreplace $envlist 4 4]
	set cl3 [lindex $envlist 3]
	set envlist [lreplace $envlist 3 3]

	set m "Rep$tnum.b"
	#
	# Client 0 has the biggest LSN of clients 0, 1, 2.
	# However, 'setpriority' will set the priority of client 1
	# to simulate client 1 being an "older version" client.
	# Client 1 should win even though its LSN is smaller.
	# This tests one "older" client and the rest "newer".
	#
	puts "\t$m: Test old client trumps new clients with bigger LSN."
	set orig_ncl $nclients
	set nclients 3
	set nsites $nclients
	set nvotes $nclients
	set winner 1
	set elector 2
	setpriority pri $nclients $winner 0 1

	# Set up databases as in-memory or on-disk and run the election.
	if { $databases_in_memory } {
		set dbname { "" "test.db" }
	} else { 
		set dbname "test.db"
	} 
	run_election env_cmd envlist err_cmd pri crash\
	    $qdir $m $elector $nsites $nvotes $nclients $winner 0 $dbname

	#
	# In all of the checks of the Election Priority stat field,
	# we use clientenv(2).  The reason is that we never expect
	# client 2 to be the winner.  The env handles of client 0 and 1
	# are getting closed and reopened as a master/client in
	# the election and the old recorded handles are invalid.
	# This one is known to be valid throughout the entire test.
	#
	error_check_bad old_pri [stat_field $clientenv(2) rep_stat \
	    "Election priority"] 0
	#
	# When we finish the election, all clients are at the same LSN.
	# Call this proc to make the winner have a larger LSN than the
	# other 2 remaining clients, and reopen the winner as a client.
	#
	rep063_movelsn_reopen $method envlist $env_cmd($winner) $winner $largs

	set m "Rep$tnum.c"
	puts "\t$m: Test old client with zero priority new client."
	#
	# Client 1 now has a bigger LSN, so make client 0 the old client
	# and client 1 a real 0 priority new client.
	#
	set winner 0
	setpriority pri $nclients $winner 0 1
	set pri(1) 0
	run_election env_cmd envlist err_cmd pri crash $qdir \
	    $m $elector $nsites $nvotes $nclients $winner 0 $dbname
	error_check_bad old_pri [stat_field $clientenv(2) rep_stat \
	    "Election priority"] 0
	rep063_movelsn_reopen $method envlist $env_cmd($winner) $winner $largs

	set m "Rep$tnum.d"
	puts "\t$m: Test multiple old clients with new client."
	#
	# Client 0 is now has a bigger LSN, so make client 1 winner.
	# We are setting client 2's priority to something bigger so that
	# we simulate having 2 "older version" clients (clients 1 and 2)
	# and one new client (client 0).  This tests that the right client
	# among the older versions gets correctly elected even though there
	# is a bigger LSN "new" client participating.
	#
	set winner 1
	setpriority pri $nclients $winner 0 1
	set pri(2) [expr $pri(1) / 2]
	run_election env_cmd envlist err_cmd pri crash $qdir \
	    $m $elector $nsites $nvotes $nclients $winner 0 $dbname
	error_check_bad old_pri [stat_field $clientenv(2) rep_stat \
	    "Election priority"] 0
	rep063_movelsn_reopen $method envlist $env_cmd($winner) $winner $largs

	set m "Rep$tnum.e"
	puts "\t$m: Test new clients, client 1 not electable."
	#
	# Client 1 now has a bigger LSN, so make it unelectable.  Add in
	# old client 3 since that should be the biggest LSN of all these.
	# Set all other priorities to electable_pri to make them all equal (and
	# all "new" clients).  We know client 3 should win because we
	# set its LSN much farther ahead in the beginning.
	#
	set winner 3
	replclear [expr $winner + 2]
	set nclients 4
	set nsites $nclients
	set nvotes $nclients
	set pri(0) $electable_pri
	set pri(1) 0
	set pri(2) $electable_pri
	set pri(3) $electable_pri
	replclear [lindex $cl3 1]
	lappend envlist $cl3
	#
	# Winner should be zero priority.
	#
	run_election env_cmd envlist err_cmd pri crash $qdir \
	    $m $elector $nsites $nvotes $nclients $winner 0 $dbname
	error_check_good elect_pri [stat_field $clientenv(2) rep_stat \
	    "Election priority"] 0
	rep063_movelsn_reopen $method envlist $env_cmd($winner) $winner $largs

	#
	# Now add in Client 4, the site with the biggest LSN of all.
	# Test with all being electable clients.
	#
	set m "Rep$tnum.f"
	puts "\t$m: Test all new electable clients."
	set winner 4
	set nclients 5
	set nsites $nclients
	set nvotes $nclients
	set pri(0) $electable_pri
	set pri(1) $electable_pri
	set pri(2) $electable_pri
	set pri(3) $electable_pri
	set pri(4) $electable_pri
	replclear [expr $winner + 2]
	lappend envlist $cl4
	#
	# Client 4 has biggest LSN and should now win, but winner should
	# be zero priority.
	#
	run_election env_cmd envlist err_cmd pri crash $qdir \
	    $m $elector $nsites $nvotes $nclients $winner 0 $dbname
	error_check_good elect_pri [stat_field $clientenv(2) rep_stat \
	    "Election priority"] 0

	foreach pair $envlist {
		set cenv [lindex $pair 0]
		error_check_good cenv_close [$cenv close] 0
	}
	replclose $testdir/MSGQUEUEDIR
}

#
# Move the LSN ahead on the newly elected master, while not sending
# those messages to the other clients.  Then close the env and
# reopen it as a client.  Use upvar so that the envlist is
# modified when we return and can get messages.
#
proc rep063_movelsn_reopen { method envlist env_cmd eindex largs } {
	upvar $envlist elist

	set clrlist { }
	set i 0
	foreach e $elist {
		#
		# If we find the master env entry, get its env handle.
		# If not, then get the id so that we can replclear it later.
		#
		if { $i == $eindex } {
			set masterenv [lindex $e 0]
		} else {
			lappend clrlist [lindex $e 1]
		}
		incr i
	}
	#
	# Move this env's LSN ahead.
	#
	set niter 10
	eval rep_test $method $masterenv NULL $niter 0 0 0 $largs

	foreach cl $clrlist {
		replclear $cl
	}
	#
	# Now close this env and reopen it as a client.
	#
	error_check_good newmaster_close [$masterenv close] 0
	set newclenv [eval $env_cmd]
	error_check_good cl [is_valid_env $newclenv] TRUE
	set newenv "$newclenv [expr $eindex + 2]"
	set elist [lreplace $elist $eindex $eindex $newenv]
	process_msgs $elist
}
