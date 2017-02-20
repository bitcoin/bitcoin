# See the file LICENSE for redistribution information.
#
# Copyright (c) 2006-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep064
# TEST	Replication rename and forced-upgrade test.
# TEST
# TEST	The test verifies that the client correctly
# TEST	(internally) closes files when upgrading to master.
# TEST	It does this by having the master have a database
# TEST	open, then crashing.  The client upgrades to master,
# TEST	and attempts to remove the open database.

proc rep064 { method { niter 10 } { tnum "064" } args } {
	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win9x platform."
		return
	}

	# Run for btree only.  Since we're testing removal of a
	# file, method doesn't make any difference.
	if { $checking_valid_methods } {
		set test_methods { btree }
		return $test_methods
	}
	if { [is_btree $method] == 0 } {
		puts "Rep$tnum: Skipping for method $method."
		return
	}

	set logsets [create_logsets 2]
	set args [convert_args $method $args]

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
	foreach r $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Rep$tnum: Skipping\
				    for in-memory logs with -recover."
				continue
			}
			puts "Rep$tnum ($method $r): Replication test\
			    closure of open files on upgrade $msg $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep064_sub $method $niter $tnum $l $r $args
		}
	}
}

proc rep064_sub { method niter tnum logset recargs largs } {
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

	set m_logtype [lindex $logset 0]
	set m_logargs [adjust_logargs $m_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]

	set c_logtype [lindex $logset 1]
	set c_logargs [adjust_logargs $c_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs $m_logargs \
	    -errpfx MASTER -errfile /dev/stderr $verbargs $repmemargs \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs -rep_master]

	# Open a client
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs $c_logargs \
	    -errpfx CLIENT -errfile /dev/stderr $verbargs $repmemargs \
	    -home $clientdir -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd $recargs -rep_client]

	# Bring the clients online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	puts "\tRep$tnum.a: Open an empty db, and leave it open."
	# Set up names for the db to be left open and empty, and 
	# also for the db that we'll let rep_test open in part .b.
	if { $databases_in_memory } {
		set opendb { "" "open.db" }
		set testfile { "" "test.db" }
	} else { 
		set opendb "open.db"
		set testfile "test.db"
	} 

	set masterdb [eval {berkdb_open}\
	    -env $masterenv -create -btree -auto_commit $opendb]
	error_check_good db [is_valid_db $masterdb] TRUE
	process_msgs $envlist

	# Run a modified test001 in the master (and update client).
	puts "\tRep$tnum.b: Open another db, and add some data."
	eval rep_test $method $masterenv NULL $niter 0 0 $largs
	process_msgs $envlist

	# This simulates a master crashing, we're the only one in the
	# group.  No need to process messages.
	#
	puts "\tRep$tnum.c: Upgrade client."
	error_check_good client_upg [$clientenv rep_start -master] 0

	puts "\tRep$tnum.d: Remove open databases."
	set stat [catch {eval $clientenv dbremove -auto_commit $opendb} ret]
	error_check_good remove_open_file $ret 0
	error_check_good remove_open_file $stat 0

	set stat [catch {eval $clientenv dbremove -auto_commit $testfile} ret]
	error_check_good remove_closed_file $ret 0
	error_check_good remove_closed_file $stat 0

	error_check_good dbclose [$masterdb close] 0
	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	replclose $testdir/MSGQUEUEDIR
}
