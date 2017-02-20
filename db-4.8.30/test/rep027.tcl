# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep027
# TEST	Replication and secondary indexes.
# TEST
# TEST	Set up a secondary index on the master and make sure
# TEST 	it can be accessed from the client.

proc rep027 { method { niter 1000 } { tnum "027" } args } {

	source ./include.tcl
	global repfiles_in_memory

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}

	# Renumbering recno is not permitted on a primary database.
	if { $checking_valid_methods } {
		set test_methods {}
		foreach method $valid_methods {
			if { [is_rrecno $method] != 1 } {
				lappend test_methods $method
			}
		}
		return $test_methods
	}
	if { [is_rrecno $method] == 1 } {
		puts "Skipping rep027 for -rrecno."
		return
	}

	set args [convert_args $method $args]
	set logsets [create_logsets 2]

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
			puts "Rep$tnum ($method $r):\
			    Replication and secondary indices $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep027_sub $method $niter $tnum $l $r $args
		}
	}
}

proc rep027_sub { method niter tnum logset recargs largs } {
	source ./include.tcl
	global repfiles_in_memory
	global verbose_check_secondaries
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
	repladd 1
	set env_cmd(M) "berkdb_env_noerr -create $verbargs $repmemargs \
	    -log_max 1000000 -home $masterdir -errpfx MASTER \
	    $m_txnargs $m_logargs -rep_master -rep_transport \
	    \[list 1 replsend\]"
	set masterenv [eval $env_cmd(M) $recargs]

	# Open a client
	repladd 2
	set env_cmd(C) "berkdb_env_noerr -create $verbargs $repmemargs \
	    $c_txnargs $c_logargs -home $clientdir -errpfx CLIENT \
	    -rep_client -rep_transport \[list 2 replsend\]"
	set clientenv [eval $env_cmd(C) $recargs]

	# Bring the client online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	# Set up database and secondary index on master.
	puts "\tRep$tnum.a: Set up database with secondary index."
	set pname "primary$tnum.db"
	set sname "secondary$tnum.db"

	# Open the primary.
	set pdb [eval {berkdb_open_noerr -create \
	    -auto_commit -env} $masterenv $omethod $largs $pname]
	error_check_good primary_open [is_valid_db $pdb] TRUE
	process_msgs $envlist

	# Open and associate a secondary.
	set sdb [eval {berkdb_open_noerr -create \
	    -auto_commit -env} $masterenv -btree $sname]
	error_check_good second_open [is_valid_db $sdb] TRUE
	error_check_good db_associate [$pdb associate [callback_n 0] $sdb] 0

	# Propagate to client.
	process_msgs $envlist

	# Put some data in the master.
	set did [open $dict]
	for { set n 0 } { [gets $did str] != -1 && $n < $niter } { incr n } {
		if { [is_record_based $method] == 1 } {
			set key [expr $n + 1]
			set datum $str
		} else {
			set key $str
			gets $did datum
		}
		set keys($n) $key
		set data($n) [pad_data $method $datum]

		set ret [$pdb put $key [chop_data $method $datum]]
		error_check_good put($n) $ret 0
	}
	close $did
	process_msgs $envlist

	# Check secondaries on master.
	set verbose_check_secondaries 1
	puts "\tRep$tnum.b: Check secondaries on master."
	check_secondaries $pdb $sdb $niter keys data "Rep$tnum.b"
	error_check_good pdb_close [$pdb close] 0
	error_check_good sdb_close [$sdb close] 0
	process_msgs $envlist

	# Get handles on primary and secondary db on client.
	set clientpdb [eval {berkdb_open -auto_commit -env} $clientenv $pname]
	error_check_good client_pri [is_valid_db $clientpdb] TRUE
	set clientsdb [eval {berkdb_open -auto_commit -env} $clientenv $sname]
	error_check_good client_sec [is_valid_db $clientsdb] TRUE
	error_check_good client_associate \
	    [$clientpdb associate [callback_n 0] $clientsdb] 0

	# Check secondaries on client.
	puts "\tRep$tnum.c: Check secondaries on client."
	check_secondaries $clientpdb $clientsdb $niter keys data "Rep$tnum.c"

	# Clean up.
	error_check_good clientpdb_close [$clientpdb close] 0
	error_check_good clientsdb_close [$clientsdb close] 0
	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0

	error_check_good verify \
	    [verify_dir $clientdir "\tRep$tnum.e: " 0 0 1] 0
	replclose $testdir/MSGQUEUEDIR
}
