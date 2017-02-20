# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST  	rep028
# TEST	Replication and non-rep env handles. (Also see rep006.)
# TEST
# TEST	Open second non-rep env on client, and create a db
# TEST	through this handle.  Open the db on master and put
# TEST	some data.  Check whether the non-rep handle keeps
# TEST	working.  Also check if opening the client database
# TEST	in the non-rep env writes log records.
#
proc rep028 { method { niter 100 } { tnum "028" } args } {

	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}

	# Run for btree only.
	if { $checking_valid_methods } {
		set test_methods { btree }
		return $test_methods
	}
	if { [is_btree $method] == 0 } {
		puts "\tRep$tnum: Skipping for method $method."
		return
	}

	# Skip test for HP-UX because we can't open an env twice.
	if { $is_hp_test == 1 } {
		puts "\tRep$tnum: Skipping for HP-UX."
		return
	}

	set args [convert_args $method $args]
	set logsets [create_logsets 2]

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
	set clopts { "create" "open" }
	foreach r $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Rep$tnum: Skipping\
				    for in-memory logs with -recover."
				continue
			}
			foreach c $clopts {
				puts "Rep$tnum ($method $r $c): Replication\
				    and non-rep env handles $msg $msg2."
				puts "Rep$tnum: Master logs are [lindex $l 0]"
				puts "Rep$tnum: Client logs are [lindex $l 1]"
				rep028_sub $method $niter $tnum $l $r $c $args
			}
		}
	}
}

proc rep028_sub { method niter tnum logset recargs clargs largs } {
	source ./include.tcl
	global is_hp_test
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

	set omethod [convert_method $method]
	env_cleanup $testdir

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR

	file mkdir $masterdir
	file mkdir $clientdir

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]

	# In-memory logs require a large log buffer, and cannot
	# be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Open a master.
	puts "\tRep$tnum.a: Open replicated envs and non-replicated client env."
	repladd 1
	set env_cmd(M) "berkdb_env_noerr -create \
	    -log_max 1000000 -home $masterdir $verbargs $repmemargs \
	    $m_txnargs $m_logargs -rep_master \
	    -rep_transport \[list 1 replsend\]"
	set masterenv [eval $env_cmd(M) $recargs]

	# Open a client
	repladd 2
	set env_cmd(C) "berkdb_env_noerr -create $c_txnargs \
	    $c_logargs -home $clientdir $verbargs $repmemargs \
	    -rep_transport \[list 2 replsend\]"
	set clientenv [eval $env_cmd(C) $recargs]

	# Open 2nd non-replication handle on client env, and create
	# a db.  Note, by not specifying any subsystem args, we
	# do a DB_JOINENV, which is what we want.
	set nonrepenv [eval {berkdb_env_noerr -home $clientdir}]
	error_check_good nonrepenv [is_valid_env $nonrepenv] TRUE

	# Set up databases in-memory or on-disk.
	if { $databases_in_memory } {
		set dbname { "" "test.db" }
	} else { 
		set dbname "test.db"
	} 

	# If we're testing create, verify that if a non-rep client
	# creates a database before the master does, then when that
	# client goes to use it, it gets DB_DEAD_HANDLE.
	#
	if { $clargs == "create" } {
		puts "\tRep$tnum.b: Create database non-replicated."
		set let c
		set nextlet d
		set nonrepdb [eval berkdb_open_noerr -auto_commit \
		    -create $omethod -env $nonrepenv $dbname]
		error_check_good nonrepdb_open [is_valid_db $nonrepdb] TRUE
		tclsleep 2
	} else {
		set let b
		set nextlet c
	}

	#
	# Now declare the clientenv a client.
	#
	puts "\tRep$tnum.$let: Declare env as rep client"
	error_check_good client [$clientenv rep_start -client] 0
	if { $clargs == "create" } {
		#
		# We'll only catch this error if we turn on no-autoinit.
		# Otherwise, the system will throw away everything on the
		# client and resync.
		#
		$clientenv rep_config {noautoinit on}
	}
	
	# Bring the client online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist 0 NONE err
	#
	# In the create case, we'll detect the non-rep log records and
	# determine this client was never part of the replication group.
	#
	if { $clargs == "create" } {
		error_check_good errchk [is_substr $err \
		    "DB_REP_JOIN_FAILURE"] 1
		error_check_good close [$nonrepdb close] 0
	} else {
		# Open the same db through the master handle.  Put data
		# and process messages.
		set db [eval berkdb_open_noerr \
		    -create $omethod -env $masterenv -auto_commit $dbname]
		error_check_good db_open [is_valid_db $db] TRUE
		eval rep_test $method $masterenv $db $niter 0 0 0 $largs
		process_msgs $envlist

		#
		# If we're the open case, we want to just read the existing
		# database through a non-rep readonly handle.  Doing so
		# should not create log records on the client (but has
		# in the past).
		#
		puts "\tRep$tnum.$nextlet: Open and read database"
		set nonrepdb [eval berkdb_open \
		    -rdonly -env $nonrepenv $dbname]
		error_check_good nonrepdb_open [is_valid_db $nonrepdb] TRUE
		#
		# If opening wrote log records, we need to process
		# some more on the client to notice the end of log
		# is now in an unexpected place.
		#
		eval rep_test $method $masterenv $db $niter 0 0 0 $largs
		process_msgs $envlist
		error_check_good close [$nonrepdb close] 0

		# By passing in "NULL" for the database name, we compare
		# only the master and client logs, not the databases.
		rep_verify $masterdir $masterenv $clientdir $clientenv 0 0 1 NULL

#		set stat [catch {eval exec $util_path/db_printlog \
#		    -h $masterdir > $masterdir/prlog} result]
#		error_check_good stat_mprlog $stat 0
#		set stat [catch {eval exec $util_path/db_printlog \
#		    -h $clientdir > $clientdir/prlog} result]
#		error_check_good stat_cprlog $stat 0
#		error_check_good log_cmp \
#		    [filecmp $masterdir/prlog $clientdir/prlog] 0

		# Clean up.
		error_check_good db_close [$db close] 0

		# Check that databases are in-memory or on-disk as expected.
		check_db_location $nonrepenv
		check_db_location $masterenv
		check_db_location $clientenv
	}

	error_check_good nonrepenv_close [$nonrepenv close] 0
	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0

	replclose $testdir/MSGQUEUEDIR
}
