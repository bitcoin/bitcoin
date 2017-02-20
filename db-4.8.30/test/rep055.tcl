# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep055
# TEST	Test of internal initialization and log archiving.
# TEST
# TEST	One master, one client.
# TEST	Generate several log files.
# TEST	Remove old master log files and generate several more.
# TEST  Get list of archivable files from db_archive and restart client.
# TEST  As client is in the middle of internal init, remove
# TEST	the log files returned earlier by db_archive.
#
proc rep055 { method { niter 200 } { tnum "055" } args } {

	source ./include.tcl
	global mixed_mode_logging
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

	# This test needs to set its own pagesize.
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Rep$tnum: skipping for specific pagesizes"
		return
	}

	# This test is all about log archive issues, so don't run with
	# in-memory logging.
	if { $mixed_mode_logging > 0 } {
		puts "Rep$tnum: Skipping for mixed-mode logging."
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

	# Run the body of the test with and without recovery,
	# and with and without cleaning.
	set opts { clean noclean }
	foreach r $test_recopts {
		foreach c $opts {
			puts "Rep$tnum ($method $r $c $args):\
			    Test of internal initialization $msg $msg2."
			rep055_sub $method $niter $tnum $r $c $args

		}
	}
}

proc rep055_sub { method niter tnum recargs opts largs } {
	global testdir
	global passwd
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
	set pagesize 4096
	append largs " -pagesize $pagesize "
	set log_buf [expr $pagesize * 2]
	set log_max [expr $log_buf * 4]

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create -txn nosync $verbargs \
	    $repmemargs \
	    -log_buffer $log_buf -log_max $log_max -errpfx MASTER \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs -rep_master]
	$masterenv rep_limit 0 0

	# Open a client
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create -txn nosync $verbargs \
	    $repmemargs \
	    -log_buffer $log_buf -log_max $log_max -errpfx CLIENT \
	    -home $clientdir -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd $recargs -rep_client]
	$clientenv rep_limit 0 0

	# Bring the clients online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	# Clobber replication's 30-second anti-archive timer, which will have
	# been started by client sync-up internal init, so that we can do a
	# log_archive in a moment.
	#
	$masterenv test force noarchive_timeout

	# Run rep_test in the master (and update client).
	puts "\tRep$tnum.a: Running rep_test in replicated env."
	eval rep_test $method $masterenv NULL $niter 0 0 0 $largs
	process_msgs $envlist

	puts "\tRep$tnum.b: Close client."
	error_check_good client_close [$clientenv close] 0

	# Find out what exists on the client.  We need to loop until
	# the first master log file > last client log file.
	# This forces internal init to happen.

	set res [eval exec $util_path/db_archive -l -h $clientdir]
	set last_client_log [lindex [lsort $res] end]
	set stop 0
	while { $stop == 0 } {
		# Run rep_test in the master (don't update client).
		puts "\tRep$tnum.c: Running rep_test in replicated env."
		eval rep_test $method $masterenv NULL $niter 0 0 0 $largs
		replclear 2

		puts "\tRep$tnum.d: Run db_archive on master."
		set res [eval exec $util_path/db_archive -d -h $masterdir]
		set res [eval exec $util_path/db_archive -l -h $masterdir]
		if { [lsearch -exact $res $last_client_log] == -1 } {
			set stop 1
		}
	}

	# Find out what exists on the master.  We need to loop until
	# the master log changes.  This is required so that we can
	# have a log_archive waiting to happen.
	#
	set res [eval exec $util_path/db_archive -l -h $masterdir]
	set last_master_log [lindex [lsort $res] end]
	set stop 0
	puts "\tRep$tnum.e: Move master logs forward again."
	while { $stop == 0 } {
		# Run rep_test in the master (don't update client).
		eval rep_test $method $masterenv NULL $niter 0 0 0 $largs
		replclear 2

		set res [eval exec $util_path/db_archive -l -h $masterdir]
		set last_log [lindex [lsort $res] end]
		if { $last_log != $last_master_log } {
			set stop 1
		}
	}

	puts "\tRep$tnum.f: Get list of files for removal."
	set logs [eval exec $util_path/db_archive -h $masterdir]

	puts "\tRep$tnum.g: Reopen client ($opts)."
	if { $opts == "clean" } {
		env_cleanup $clientdir
	}
	set clientenv [eval $cl_envcmd $recargs -rep_client]
	error_check_good client_env [is_valid_env $clientenv] TRUE
	$clientenv rep_limit 0 0
	set envlist "{$masterenv 1} {$clientenv 2}"
	#
	# Process messages once to get partially through internal init.
	#
	proc_msgs_once $envlist NONE err

	if { $opts != "clean" } {
		puts "\tRep$tnum.g.1: Trigger log request"
		#
		# When we don't clean, starting the client doesn't
		# trigger any events.  We need to generate some log
		# records so that the client requests the missing
		# logs and that will trigger it.
		#
		set entries 10
		eval rep_test $method $masterenv NULL $entries $niter 0 0 $largs
		#
		# Process messages three times to get us into internal init
		# but not enough to get us all the way through it.
		#
		proc_msgs_once $envlist NONE err
		proc_msgs_once $envlist NONE err
		proc_msgs_once $envlist NONE err
	}

	#
	# Now in the middle of internal init, remove the log files
	# db_archive reported earlier.
	#
	foreach l $logs {
		fileremove -f $masterdir/$l
	}
	#
	# Now finish processing all the messages.
	#
	process_msgs $envlist 0 NONE err

	puts "\tRep$tnum.h: Verify logs and databases"
        rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 1

	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	replclose $testdir/MSGQUEUEDIR
}
