# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep031
# TEST	Test of internal initialization and blocked operations.
# TEST
# TEST	One master, one client.
# TEST	Put one more record to the master.
# TEST  Test that internal initialization blocks:
# TEST  log_archive, rename, remove, fileid_reset, lsn_reset.
# TEST	Sleep 30+ seconds.
# TEST  Test that blocked operations are now unblocked.
#
proc rep031 { method { niter 200 } { tnum "031" } args } {

	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}

	# There is nothing method-sensitive in this test, so
	# skip for all except btree.
	if { $checking_valid_methods } {
		set test_methods { btree }
		return $test_methods
	}
	if { [is_btree $method] != 1 } {
		puts "Skipping rep031 for method $method."
		return
	}

	set args [convert_args $method $args]
	set logsets [create_logsets 2]

	# This test needs to set its own pagesize.
	set pgindex [lsearch -exact $args "-pagesize"]
        if { $pgindex != -1 } {
                puts "Rep$tnum: skipping for specific pagesizes"
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
	# and with and without cleaning.  Skip recovery with in-memory
	# logging - it doesn't make sense.
	set cleanopts { clean noclean }
	foreach r $test_recopts {
		foreach c $cleanopts {
			foreach l $logsets {
				set logindex [lsearch -exact $l "in-memory"]
				if { $r == "-recover" && $logindex != -1 } {
					puts "Skipping rep$tnum for -recover\
					    with in-memory logs."
					continue
				}
				puts "Rep$tnum ($method $r $c $args):\
				    Test of internal init and blocked\
				    operations $msg $msg2."
				puts "Rep$tnum: Master logs are [lindex $l 0]"
				puts "Rep$tnum: Client logs are [lindex $l 1]"
				rep031_sub $method $niter $tnum $l $r $c $args
			}
		}
	}
}

proc rep031_sub { method niter tnum logset recargs clean largs } {
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
	set log_max [expr $pagesize * 8]

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]

	# In-memory logs cannot be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs \
	    $m_logargs -log_max $log_max $verbargs $repmemargs \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs -rep_master]

	# Open a client
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs \
	    $c_logargs -log_max $log_max $verbargs $repmemargs \
	    -home $clientdir -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd $recargs -rep_client]

	# Bring the clients online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	$masterenv test force noarchive_timeout

	# Run rep_test in the master (and update client).
	puts "\tRep$tnum.a: Running rep_test in replicated env."
	set start 0
	eval rep_test $method $masterenv NULL $niter $start $start 0 $largs
	incr start $niter
	process_msgs $envlist

	puts "\tRep$tnum.b: Close client."
	# Find out what exists on the client.  We need to loop until
	# the first master log file > last client log file.
	if { $c_logtype != "in-memory" } {
		set res [eval exec $util_path/db_archive -l -h $clientdir]
	}
	set last_client_log [get_logfile $clientenv last]
	error_check_good client_close [$clientenv close] 0

	set stop 0
	while { $stop == 0 } {
		# Run rep_test in the master (don't update client).
		puts "\tRep$tnum.c: Running rep_test in replicated env."
	 	eval rep_test \
		    $method $masterenv NULL $niter $start $start 0 $largs
		incr start $niter
		replclear 2

		puts "\tRep$tnum.d: Run db_archive on master."
		if { $m_logtype != "in-memory"} {
			set res \
			    [eval exec $util_path/db_archive -d -h $masterdir]
		}
		set first_master_log [get_logfile $masterenv first]
		if { $first_master_log > $last_client_log } {
			set stop 1
		}
	}

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
		eval rep_test \
		    $method $masterenv NULL $entries $start $start 0 $largs
		incr start $entries
		process_msgs $envlist 0 NONE err
	}

	#
	# We have now forced an internal initialization.  Verify it is correct.
	#
	puts "\tRep$tnum.f: Verify logs and databases"
	rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 1

	#
	# Internal initializations disable certain operations on the master for
	# 30 seconds after the last init-related message is received
	# by the master.  Those operations are dbremove, dbrename and
	# log_archive (with removal).
	#
	puts "\tRep$tnum.g: Try to remove and rename the database."
	set dbname "test.db"
	set old $dbname
	set new $dbname.new
	if { $databases_in_memory } {
		set stat [catch {$masterenv dbrename -auto_commit "" $old $new} ret]
	} else {
		set stat [catch {$masterenv dbrename -auto_commit $old $new} ret]
	}
	error_check_good rename_fail $stat 1
	error_check_good rename_err [is_substr $ret "invalid"] 1
	if { $databases_in_memory } {
		set stat [catch {$masterenv dbremove -auto_commit "" $old} ret]
	} else {
		set stat [catch {$masterenv dbremove -auto_commit $old} ret]
	}
	error_check_good remove_fail $stat 1
	error_check_good remove_err [is_substr $ret "invalid"] 1

	# The fileid_reset and lsn_reset operations work on physical files 
	# so we do not need to test them for in-memory databases.
	if { $databases_in_memory != 1 } {
		puts "\tRep$tnum.h: Try to reset LSNs and fileid on the database."
		set stat [catch {$masterenv id_reset $old} ret]
		error_check_good id_reset $stat 1
		error_check_good id_err [is_substr $ret "invalid"] 1
		set stat [catch {$masterenv lsn_reset $old} ret]
		error_check_good lsn_reset $stat 1
		error_check_good lsn_err [is_substr $ret "invalid"] 1
	}

	#
	# Need entries big enough to generate additional log files.
	# However, db_archive will not return an error, it will
	# just retain the log file.
	#
	puts "\tRep$tnum.i: Run rep_test to generate more logs."
	set entries 200
	eval rep_test $method $masterenv NULL $entries $start $start 0 $largs
	incr start $entries
	process_msgs $envlist 0 NONE err

	# Test lockout of archiving only in on-disk case.
	if { $m_logtype != "in-memory" } {
		puts "\tRep$tnum.j: Try to db_archive."
		set res [eval exec $util_path/db_archive -l -h $masterdir]
		set first [lindex $res 0]
		set res [eval exec $util_path/db_archive -d -h $masterdir]
		set res [eval exec $util_path/db_archive -l -h $masterdir]
		error_check_bad log.gone [lsearch -exact $res $first] -1

		puts "\tRep$tnum.j.0: Try to log_archive in master env."
		set res [$masterenv log_archive -arch_remove]
		set res [eval exec $util_path/db_archive -l -h $masterdir]
		error_check_bad log.gone0 [lsearch -exact $res $first] -1

		# We can't open a second handle on the env in HP-UX.
		if { $is_hp_test != 1 } {
			puts "\tRep$tnum.j.1: Log_archive in new non-rep env."
			set newenv [berkdb_env_noerr -txn nosync \
			    -log_max $log_max -home $masterdir]
			error_check_good newenv [is_valid_env $newenv] TRUE
			set res [$newenv log_archive -arch_remove]
			set res [eval exec \
			    $util_path/db_archive -l -h $masterdir]
			error_check_bad \
			    log.gone1 [lsearch -exact $res $first] -1
		}
	}

	# Check that databases are in-memory or on-disk as expected, before
	# we try to delete the databases!
	check_db_location $masterenv
	check_db_location $clientenv

	set timeout 30
	#
	# Sleep timeout+2 seconds - The timeout is 30 seconds, but we need
	# to sleep a bit longer to make sure we cross the timeout.
	#
	set to [expr $timeout + 2]
	puts "\tRep$tnum.k: Wait $to seconds to timeout"
	tclsleep $to
	puts "\tRep$tnum.l: Retry blocked operations after wait"
	if { $databases_in_memory == 1 } {
		set stat [catch {$masterenv dbrename -auto_commit "" $old $new} ret]
		error_check_good rename_work $stat 0
		set stat [catch {$masterenv dbremove -auto_commit "" $new} ret]
		error_check_good remove_work $stat 0
	} else {
		set stat [catch {$masterenv id_reset $old} ret]
		error_check_good id_reset_work $stat 0
		set stat [catch {$masterenv lsn_reset $old} ret]
		error_check_good lsn_reset_work $stat 0
		set stat [catch {$masterenv dbrename -auto_commit $old $new} ret]
		error_check_good rename_work $stat 0
		set stat [catch {$masterenv dbremove -auto_commit $new} ret]
		error_check_good remove_work $stat 0
	}
	process_msgs $envlist 0 NONE err

	if { $m_logtype != "in-memory" } {
		# Remove files via the 2nd non-rep env, check via db_archive.
		if { $is_hp_test != 1 } {
			set res [$newenv log_archive -arch_remove]
			set res \
			    [eval exec $util_path/db_archive -l -h $masterdir]
			error_check_good \
			    log.gone [lsearch -exact $res $first] -1
			error_check_good newenv_close [$newenv close] 0
		} else {
			set res [$masterenv log_archive -arch_remove]
			set res \
			    [eval exec $util_path/db_archive -l -h $masterdir]
			error_check_good \
			    log.gone [lsearch -exact $res $first] -1
		}
	}
	
	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	replclose $testdir/MSGQUEUEDIR
}
