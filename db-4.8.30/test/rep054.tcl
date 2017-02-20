# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep054
# TEST	Test of internal initialization where a far-behind
# TEST	client takes over as master.
# TEST
# TEST	One master, two clients.
# TEST	Run rep_test and process.
# TEST	Close client 1.
# TEST	Run rep_test, opening new databases, and processing
# TEST	messages.  Archive as we go so that log files get removed.
# TEST	Close master and reopen client 1 as master.  Process messages.
# TEST	Verify that new master and client are in sync.
# TEST	Run rep_test again, adding data to one of the new
# TEST	named databases.

proc rep054 { method { nentries 200 } { tnum "054" } args } {
	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory

	# Valid for all access methods.
	if { $checking_valid_methods } {
		return "ALL"
	}

	# Skip this test for named in-memory databases; it tries
	# to close and re-open envs, which just won't work.
	if { $databases_in_memory } {
		puts "Skipping Rep$tnum for in-memory databases."
		return
	}

	# This test needs to set its own pagesize.
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Rep$tnum: skipping for specific pagesizes"
		return
	}

	set args [convert_args $method $args]
	set logsets [create_logsets 3]

	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	# Run the body of the test with and without recovery,
	# and with and without cleaning.  Skip recovery with in-memory
	# logging - it doesn't make sense.
	foreach r $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Skipping rep$tnum for -recover\
				    with in-memory logs."
				continue
			}
			puts "Rep$tnum ($method $r $args):  Internal\
			    initialization test: far-behind client\
			    becomes master $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			puts "Rep$tnum: Client2 logs are [lindex $l 2]"

			rep054_sub $method $nentries $tnum $l $r $args
		}
	}
}

proc rep054_sub { method nentries tnum logset recargs largs } {
	global testdir
	global util_path
	global errorInfo
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
	set omethod [convert_method $method]

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR2

	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $clientdir2

	# Log size is small so we quickly create more than one.
	# The documentation says that the log file must be at least
	# four times the size of the in-memory log buffer.
	set pagesize 4096
	append largs " -pagesize $pagesize "
	set log_max [expr $pagesize * 8]

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]
	set c2_logtype [lindex $logset 2]

	# In-memory logs cannot be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set c2_logargs [adjust_logargs $c2_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]
	set c2_txnargs [adjust_txnargs $c2_logtype]

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs \
	    $m_logargs -log_max $log_max $verbargs $repmemargs \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs -rep_master]
	error_check_good master_env [is_valid_env $masterenv] TRUE

	# Open a client
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs \
	    $c_logargs -log_max $log_max $verbargs $repmemargs \
	    -home $clientdir -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd $recargs -rep_client]
	error_check_good client_env [is_valid_env $clientenv] TRUE

	# Open 2nd client
	repladd 3
	set cl2_envcmd "berkdb_env_noerr -create $c2_txnargs \
	    $c2_logargs -log_max $log_max $verbargs $repmemargs \
	    -home $clientdir2 -rep_transport \[list 3 replsend\]"
	set clientenv2 [eval $cl2_envcmd $recargs -rep_client]
	error_check_good client2_env [is_valid_env $clientenv2] TRUE

	# Bring the clients online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2} {$clientenv2 3}"
	process_msgs $envlist

	# Clobber replication's 30-second anti-archive timer, which will have
	# been started by client sync-up internal init, so that we can do a
	# log_archive in a moment.
	#
	$masterenv test force noarchive_timeout

	# Run rep_test in the master and in each client.
	puts "\tRep$tnum.a: Running rep_test in master & clients."
	set start 0
	eval rep_test $method $masterenv NULL $nentries $start $start 0 $largs
	incr start $nentries
	process_msgs $envlist

	# Master is in sync with both clients.
	rep_verify $masterdir $masterenv $clientdir $clientenv

	# Process messages again in case we are running with debug_rop.
	process_msgs $envlist
	rep_verify $masterdir $masterenv $clientdir2 $clientenv2

	# Identify last log on client, then close.  Loop until the first
	# master log file is greater than the last client log file.
	set last_client_log [get_logfile $clientenv last]

	puts "\tRep$tnum.b: Close client 1."
	error_check_good client_close [$clientenv close] 0
	set envlist "{$masterenv 1} {$clientenv2 3}"

	set stop 0
	while { $stop == 0 } {
		# Run rep_test in the master (don't update client).
		puts "\tRep$tnum.c: Running rep_test in replicated env."
		eval rep_test $method $masterenv NULL $nentries \
		    $start $start 0 $largs
		incr start $nentries
		replclear 2

		puts "\tRep$tnum.d: Run db_archive on master."
		if { $m_logtype != "in-memory" } {
			set res [eval exec $util_path/db_archive -d -h $masterdir]
		}
		# Make sure we have a gap between the last client log and
		# the first master log.
		set first_master_log [get_logfile $masterenv first]
		if { $first_master_log > $last_client_log } {
			set stop 1
		}
	}

	# Create a database that does not even exist on client 1.
	set newfile "newtest.db"
	set newdb [eval {berkdb_open_noerr -env $masterenv -create \
	    -auto_commit -mode 0644} $largs $omethod $newfile]
	error_check_good newdb_open [is_valid_db $newdb] TRUE
	eval rep_test $method $masterenv $newdb $nentries $start $start 0 $largs
	set start [expr $start + $nentries]
	process_msgs $envlist

	# Identify last master log file.
	set res [eval exec $util_path/db_archive -l -h $masterdir]
	set last_master_log [get_logfile $masterenv last]
	set stop 0

	# Send the master and client2 far ahead of client 1.  Archive
	# so there will be a gap between the log files of the closed
	# client and the active master and client and we've
	# archived away the creation of the new database.
	puts "\tRep$tnum.e: Running rep_test in master & remaining client."
	while { $stop == 0 } {

		eval rep_test \
		    $method $masterenv NULL $nentries $start $start 0 $largs
		incr start $nentries

		process_msgs $envlist

		puts "\tRep$tnum.f: Send master ahead of closed client."
		if { $m_logtype != "in-memory" } {
			set res [eval exec $util_path/db_archive -d -h $masterdir]
		}
		if { $c2_logtype != "in-memory" } {
			set res [eval exec $util_path/db_archive -d -h $clientdir2]
		}
		set first_master_log [get_logfile $masterenv first]
		if { $first_master_log > $last_master_log } {
			set stop 1
		}
	}
	process_msgs $envlist

	# Master is in sync with client 2.
	rep_verify $masterdir $masterenv $clientdir2 $clientenv2 1

	# Close master.
	puts "\tRep$tnum.g: Close master."
	error_check_good newdb_close [$newdb close] 0
	error_check_good close_master [$masterenv close] 0

	# The new database is still there.
	error_check_good newfile_exists [file exists $masterdir/$newfile] 1

	puts "\tRep$tnum.h: Reopen client1 as master."
	replclear 2
	set newmasterenv [eval $cl_envcmd $recargs -rep_master]
	error_check_good newmasterenv [is_valid_env $newmasterenv] TRUE

	# Force something into the log
	$newmasterenv txn_checkpoint -force

	puts "\tRep$tnum.i: Reopen master as client."
	set oldmasterenv [eval $ma_envcmd $recargs -rep_client]
	error_check_good oldmasterenv [is_valid_env $oldmasterenv] TRUE
	set envlist "{$oldmasterenv 1} {$newmasterenv 2} {$clientenv2 3}"
	process_msgs $envlist

	rep_verify $clientdir $newmasterenv $masterdir $oldmasterenv 1

	error_check_good newmasterenv_close [$newmasterenv close] 0
	error_check_good oldmasterenv_close [$oldmasterenv close] 0
	error_check_good clientenv2_close [$clientenv2 close] 0
	replclose $testdir/MSGQUEUEDIR
}
