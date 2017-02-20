# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST  rep048
# TEST	Replication and log gap bulk transfers.
# TEST	Have two master env handles.  Turn bulk on in
# TEST	one (turns it on for both).  Turn it off in the other.
# TEST	While toggling, send log records from both handles.
# TEST	Process message and verify master and client match.
#
proc rep048 { method { nentries 3000 } { tnum "048" } args } {
	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory

	if { $checking_valid_methods } {
		return "ALL"
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
	foreach r $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Skipping test with -recover for \
				    in-memory logs."
				continue 
			}
			puts "Rep$tnum ($method $r): Replication\
			    and toggling bulk transfer $msg $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep048_sub $method $nentries $tnum $l $r $args
		}
	}
}

proc rep048_sub { method niter tnum logset recargs largs } {
	source ./include.tcl
	global overflowword1
	global overflowword2
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

	set orig_tdir $testdir
	env_cleanup $testdir

	replsetup $testdir/MSGQUEUEDIR
	set overflowword1 "0"
	set overflowword2 "0"

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	file mkdir $clientdir
	file mkdir $masterdir

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]

	set in_memory_log \
	    [expr { $m_logtype == "in-memory" || $c_logtype == "in-memory" }]

	# In-memory logs require a large log buffer, and can not
	# be used with -txn nosync.  Adjust the args for master
	# and client.
	# This test has a long transaction, allocate a larger log 
	# buffer for in-memory test.
	set m_logargs [adjust_logargs $m_logtype [expr 20 * 1024 * 1024]]
	set c_logargs [adjust_logargs $c_logtype [expr 20 * 1024 * 1024]]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs $m_logargs \
	    -errpfx MASTER $verbargs -home $masterdir $repmemargs \
	    -rep_master -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs]
	error_check_good master_env [is_valid_env $masterenv] TRUE

	# Open a client.
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs $c_logargs \
	    -errpfx CLIENT $verbargs -home $clientdir $repmemargs \
	    -rep_client -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd $recargs]
	error_check_good client_env [is_valid_env $clientenv] TRUE

	# Bring the client online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	puts "\tRep$tnum.a: Create and open master databases"
	if { $databases_in_memory } {
		set dbname { "" "test.db" }
	} else { 
		set dbname "test.db"
	} 

	set omethod [convert_method $method]
	set masterdb [eval {berkdb_open_noerr -env $masterenv -auto_commit \
	    -create -mode 0644} $largs $omethod $dbname]
	error_check_good dbopen [is_valid_db $masterdb] TRUE

	set scrlog $testdir/repscript.log
	puts "\tRep$tnum.b: Fork child process."
	set pid [exec $tclsh_path $test_path/wrap.tcl \
	    rep048script.tcl $scrlog $masterdir $databases_in_memory &]

	# Wait for child process to start up.
	while { 1 } {
		if { [file exists $masterdir/marker.file] == 0  } {
			tclsleep 1
		} else {
			tclsleep 1
			break
		}
	}
	# Run a modified test001 in the master (and update clients).
	# Call it several times so make sure that we get descheduled.
	puts "\tRep$tnum.c: Basic long running txn"
	set div 10
	set loop [expr $niter / $div]
	set start 0
	for { set i 0 } { $i < $div } {incr i} {
		rep_test_bulk $method $masterenv $masterdb $loop $start $start 0
		process_msgs $envlist
		set start [expr $start + $loop]
		tclsleep 1
	}
	error_check_good dbclose [$masterdb close] 0
	set marker [open $masterdir/done.file w]
	close $marker

	set bulkxfer1 [stat_field $masterenv rep_stat "Bulk buffer transfers"]
	error_check_bad bulk $bulkxfer1 0

	puts "\tRep$tnum.d: Waiting for child ..."
	# Watch until the child is done.
	watch_procs $pid 5
	process_msgs $envlist
	set childname "child.db"

	rep_verify $masterdir $masterenv $clientdir $clientenv \
	    $in_memory_log 1 1
	rep_verify $masterdir $masterenv $clientdir $clientenv \
	    0 1 0 $childname

	error_check_good mclose [$masterenv close] 0
	error_check_good cclose [$clientenv close] 0
	replclose $testdir/MSGQUEUEDIR
}
