# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep036
# TEST	Multiple master processes writing to the database.
# TEST	One process handles all message processing.

proc rep036 { method { niter 200 } { tnum "036" } args } {

	source ./include.tcl
	global repfiles_in_memory

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}

	# Valid for btree only.
	if { $checking_valid_methods } {
		set test_methods { btree }
		return $test_methods
	}
	if { [is_btree $method] == 0 } {
		puts "Rep$tnum: Skipping for method $method."
		return
	}

	set saved_args $args
	set logsets [create_logsets 3]

	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	foreach l $logsets {
		set envargs ""
		set args $saved_args
		puts "Rep$tnum: Test sync-up recovery ($method) $msg2."
		puts "Rep$tnum: Master logs are [lindex $l 0]"
		puts "Rep$tnum: Client 0 logs are [lindex $l 1]"
		puts "Rep$tnum: Client 1 logs are [lindex $l 2]"
		rep036_sub $method $niter $tnum $envargs $l $args
	}
}

proc rep036_sub { method niter tnum envargs logset args } {
	source ./include.tcl
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

	# In-memory logs require a large log buffer.
	# We always run this test with -txn, so don't adjust txnargs.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]

	# Open a master.
	repladd 1
	set env_cmd(M) "berkdb_env_noerr -create $verbargs $repmemargs \
	    -log_max 1000000 $envargs -home $masterdir $m_logargs \
	    -errpfx MASTER -errfile /dev/stderr -txn -rep_master \
	    -rep_transport \[list 1 replsend\]"
	set env1 [eval $env_cmd(M)]

	# Open a client
	repladd 2
	set env_cmd(C) "berkdb_env_noerr -create $verbargs $repmemargs \
	    -log_max 1000000 $envargs -home $clientdir $c_logargs \
	    -errfile /dev/stderr -errpfx CLIENT -txn -rep_client \
	    -rep_transport \[list 2 replsend\]"
	set env2 [eval $env_cmd(C)]

	# Bring the client online by processing the startup messages.
	set envlist "{$env1 1} {$env2 2}"
	process_msgs $envlist

#	# Start up deadlock detector.
#	# Commented out, as are two more sections below - see [#15049].
#       set dpid [eval {exec $util_path/db_deadlock} \
#	    -a o -v -t 2.0 -h $masterdir >& $testdir/dd.parent.out &]
				 
	# Set up master database.
	set testfile "rep$tnum.db"
	set omethod [convert_method $method]
	set mdb [eval {berkdb_open_noerr} -env $env1 -auto_commit \
	    -create -mode 0644 $omethod $testfile]
	error_check_good dbopen [is_valid_db $mdb] TRUE

	# Put a record in the master database.
	set key MAIN_KEY
	set string MAIN_STRING
	set t [$env1 txn]
	error_check_good txn [is_valid_txn $t $env1] TRUE
	set txn "-txn $t"

	set ret [eval \
	    {$mdb put} $txn {$key [chop_data $method $string]}]
	error_check_good mdb_put $ret 0
	error_check_good txn_commit [$t commit] 0

	# Fork two writers that write to the master.
	set pidlist {}
	foreach writer { 1 2 } {
		puts "\tRep$tnum.a: Fork child process WRITER$writer."
		set pid [exec $tclsh_path $test_path/wrap.tcl \
		    rep036script.tcl $testdir/rep036script.log.$writer \
		    $masterdir $writer $niter btree &]
		lappend pidlist $pid
	}

	# Run the main loop until the writers signal completion.
	set i 0
	while { [file exists $testdir/1.db] == 0 && \
	   [file exists $testdir/2.db] == 0 } {
		set string MAIN_STRING.$i

		set t [$env1 txn]
		error_check_good txn [is_valid_txn $t $env1] TRUE
		set txn "-txn $t"
		set ret [eval \
			{$mdb put} $txn {$key [chop_data $method $string]}]

#		# Writing to this database can deadlock.  If we do, let the
#		# deadlock detector break the lock, wait a second, and try again.
#		while { [catch {eval {$mdb put}\
#		    $txn {$key [chop_data $method $string]}} ret] } {
#			# Make sure the failure is a deadlock.
#			error_check_good deadlock [is_substr $ret DB_LOCK_DEADLOCK] 1
#			tclsleep 1
#		}


		error_check_good mdb_put $ret 0
		error_check_good txn_commit [$t commit] 0

		if { [expr $i % 10] == 0 } {
			puts "\tRep036.c: Wrote MAIN record $i"
		}
		incr i

		# Process messages.
		process_msgs $envlist

		# Wait a while, then do it all again.
		tclsleep 1
	}


	# Confirm that the writers are done and process the messages
	# once more to be sure the client is caught up.
	watch_procs $pidlist 1
	process_msgs $envlist

#	# We are done with the deadlock detector.
#	error_check_good kill_deadlock_detector [tclkill $dpid] ""

	puts "\tRep$tnum.c: Verify logs and databases"
	# Check that master and client logs and dbs are identical.
	# Logs first ...
	set stat [catch {eval exec $util_path/db_printlog \
	    -h $masterdir > $masterdir/prlog} result]
	error_check_good stat_mprlog $stat 0
	set stat [catch {eval exec $util_path/db_printlog \
	    -h $clientdir > $clientdir/prlog} result]
	error_check_good mdb [$mdb close] 0
	error_check_good stat_cprlog $stat 0
#	error_check_good log_cmp \
#	    [filecmp $masterdir/prlog $clientdir/prlog] 0

	# ... now the databases.
	set db1 [eval {berkdb_open_noerr -env $env1 -rdonly $testfile}]
	set db2 [eval {berkdb_open_noerr -env $env2 -rdonly $testfile}]

	error_check_good comparedbs [db_compare \
	    $db1 $db2 $masterdir/$testfile $clientdir/$testfile] 0
	error_check_good db1_close [$db1 close] 0
	error_check_good db2_close [$db2 close] 0

	error_check_good env1_close [$env1 close] 0
	error_check_good env2_close [$env2 close] 0
	replclose $testdir/MSGQUEUEDIR
}
