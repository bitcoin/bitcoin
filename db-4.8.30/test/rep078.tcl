# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST  rep078
# TEST
# TEST	Replication and basic lease test.
# TEST	Set leases on master and 2 clients.
# TEST	Do a lease operation and process to all clients.
# TEST	Read with lease on master.  Do another lease operation
# TEST	and don't process on any client.  Try to read with
# TEST	on the master and verify it fails.  Process the messages
# TEST	to the clients and retry the read.
#
proc rep078 { method { tnum "078" } args } {
	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win9x platform."
		return
	}

	# Valid for all access methods.  Other lease tests limit the
	# test because there is nothing method-specific being tested.
	# Use all methods for this basic test.
	if { $checking_valid_methods } {
		return "ALL"
	}

	set args [convert_args $method $args]
	set logsets [create_logsets 3]

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
	#
	# Also skip the case where the master is in-memory and at least
	# one of the clients is on-disk.  If the master is in-memory,
	# the wrong site gets elected because on-disk envs write a log 
	# record when they create the env and in-memory ones do not
	# and the test wants to control which env gets elected.
	#
	foreach r $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Skipping rep$tnum for -recover\
				    with in-memory logs."
				continue
			}
			set master_logs [lindex $l 0]
			if { $master_logs == "in-memory" } {
				set client_logs [lsearch -exact $l "on-disk"]
				if { $client_logs != -1 } {
					puts "Skipping for in-memory master\
					    and on-disk client."
					continue
				}
			}

			puts "Rep$tnum ($method $r): Replication\
			    and basic master leases $msg $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client 1 logs are [lindex $l 1]"
			puts "Rep$tnum: Client 2 logs are [lindex $l 2]"
			rep078_sub $method $tnum $l $r $args
		}
	}
}

proc rep078_sub { method tnum logset recargs largs } {
	source ./include.tcl
	global testdir
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

	set qdir $testdir/MSGQUEUEDIR
	replsetup $qdir

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR2

	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $clientdir2

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]
	set c2_logtype [lindex $logset 2]

	# In-memory logs require a large log buffer, and cannot
	# be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set c2_logargs [adjust_logargs $c2_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]
	set c2_txnargs [adjust_txnargs $c2_logtype]

	# Set leases for 3 sites, 3 second timeout, 0% clock skew
	set nsites 3
	set lease_to 3000000
	set lease_tosec [expr $lease_to / 1000000]
	set clock_fast 0
	set clock_slow 0
	set testfile test.db
	#
	# Since we have to use elections, the election code
	# assumes a 2-off site id scheme.
	# Open a master.
	repladd 2
	set err_cmd(0) "none"
	set crash(0) 0
	set pri(0) 100
	#
	# Note that using the default clock skew should be the same
	# as specifying "no skew" through the API.  We want to
	# test both API usages here.
	#
	set envcmd(0) "berkdb_env -create $m_txnargs $m_logargs \
	    $verbargs -errpfx MASTER -home $masterdir \
	    -rep_lease \[list $nsites $lease_to\] \
	    -event rep_event $repmemargs \
	    -rep_client -rep_transport \[list 2 replsend\]"
	set masterenv [eval $envcmd(0) $recargs]
	error_check_good master_env [is_valid_env $masterenv] TRUE

	# Open two clients.
	repladd 3
	set err_cmd(1) "none"
	set crash(1) 0
	set pri(1) 10
	set envcmd(1) "berkdb_env -create $c_txnargs $c_logargs \
	    $verbargs -errpfx CLIENT -home $clientdir \
	    -rep_lease \[list $nsites $lease_to $clock_fast $clock_slow\] \
	    -event rep_event $repmemargs \
	    -rep_client -rep_transport \[list 3 replsend\]"
	set clientenv [eval $envcmd(1) $recargs]
	error_check_good client_env [is_valid_env $clientenv] TRUE

	repladd 4
	set err_cmd(2) "none"
	set crash(2) 0
	set pri(2) 10
	set envcmd(2) "berkdb_env -create $c2_txnargs $c2_logargs \
	    $verbargs -errpfx CLIENT2 -home $clientdir2 \
	    -rep_lease \[list $nsites $lease_to\] \
	    -event rep_event $repmemargs \
	    -rep_client -rep_transport \[list 4 replsend\]"
	set clientenv2 [eval $envcmd(2) $recargs]
	error_check_good client_env [is_valid_env $clientenv2] TRUE

	# Bring the clients online by processing the startup messages.
	set envlist "{$masterenv 2} {$clientenv 3} {$clientenv2 4}"
	process_msgs $envlist

	#
	# Run election to get a master.  Leases prevent us from
	# simply assigning a master.
	#
	set msg "Rep$tnum.a"
	puts "\tRep$tnum.a: Run initial election."
	set nvotes $nsites
	set winner 0
	setpriority pri $nsites $winner
	set elector [berkdb random_int 0 2]
	#
	# Note we send in a 0 for nsites because we set nsites back
	# when we started running with leases.  Master leases require
	# that nsites be set before calling rep_start, and master leases
	# require that the nsites arg to rep_elect be 0.
	#
	run_election envcmd envlist err_cmd pri crash $qdir $msg \
	    $elector 0 $nvotes $nsites $winner 0 NULL

	puts "\tRep$tnum.b: Spawn a child tclsh to do txn work."
	set pid [exec $tclsh_path $test_path/wrap.tcl \
	    rep078script.tcl $testdir/rep078script.log \
		   $masterdir $testfile $method &]

	# Let child run, create database and put a txn into it.
	# Process messages while we wait for the child to complete
	# its txn so that the clients can grant leases.
	puts "\tRep$tnum.c: Wait for child to write txn."
	while { 1 } {
		if { [file exists $testdir/marker.db] == 0  } {
			tclsleep 1
		} else {
			set markerenv [berkdb_env -home $testdir -txn]
			error_check_good markerenv_open \
			    [is_valid_env $markerenv] TRUE
			set marker [berkdb_open -unknown -env $markerenv \
			    -auto_commit marker.db]
			set kd [$marker get CHILD1]
			while { [llength $kd] == 0 } {
				process_msgs $envlist
				tclsleep 1
				set kd [$marker get CHILD1]
			}
			process_msgs $envlist
			#
			# Child sends us the key it used as the data
			# of the CHILD1 key.
			#
			set key [lindex [lindex $kd 0] 1]
			break
		}
	}
	set masterdb [eval \
	    {berkdb_open_noerr -env $masterenv -rdonly $testfile}]
	error_check_good dbopen [is_valid_db $masterdb] TRUE

	process_msgs $envlist
	set omethod [convert_method $method]
	set clientdb [eval {berkdb_open_noerr \
	    -env $clientenv $omethod -rdonly $testfile}]
	error_check_good dbopen [is_valid_db $clientdb] TRUE

	set uselease ""
	set ignorelease "-nolease"
	puts "\tRep$tnum.d.0: Read with leases."
	check_leaseget $masterdb $key $uselease 0
	check_leaseget $clientdb $key $uselease 0
	puts "\tRep$tnum.d.1: Read ignoring leases."
	check_leaseget $masterdb $key $ignorelease 0
	check_leaseget $clientdb $key $ignorelease 0
	#
	# This should fail because the lease is expired and all
	# attempts by master to refresh it will not be processed.
	#
	set sleep [expr $lease_tosec + 1]
	puts "\tRep$tnum.e.0: Sleep $sleep secs to expire leases and read again."
	tclsleep $sleep
	#
	# Verify the master gets REP_LEASE_EXPIRED.  Verify that the
	# read on the client ignores leases and succeeds.
	#
	check_leaseget $masterdb $key $uselease REP_LEASE_EXPIRED
	check_leaseget $clientdb $key $uselease 0
	puts "\tRep$tnum.e.1: Read ignoring leases."
	check_leaseget $masterdb $key $ignorelease 0
	check_leaseget $clientdb $key $ignorelease 0

	error_check_good timestamp_done \
	    [$marker put PARENT1 [timestamp -r]] 0

	set kd [$marker get CHILD2]
	while { [llength $kd] == 0 } {
		process_msgs $envlist
		tclsleep 1
		set kd [$marker get CHILD2]
	}
	process_msgs $envlist
	#
	# Child sends us the key it used as the data
	# of the CHILD2 key.
	#
	set key [lindex [lindex $kd 0] 1]

	puts "\tRep$tnum.f: Child writes txn + ckp. Don't process msgs."
	#
	# Child has committed the txn and we have processed it.  Now
	# signal the child process to put a checkpoint, which we
	# will not process.  That will invalidate leases.
	error_check_good timestamp_done \
	    [$marker put PARENT2 [timestamp -r]] 0

	set kd [$marker get CHILD3]
	while { [llength $kd] == 0 } {
		tclsleep 1
		set kd [$marker get CHILD3]
	}

	puts "\tRep$tnum.f.0: Read using leases fails."
	check_leaseget $masterdb $key $uselease REP_LEASE_EXPIRED
	puts "\tRep$tnum.f.1: Read ignoring leases."
	check_leaseget $masterdb $key $ignorelease 0
	puts "\tRep$tnum.g: Process messages to clients."
	process_msgs $envlist
	puts "\tRep$tnum.h: Verify read with leases now succeeds."
	check_leaseget $masterdb $key $uselease 0

	watch_procs $pid 5

	process_msgs $envlist
	puts "\tRep$tnum.i: Downgrade master."
	$masterenv rep_start -client
	process_msgs $envlist

	rep_verify $masterdir $masterenv $clientdir $clientenv
	process_msgs $envlist
	rep_verify $masterdir $masterenv $clientdir2 $clientenv2 0 1 0

	# Clean up.
	error_check_good marker_db_close [$marker close] 0
	error_check_good marker_env_close [$markerenv close] 0
	error_check_good masterdb_close [$masterdb close] 0
	error_check_good masterdb_close [$clientdb close] 0
	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	error_check_good clientenv_close [$clientenv2 close] 0

	replclose $testdir/MSGQUEUEDIR

	# Check log file for failures.
	set errstrings [eval findfail $testdir/rep078script.log]
	foreach str $errstrings {
		puts "FAIL: error message in rep078 log file: $str"
	}
}

