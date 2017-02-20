# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep033
# TEST	Test of internal initialization with rename and remove of dbs.
# TEST
# TEST	One master, one client.
# TEST	Generate several databases.  Replicate to client.
# TEST	Do some renames and removes, both before and after
# TEST	closing the client.
#
proc rep033 { method { niter 200 } { tnum "033" } args } {

	source ./include.tcl
	global databases_in_memory 
	global repfiles_in_memory

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}

	# Valid for all access methods.
	if { $checking_valid_methods } {
		return "ALL"
	}

	# This test depends on manipulating logs, so can not be run with
	# in-memory logging.
	global mixed_mode_logging
	if { $mixed_mode_logging > 0 } {
		puts "Rep$tnum: Skipping for mixed-mode logging."
		return
	}

	set args [convert_args $method $args]
	set omethod [convert_method $method]

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

	# Run the body of the test with and without recovery,
	# and with and without cleaning.
	set envargs ""
	set cleanopts { noclean clean }
	set when { before after }
	foreach r $test_recopts {
		foreach c $cleanopts {
			foreach w $when {
				puts "Rep$tnum ($method $envargs $c $r $w $args):\
				    Test of internal initialization $msg $msg2."
				rep033_sub $omethod $niter $tnum $envargs \
				    $r $c $w $args
			}
		}
	}
}

proc rep033_sub { method niter tnum envargs recargs clean when largs } {
	global testdir
	global util_path
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

	# Log size is small so we quickly create more than one.
	# The documentation says that the log file must be at least
	# four times the size of the in-memory log buffer.
	set pagesize 4096
	append largs " -pagesize $pagesize "
	set log_buf [expr $pagesize * 2]
	set log_max [expr $log_buf * 4]

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create -txn nosync \
	    -log_buffer $log_buf -log_max $log_max $envargs \
	    -errpfx MASTER $verbargs $repmemargs \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs -rep_master]

	# Open a client
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create -txn nosync \
	    -log_buffer $log_buf -log_max $log_max $envargs \
	    -errpfx CLIENT $verbargs $repmemargs \
	    -home $clientdir -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd $recargs -rep_client]

	# Bring the clients online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	# Clobber replication's 30-second anti-archive timer, which will have
	# been started by client sync-up internal init, so that we can do a
	# log_archive in a moment.
	#
	$masterenv test force noarchive_timeout

	# Set up for in-memory or on-disk databases.
	if { $databases_in_memory } {
		set memargs { "" }
	} else {
		set memargs ""
	}
	
	puts "\tRep$tnum.a: Create several databases on master."
	set oflags " -env $masterenv $method -create -auto_commit "
	set dbw [eval {berkdb_open_noerr} $oflags $largs $memargs w.db]
	set dbx [eval {berkdb_open_noerr} $oflags $largs $memargs x.db]
	set dby [eval {berkdb_open_noerr} $oflags $largs $memargs y.db]
	set dbz [eval {berkdb_open_noerr} $oflags $largs $memargs z.db]
	error_check_good dbw_close [$dbw close] 0
	error_check_good dbx_close [$dbx close] 0
	error_check_good dby_close [$dby close] 0
	error_check_good dbz_close [$dbz close] 0

	# Update client, then close.
	process_msgs $envlist

	puts "\tRep$tnum.b: Close client."
	error_check_good client_close [$clientenv close] 0

	# If we're doing the rename/remove operations before adding
	# databases A and B, manipulate only the existing files.
	if { $when == "before" } {
		rep033_rename_remove $masterenv
	}

	# Run rep_test in the master (don't update client).
	#
	# We'd like to control the names of these dbs, so give
	# rep_test an existing handle.
	#
	puts "\tRep$tnum.c: Create new databases.  Populate with rep_test."
	set dba [eval {berkdb_open_noerr} $oflags $largs $memargs a.db]
	set dbb [eval {berkdb_open_noerr} $oflags $largs $memargs b.db]
	eval rep_test $method $masterenv $dba $niter 0 0 0 $largs
	eval rep_test $method $masterenv $dbb $niter 0 0 0 $largs
	error_check_good dba_close [$dba close] 0
	error_check_good dbb_close [$dbb close] 0

	# Throw away messages for client.
	replclear 2

	# If we're doing the rename/remove afterwards, manipulate
	# all the files including A and B.
	if { $when == "after" } {
		rep033_rename_remove $masterenv
	}
	error_check_good rename_b [eval {$masterenv dbrename} $memargs b.db x.db] 0
	error_check_good remove_a [eval {$masterenv dbremove} $memargs a.db] 0

	puts "\tRep$tnum.d: Run db_archive on master."
	set res [eval exec $util_path/db_archive -l -h $masterdir]
	error_check_bad log.1.present [lsearch -exact $res log.0000000001] -1
	set res [eval exec $util_path/db_archive -d -h $masterdir]
	set res [eval exec $util_path/db_archive -l -h $masterdir]
	error_check_good log.1.gone [lsearch -exact $res log.0000000001] -1

	puts "\tRep$tnum.e: Reopen client ($clean)."
	if { $clean == "clean" } {
		env_cleanup $clientdir
	}
	set clientenv [eval $cl_envcmd $recargs -rep_client]
	error_check_good client_env [is_valid_env $clientenv] TRUE
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist 0 NONE err
	if { $clean == "noclean" } {
		puts "\tRep$tnum.e.1: Trigger log request"
		#
		# When we don't clean, starting the client doesn't
		# trigger any events.  We need to generate some log
		# records so that the client requests the missing
		# logs and that will trigger it.
		#
		set entries 10
		eval rep_test $method $masterenv NULL $entries $niter 0 0 $largs
		process_msgs $envlist 0 NONE err
	}

	puts "\tRep$tnum.f: Verify logs and databases"
	#
	# By sending in a NULL for dbname, we only compare logs.
	#
	rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 1 NULL
	#
	# ... now the databases, manually.  X, Y, and C should exist.
	#
	set dbnames "x.db w.db c.db"
	foreach db $dbnames {
		set db1 [eval \
		    {berkdb_open_noerr -env $masterenv} $largs -rdonly $memargs $db]
		set db2 [eval \
		    {berkdb_open_noerr -env $clientenv} $largs -rdonly $memargs $db]

		error_check_good compare:$db [db_compare \
		    $db1 $db2 $masterdir/$db $clientdir/$db] 0
		error_check_good db1_close [$db1 close] 0
		error_check_good db2_close [$db2 close] 0
	}

	# A, B, and Z should be gone on client.
	error_check_good dba_gone [file exists $clientdir/a.db] 0
	error_check_good dbb_gone [file exists $clientdir/b.db] 0
	#
	# Currently we cannot remove z.db on the client because
	# we don't own the file namespace.  So, we cannot do
	# the check below.  If that changes, we want the test below.
	error_check_good dbz_gone [file exists $clientdir/z.db] 0

	# Clean up.
	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	replclose $testdir/MSGQUEUEDIR
}

proc rep033_rename_remove { env } {
	global databases_in_memory 
	if { $databases_in_memory } {
		set memargs { "" }
	} else {
		set memargs ""
	}	

	# Here we manipulate databases W, X, Y, and Z.
	# Remove W.
	error_check_good remove_w [eval $env dbremove $memargs w.db] 0

	# Rename X to W, Y to C (an entirely new name).
	error_check_good rename_x [eval $env dbrename $memargs x.db w.db] 0
	error_check_good rename_y [eval $env dbrename $memargs y.db c.db] 0

	# Remove Z.
	error_check_good remove_z [eval $env dbremove $memargs z.db] 0
}
