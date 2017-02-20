# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep021
# TEST	Replication and multiple environments.
# TEST	Run similar tests in separate environments, making sure
# TEST	that some data overlaps.  Then, "move" one client env
# TEST	from one replication group to another and make sure that
# TEST	we do not get divergent logs.  We either match the first
# TEST	record and end up with identical logs or we get an error.
# TEST	Verify all client logs are identical if successful.
#
proc rep021 { method { nclients 3 } { tnum "021" } args } {

	source ./include.tcl
	global repfiles_in_memory

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}

	# Run for all access methods.
	if { $checking_valid_methods } {
		return "ALL"
	}

	# This test depends on copying logs, so can't be run with
	# in-memory logging.
	global mixed_mode_logging
	if { $mixed_mode_logging > 0 } {
		puts "Rep$tnum: Skipping for mixed-mode logging."
		return
	}

	# This test closes its envs, so it's not appropriate for 
	# testing of in-memory named databases.
	global databases_in_memory
	if { $databases_in_memory } { 
		puts "Rep$tnum: Skipping for in-memory databases."
		return
	}

	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	set args [convert_args $method $args]
	set logsets [create_logsets [expr $nclients + 1]]

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
			    and $nclients recovered clients in sync $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			for { set i 0 } { $i < $nclients } { incr i } {
				puts "Rep$tnum: Client $i logs are\
				    [lindex $l [expr $i + 1]]"
			}
			rep021_sub $method $nclients $tnum $l $r $args
		}
	}
}

proc rep021_sub { method nclients tnum logset recargs largs } {
	global testdir
	global util_path
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

	set orig_tdir $testdir
	env_cleanup $testdir

	replsetup $testdir/MSGQUEUEDIR

	set niter 100
	set offset 5
	set masterdir $testdir/MASTERDIR
	set masterdir2 $testdir/MASTERDIR.NEW
	file mkdir $masterdir
	file mkdir $masterdir2

	set m_logtype [lindex $logset 0]
	set m_logargs [adjust_logargs $m_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]

	# We want to run the test 3 times in 2 separate repl envs.
	# This is a little bit tricky due to how we manage replication
	# in Tcl.  It assumes one replication group.
	# This is tricky because we need to manage/clear the repl
	# message queues for the different groups when running
	# to one group or the other.
	# To accomplish this we run entirely in the 2nd group first.
	# We set it up and then close all its envs.  Then we run
	# to the 1st group, and set it up.  Then we add in a client
	# from the 2nd group into the existing 1st group.
	# Although we're doing them in separate parts, this is
	# a description of what we're doing.
	#
	# 1.  First add divergent data to database:
	# RepGrp1: Add niter data from 0 to database.
	# RepGrp2: Add niter data from offset to database.
	# This gives us overlapping data in the databases, but they're
	# additions will be at different offsets in the log files.
	#
	# 2.  Add identical data to both databases.
	# RepGrp1: Add niter data from niter + offset to database.
	# RepGrp2: Add niter data from niter + offset to database.
	# This gives us identical data in the databases and logs.
	#
	# 3.  Again add divergent data to databases.
	# RepGrp1: Add niter data from niter*2+offset to database.
	# RepGrp2: Add niter data from niter*2+offset*2 to database.
	# This gives us overlapping data in the databases, but they're
	# additions will be at different offsets in the log files.
	#
	# 4.  Add a client from one group to the other.  Then try
	# to sync up that client.  We should get a failure with
	# one of the non-matching error messages:
	#	"Too few log files to sync with master"
	#	 REP_JOIN_FAILURE

	# Open a 2nd master.  Make all the 2nd env ids >= 10.
	# For the 2nd group, just have 1 master and 1 client.
	repladd 10
	set ma2_envcmd "berkdb_env_noerr -create $m_txnargs $verbargs \
	    $m_logargs -home $masterdir2 $repmemargs \
	    -rep_master -rep_transport \[list 10 replsend\]"
	set menv2 [eval $ma2_envcmd $recargs]

	set clientdir2 $testdir/CLIENTDIR.NEW
	file mkdir $clientdir2
	set id2 11
	set c_logtype($id2) [lindex $logset 1]
	set c_logargs($id2) [adjust_logargs $c_logtype($id2)]
	set c_txnargs($id2) [adjust_txnargs $c_logtype($id2)]

	set id2 11
	repladd $id2
	set cl2_envcmd "berkdb_env_noerr -create $c_txnargs($id2) $verbargs \
	    $c_logargs($id2) -home $clientdir2 $repmemargs \
	    -rep_client -rep_transport \[list $id2 replsend\]"
	set clenv2 [eval $cl2_envcmd $recargs]

	set testfile "test$tnum.db"
	set omethod [convert_method $method]

	set masterdb2 [eval {berkdb_open_noerr -env $menv2 -auto_commit \
	    -create -mode 0644} $largs $omethod $testfile]
	error_check_good dbopen [is_valid_db $masterdb2] TRUE

	#
	# Process startup messages
	#
	set env2list {}
	lappend env2list "$menv2 10"
	lappend env2list "$clenv2 $id2"
	process_msgs $env2list

	#
	# Set up the three runs of rep_test.  We need the starting
	# point for each phase of the test for each group.
	#
	set e1phase1 0
	set e2phase1 $offset
	set e1phase2 [expr $niter + $offset]
	set e2phase2 $e1phase2
	set e1phase3 [expr $e1phase2 + $niter]
	set e2phase3 [expr $e2phase2 + $niter + $offset]

	puts "\tRep$tnum.a: Running rep_test in 2nd replicated env."
	eval rep_test $method $menv2 $masterdb2 $niter $e2phase1 1 1 $largs
	eval rep_test $method $menv2 $masterdb2 $niter $e2phase2 1 1 $largs
	eval rep_test $method $menv2 $masterdb2 $niter $e2phase3 1 1 $largs
	error_check_good mdb_cl [$masterdb2 close] 0
	process_msgs $env2list

	puts "\tRep$tnum.b: Close 2nd replicated env.  Open primary."
	error_check_good mdb_cl [$clenv2 close] 0
	error_check_good mdb_cl [$menv2 close] 0
	replclose $testdir/MSGQUEUEDIR

	#
	# Run recovery in client now to blow away region files so
	# that this client comes in as a "new" client and announces itself.
	#
	set stat [catch {eval exec $util_path/db_recover -h $clientdir2} result]
	error_check_good stat $stat 0

	#
	# Now we've run in the 2nd env.  We have everything we need
	# set up and existing in that env.  Now run the test in the
	# 1st env and then we'll try to add in the client.
	#
	replsetup $testdir/MSGQUEUEDIR
	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs $verbargs \
	    $m_logargs -home $masterdir $repmemargs \
	    -rep_master -rep_transport \[list 1 replsend\]"
	set menv [eval $ma_envcmd $recargs]

	for {set i 0} {$i < $nclients} {incr i} {
		set clientdir($i) $testdir/CLIENTDIR.$i
		file mkdir $clientdir($i)
		set c_logtype($i) [lindex $logset [expr $i + 1]]
		set c_logargs($i) [adjust_logargs $c_logtype($i)]
		set c_txnargs($i) [adjust_txnargs $c_logtype($i)]
		set id($i) [expr 2 + $i]
		repladd $id($i)
		set cl_envcmd($i) "berkdb_env_noerr -create $c_txnargs($i) \
		    $c_logargs($i) -home $clientdir($i) $repmemargs \
		    $verbargs \
		    -rep_client -rep_transport \[list $id($i) replsend\]"
		set clenv($i) [eval $cl_envcmd($i) $recargs]
	}

	set masterdb [eval {berkdb_open_noerr -env $menv -auto_commit \
	    -create -mode 0644} $largs $omethod $testfile]
	error_check_good dbopen [is_valid_db $masterdb] TRUE

	# Bring the clients online by processing the startup messages.
	set envlist {}
	lappend envlist "$menv 1"
	for { set i 0 } { $i < $nclients } { incr i } {
		lappend envlist "$clenv($i) $id($i)"
	}
	process_msgs $envlist

	# Run a modified test001 in the master (and update clients).
	puts "\tRep$tnum.c: Running rep_test in primary replicated env."
	eval rep_test $method $menv $masterdb $niter $e1phase1 1 1 $largs
	eval rep_test $method $menv $masterdb $niter $e1phase2 1 1 $largs
	eval rep_test $method $menv $masterdb $niter $e1phase3 1 1 $largs
	error_check_good mdb_cl [$masterdb close] 0
	# Process any close messages.
	process_msgs $envlist

	puts "\tRep$tnum.d: Add unrelated client into replication group."
	set i $nclients
	set orig $nclients
	set nclients [expr $nclients + 1]

	set clientdir($i) $clientdir2
	set id($i) [expr 2 + $i]
	repladd $id($i)
	set cl_envcmd($i) "berkdb_env_noerr -create -txn nosync \
	    -home $clientdir($i) $verbargs $repmemargs \
	    -rep_client -rep_transport \[list $id($i) replsend\]"
	set clenv($i) [eval $cl_envcmd($i) $recargs]
	#
	# We'll only catch an error if we turn on no-autoinit.
	# Otherwise, the system will throw away everything on the
	# client and resync.
	#
	$clenv($i) rep_config {noautoinit on}

	lappend envlist "$clenv($i) $id($i)"

	fileremove -f $clientdir2/prlog.orig
	set stat [catch {eval exec $util_path/db_printlog \
	    -h $clientdir2 >> $clientdir2/prlog.orig} result]

	set err 0
	process_msgs $envlist 0 NONE err

	puts "\tRep$tnum.e: Close all envs and run recovery in clients."
	error_check_good menv_cl [$menv close] 0
	for {set i 0} {$i < $nclients} {incr i} {
		error_check_good cl$i.close [$clenv($i) close] 0
		set hargs($i) "-h $clientdir($i)"
	}
	set i [expr $nclients - 1]
	fileremove -f $clientdir($i)/prlog
	set stat [catch {eval exec $util_path/db_printlog \
		    -h $clientdir($i) >> $clientdir($i)/prlog} result]

	# If we got an error, then the log should match the original
	# and the error message should tell us the client was never
	# part of this environment.
	#
	if { $err != 0 } {
		puts "\tRep$tnum.f: Verify client log matches original."
		error_check_good log_cmp(orig,$i) \
		    [filecmp $clientdir($i)/prlog.orig $clientdir($i)/prlog] 0
		puts "\tRep$tnum.g: Verify client error."
		error_check_good errchk [is_substr $err \
		    "REP_JOIN_FAILURE"] 1
	} else {
		puts "\tRep$tnum.f: Verify client log doesn't match original."
		error_check_good log_cmp(orig,$i) \
		    [filecmp $clientdir($i)/prlog.orig $clientdir($i)/prlog] 1
		puts "\tRep$tnum.g: Verify new client log matches master."
		set stat [catch {eval exec $util_path/db_printlog \
		    -h $masterdir >& $masterdir/prlog} result]
		fileremove -f $clientdir($i)/prlog
		set stat [catch {eval exec $util_path/db_printlog \
		    -h $clientdir($i) >> $clientdir($i)/prlog} result]
		error_check_good stat_prlog $stat 0
		error_check_good log_cmp(master,$i) \
		    [filecmp $masterdir/prlog $clientdir($i)/prlog] 0
	}

	replclose $testdir/MSGQUEUEDIR
	set testdir $orig_tdir
	return
}

