# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST  rep019
# TEST	Replication and multiple clients at same LSN.
# TEST  Have several clients at the same LSN.  Run recovery at
# TEST  different times.  Declare a client master and after sync-up
# TEST  verify all client logs are identical.
#
proc rep019 { method { nclients 3 } { tnum "019" } args } {

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

	# This test needs to use recovery, so mixed-mode testing
	# isn't appropriate, nor in-memory database testing. 
	global databases_in_memory
	if { $databases_in_memory > 0 } {
		puts "Rep$tnum: Skipping for in-memory databases."
		return
	}
	global mixed_mode_logging
	if { $mixed_mode_logging > 0 } {
		puts "Rep$tnum: Skipping for mixed-mode logging."
		return
	}

	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	set args [convert_args $method $args]

	# Run the body of the test with and without recovery.
	foreach r $test_recopts {
		puts "Rep$tnum ($method $r): Replication\
		     and $nclients recovered clients in sync $msg2."
		rep019_sub $method $nclients $tnum $r $args
	}
}

proc rep019_sub { method nclients tnum recargs largs } {
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
	set masterdir $testdir/MASTERDIR
	file mkdir $masterdir

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create -txn nosync $verbargs \
	    -home $masterdir -rep_master -errpfx MASTER $repmemargs \
	    -rep_transport \[list 1 replsend\]"
	set menv [eval $ma_envcmd $recargs]

	for {set i 0} {$i < $nclients} {incr i} {
		set clientdir($i) $testdir/CLIENTDIR.$i
		file mkdir $clientdir($i)
		set id($i) [expr 2 + $i]
		repladd $id($i)
		set cl_envcmd($i) "berkdb_env_noerr -create -txn nosync \
		    -home $clientdir($i) $verbargs -errpfx CLIENT.$i \
		    $repmemargs \
		    -rep_client -rep_transport \[list $id($i) replsend\]"
		set clenv($i) [eval $cl_envcmd($i) $recargs]
		error_check_good client_env [is_valid_env $clenv($i)] TRUE
	}
	set testfile "test$tnum.db"
	set omethod [convert_method $method]
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
	puts "\tRep$tnum.a: Running test001 in replicated env."
	eval rep_test $method $menv $masterdb $niter 0 0 0 0 $largs
	process_msgs $envlist

	error_check_good mdb_cl [$masterdb close] 0
	# Process any close messages.
	process_msgs $envlist

	error_check_good menv_cl [$menv close] 0
	puts "\tRep$tnum.b: Close all envs and run recovery in clients."
	for {set i 0} {$i < $nclients} {incr i} {
		error_check_good cl$i.close [$clenv($i) close] 0
		set hargs($i) "-h $clientdir($i)"
	}
	foreach sleep {2 1 0} {
		for {set i 0} {$i < $nclients} {incr i} {
			set stat [catch {eval exec $util_path/db_recover \
			    $hargs($i)} result]
			error_check_good stat $stat 0
			#
			# Need to sleep to make sure recovery's checkpoint
			# records have different timestamps.
			tclsleep $sleep
		}
	}

	puts "\tRep$tnum.c: Reopen clients and declare one master."
	for {set i 0} {$i < $nclients} {incr i} {
		set clenv($i) [eval $cl_envcmd($i) $recargs]
		error_check_good client_env [is_valid_env $clenv($i)] TRUE
	}
	error_check_good master0 [$clenv(0) rep_start -master] 0

	puts "\tRep$tnum.d: Sync up with other clients."
	while { 1 } {
		set nproced 0

		for {set i 0} {$i < $nclients} {incr i} {
			incr nproced [replprocessqueue $clenv($i) $id($i)]
		}

		if { $nproced == 0 } {
			break
		}
	}
	puts "\tRep$tnum.e: Verify client logs match."
	set i 0
	error_check_good cl$i.close [$clenv($i) close] 0
	set stat [catch {eval exec $util_path/db_printlog \
	    $hargs($i) >& $clientdir($i)/prlog} result]
	#
	# Note we start the loop at 1 here and compare against client0
	# which became the master.
	#
	for {set i 1} {$i < $nclients} {incr i} {
		error_check_good cl$i.close [$clenv($i) close] 0
		fileremove -f $clientdir($i)/prlog
		set stat [catch {eval exec $util_path/db_printlog \
		    $hargs($i) >> $clientdir($i)/prlog} result]
		error_check_good stat_prlog $stat 0
		error_check_good log_cmp(0,$i) \
		    [filecmp $clientdir(0)/prlog $clientdir($i)/prlog] 0
	}

	replclose $testdir/MSGQUEUEDIR
	set testdir $orig_tdir
	return
}

