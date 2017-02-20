# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST  rep047
# TEST	Replication and log gap bulk transfers.
# TEST	Set bulk transfer replication option.
# TEST	Run test.  Start a new client (to test ALL_REQ and bulk).
# TEST	Run small test again.  Clear messages for 1 client.
# TEST	Run small test again to test LOG_REQ gap processing and bulk.
# TEST	Process and verify on clients.
#
proc rep047 { method { nentries 200 } { tnum "047" } args } {
	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win9x platform."
		return
	}

	# Valid for all access methods.
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
	foreach r $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Skipping rep$tnum for -recover\
				    with in-memory logs."
				continue
			}
			puts "Rep$tnum ($method $r):\
			    Replication and resend bulk transfer $msg"
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			puts "Rep$tnum: Client 2 logs are [lindex $l 2]"
			rep047_sub $method $nentries $tnum $l $r $args
		}
	}
}

proc rep047_sub { method niter tnum logset recargs largs } {
	global testdir
	global util_path
	global overflowword1 overflowword2
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

	set overflowword1 "0"
	set overflowword2 "0"
	set orig_tdir $testdir
	env_cleanup $testdir

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR2

	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $clientdir2

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

	set in_memory_log \
	    [expr { $m_logtype == "in-memory" || $c_logtype == "in-memory" || \
	    $c2_logtype == "in-memory" }]

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env -create $m_txnargs $m_logargs \
	    $verbargs -errpfx MASTER -home $masterdir $repmemargs \
	    -rep_master -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs]
	error_check_good master_env [is_valid_env $masterenv] TRUE

	# Open two clients.
	repladd 2
	set cl_envcmd "berkdb_env -create $c_txnargs $c_logargs \
	    $verbargs -errpfx CLIENT -home $clientdir $repmemargs \
	    -rep_client -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd $recargs]
	error_check_good client_env [is_valid_env $clientenv] TRUE

	repladd 3
	set cl2_envcmd "berkdb_env -create $c2_txnargs $c2_logargs \
	    $verbargs -errpfx CLIENT2 -home $clientdir2 $repmemargs \
	    -rep_client -rep_transport \[list 3 replsend\]"

	# Bring the client online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	error_check_good set_bulk [$masterenv rep_config {bulk on}] 0

	puts "\tRep$tnum.a: Create and open master database"

	# Set up databases as in-memory or on-disk.
	if { $databases_in_memory } {
		set dbname { "" "test.db" }
	} else { 
		set dbname "test.db"
	} 
	
	set omethod [convert_method $method]
	set masterdb [eval {berkdb_open -env $masterenv -auto_commit \
	    -create -mode 0644} $largs $omethod $dbname]
	error_check_good dbopen [is_valid_db $masterdb] TRUE

	# Bring the client online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	# Run a modified test001 in the master (and update clients).
	puts "\tRep$tnum.b: Basic long running txn"
	rep_test_bulk $method $masterenv $masterdb $niter 0 0 0
	process_msgs $envlist
	rep_verify $masterdir $masterenv\
	    $clientdir $clientenv $in_memory_log 1 1

	# Clean up after rep_verify: remove the temporary "prlog" file.  Now
	# that a newly joining client uses internal init, when the master scans
	# its directory for database files it complains about prlog not looking
	# like a proper db.  This is harmless, but it does put a distracting
	# error message into the test output.
	#
	file delete $masterdir/prlog

	puts "\tRep$tnum.c: Bring new client online"
	replclear 3
	set bulkrec1 [stat_field $masterenv rep_stat "Bulk records stored"]
	set bulkxfer1 [stat_field $masterenv rep_stat "Bulk buffer transfers"]
	set clientenv2 [eval $cl2_envcmd $recargs]
	error_check_good client_env [is_valid_env $clientenv2] TRUE
	set envlist "{$masterenv 1} {$clientenv 2} {$clientenv2 3}"
	process_msgs $envlist

	#
	# We know we added $niter items to the database so there should be
	# at least $niter records stored to the log.  Verify that
	# when we brought client 2 online, we sent at least $niter more
	# records via bulk.
	#
	set bulkrec2 [stat_field $masterenv rep_stat "Bulk records stored"]
	set bulkxfer2 [stat_field $masterenv rep_stat "Bulk buffer transfers"]
	set recstat [expr $bulkrec2 > [expr $bulkrec1 + $niter]]
	error_check_good recstat $recstat 1
	error_check_good xferstat [expr $bulkxfer2 > $bulkxfer1] 1
	puts "\tRep$tnum.c.0: Take new client offline"

	puts "\tRep$tnum.d: Run small test creating a log gap"
	set skip $niter
	set start $niter
	set niter 10
	rep_test_bulk $method $masterenv $masterdb $niter $start $skip 0
	#
	# Skip and clear messages for client 2.
	#
	replclear 3
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	puts "\tRep$tnum.e: Bring new client online again"
	set envlist "{$masterenv 1} {$clientenv 2} {$clientenv2 3}"
	#
	# Since we're relying on the client to detect a gap and request missing
	# records, reset gap parameters to small values.  Otherwise,
	# "wait_recs" is still set at its maximum "high" value, due to this
	# client having been through an internal init.  Send a record to
	# create a real gap and not an end-of-record stream pause.
	#
	$masterenv txn_checkpoint -force
	$clientenv2 rep_request 4000 128000
	process_msgs $envlist
	tclsleep 1
	set bulkrec1 [stat_field $masterenv rep_stat "Bulk records stored"]
	set bulkxfer1 [stat_field $masterenv rep_stat "Bulk buffer transfers"]
	set skip [expr $skip + $niter]
	set start $skip
	rep_test_bulk $method $masterenv $masterdb $niter $start $skip 0

	process_msgs $envlist
	#
	# We know we added 2*$niter items to the database so there should be
	# at least 2*$niter records stored to the log.  Verify that
	# when we brought client 2 online, we sent at least 2*$niter more
	# records via bulk.
	#
	set bulkrec2 [stat_field $masterenv rep_stat "Bulk records stored"]
	set bulkxfer2 [stat_field $masterenv rep_stat "Bulk buffer transfers"]
	set recstat [expr $bulkrec2 > [expr $bulkrec1 + [expr 2 * $niter]]]
	error_check_good recstat $recstat 1
	error_check_good xferstat [expr $bulkxfer2 > $bulkxfer1] 1

	# Turn off bulk processing now on the master.  We need to do
	# this because some configurations (like debug_rop/wop) will
	# generate log records when verifying the logs and databases.
	# We want to control processing those messages.
	#
	error_check_good set_bulk [$masterenv rep_config {bulk off}] 0

	rep_verify $masterdir $masterenv\
	    $clientdir $clientenv $in_memory_log 1 1

	# Process messages again in case we are running with debug_rop.
	process_msgs $envlist
	rep_verify $masterdir $masterenv\
	    $clientdir2 $clientenv2 $in_memory_log 1 1

	error_check_good dbclose [$masterdb close] 0
	error_check_good mclose [$masterenv close] 0
	error_check_good cclose [$clientenv close] 0
	error_check_good c2close [$clientenv2 close] 0
	replclose $testdir/MSGQUEUEDIR
}
