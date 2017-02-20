# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep051
# TEST	Test of compaction with replication.
# TEST
# TEST	Run rep_test in a replicated master environment.
# TEST	Delete a large number of entries and compact with -freespace.
# TEST	Propagate the changes to the client and make sure client and
# TEST	master match.

proc rep051 { method { niter 1000 } { tnum "051" } args } {
	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win9x platform."
		return
	}

	# Compaction is an option for btree and recno databases only.
	if { $checking_valid_methods } {
		set test_methods {}
		foreach method $valid_methods {
			if { [is_btree $method] == 1 || \
			    [is_recno $method] == 1 } {
				lappend test_methods $method
			}
		}
		return $test_methods
	}
	if { [is_hash $method] == 1 || [is_queue $method] == 1 } {
		puts "Skipping test$tnum for method $method."
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

	# Run tests with and without recovery.  If we're doing testing
	# of in-memory logging, skip the combination of recovery
	# and in-memory logging -- it doesn't make sense.
	set logsets [create_logsets 2]
	set saved_args $args

	foreach recopt $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $recopt == "-recover" && $logindex != -1 } {
				puts "Skipping test \
				    with -recover for in-memory logs."
				continue
			}
			set envargs ""
			set args $saved_args
			puts "Rep$tnum: Replication with\
			    compaction ($method $recopt) $msg $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep051_sub $method \
			    $niter $tnum $envargs $l $recopt $args
		}
	}
}

proc rep051_sub { method niter tnum envargs logset recargs largs } {
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

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]

	set verify_subset \
	    [expr { $m_logtype == "in-memory" || $c_logtype == "in-memory" }]

	# In-memory logs require a large log buffer, and cannot
	# be used with -txn nosync.  Adjust the args for master
	# and client.
	# This test has a long transaction, allocate a larger log 
	# buffer for in-memory test.
	set m_logargs [adjust_logargs $m_logtype [expr 2 * [expr 1024 * 1024]]]
	set c_logargs [adjust_logargs $c_logtype [expr 2 * [expr 1024 * 1024]]]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Open a master.
	repladd 1
	set env_cmd(M) "berkdb_env_noerr -create $verbargs \
	    -log_max 1000000 $envargs $m_logargs $recargs $repmemargs \
	    -home $masterdir -errpfx MASTER $m_txnargs -rep_master \
	    -rep_transport \[list 1 replsend\]"
	set masterenv [eval $env_cmd(M)]

	# Open a client
	repladd 2
	set env_cmd(C) "berkdb_env_noerr -create $verbargs \
	    -log_max 1000000 $envargs $c_logargs $recargs $repmemargs \
	    -home $clientdir -errpfx CLIENT $c_txnargs -rep_client \
	    -rep_transport \[list 2 replsend\]"
	set clientenv [eval $env_cmd(C)]

	# Bring the client online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	# Explicitly create the db handle so we can do deletes,
	# and also to make the page size small.
	if { $databases_in_memory } {
		set dbname { "" "test.db" }
	} else { 
		set dbname "test.db"
	} 

	set omethod [convert_method $method]
	set db [eval {berkdb_open_noerr -env $masterenv -auto_commit\
	    -pagesize 512 -create -mode 0644} $largs $omethod $dbname]
 	error_check_good db_open [is_valid_db $db] TRUE

	# Run rep_test in the master and update client.
	puts "\tRep$tnum.a:\
	    Running rep_test in replicated env ($envargs $recargs)."

	eval rep_test $method $masterenv $db $niter 0 0 0 $largs
	process_msgs $envlist

	# Verify that contents match.
	puts "\tRep$tnum.b: Verifying client database contents."
	rep_verify $masterdir $masterenv\
	    $clientdir $clientenv $verify_subset 1 1

	# Delete most entries.  Since some of our methods renumber,
	# delete starting at $niter and working down to 0.
	puts "\tRep$tnum.c: Remove most entries, by cursor."
	set count [expr $niter - 1]
	set n 20
	set t [$masterenv txn]
	error_check_good txn [is_valid_txn $t $masterenv] TRUE
	set txn "-txn $t"

	set dbc [eval {$db cursor} $txn]

	# Leave every nth item.
	set dbt [$dbc get -first]
	while { $count > 0 } {
		if { [expr $count % $n] != 0 } {
			error_check_good dbc_del [$dbc del] 0
		}
		set dbt [$dbc get -next]
		incr count -1
	}

	error_check_good dbc_close [$dbc close] 0
	error_check_good t_commit [$t commit] 0

	# Open read-only handle on client, so we can call $db stat.
	set client_db \
	    [eval {berkdb_open_noerr} -env $clientenv -rdonly $dbname]
 	error_check_good client_open [is_valid_db $client_db] TRUE

	# Check database size on both client and master.
	process_msgs $envlist
	set master_pages_before [stat_field $db stat "Page count"]
	set client_pages_before [stat_field $client_db stat "Page count"]
	error_check_good \
	    pages_match_before $client_pages_before $master_pages_before

	# Compact database.
	puts "\tRep$tnum.d: Compact database."
	set t [$masterenv txn]
	error_check_good txn [is_valid_txn $t $masterenv] TRUE
	set txn "-txn $t"

	set ret [eval {$db compact} $txn {-freespace}]

	error_check_good t_commit [$t commit] 0
	error_check_good db_sync [$db sync] 0

	# There will be fewer pages in use after the compact -freespace call.
	set master_pages_after [stat_field $db stat "Page count"]
	set page_reduction [expr $master_pages_before - $master_pages_after]
	error_check_good page_reduction [expr $page_reduction > 0] 1
	
	# Process messages so the client sees the reduction in pages used.
	process_msgs $envlist

	set client_pages_after [stat_field $client_db stat "Page count"]
	error_check_good \
	    pages_match_after $client_pages_after $master_pages_after

	# Close client handle.
	error_check_good client_handle [$client_db close] 0

	# Reverify.
	puts "\tRep$tnum.b: Verifying client database contents."
	rep_verify $masterdir $masterenv\
	    $clientdir $clientenv $verify_subset 1 1

	# Clean up.
	error_check_good db_close [$db close] 0
	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	replclose $testdir/MSGQUEUEDIR
}
