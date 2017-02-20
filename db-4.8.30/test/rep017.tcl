# See the file LICENSE for redistribution information.
#
# Copyright (c) 2003-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep017
# TEST	Concurrency with checkpoints.
# TEST
# TEST 	Verify that we achieve concurrency in the presence of checkpoints.
# TEST 	Here are the checks that we wish to make:
# TEST  	While dbenv1 is handling the checkpoint record:
# TEST		Subsequent in-order log records are accepted.
# TEST          	Accepted PERM log records get NOTPERM
# TEST          	A subsequent checkpoint gets NOTPERM
# TEST          	After checkpoint completes, next txn returns PERM
proc rep017 { method { niter 10 } { tnum "017" } args } {

	source ./include.tcl
	global repfiles_in_memory

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}

	# Run for all access methods.
	if { $checking_valid_methods } {
		return "ALL"
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
			    Concurrency with checkpoints $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep017_sub $method $niter $tnum $l $r $args
		}
	}
}

proc rep017_sub { method niter tnum logset recargs largs } {
	source ./include.tcl
	global perm_response_list
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
	set ma_cmd "berkdb_env_noerr -create $verbargs \
	    -log_max 1000000 $m_txnargs $m_logargs $repmemargs \
	    -home $masterdir -rep_master -errpfx MASTER \
	    -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_cmd $recargs]

	# Open a client
	repladd 2
	set cl_cmd "berkdb_env_noerr -create -home $clientdir $verbargs \
	    $c_txnargs $c_logargs -rep_client -errpfx CLIENT $repmemargs \
	    -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_cmd $recargs]

	# Bring the client online.
	process_msgs "{$masterenv 1} {$clientenv 2}"

	# Open database in master, make lots of changes so checkpoint
	# will take a while, and propagate to client.
	puts "\tRep$tnum.a: Create and populate database."
	set dbname rep017.db
	set db [eval "berkdb_open_noerr -create $omethod -auto_commit \
	    -env $masterenv $largs $dbname"]
	for { set i 1 } { $i <= $niter } { incr i } {
		set t [$masterenv txn]
		error_check_good db_put \
		    [eval $db put -txn $t $i [chop_data $method data$i]] 0
		error_check_good txn_commit [$t commit] 0
	}
	process_msgs "{$masterenv 1} {$clientenv 2}" 1

	# Get the master's last LSN before the checkpoint
	set pre_ckp_offset \
		[stat_field $masterenv log_stat "Current log file offset"]

	puts "\tRep$tnum.b: Checkpoint on master."
	error_check_good checkpoint [$masterenv txn_checkpoint] 0

	# Now get ckp LSN
	set ckp_lsn [stat_field $masterenv txn_stat "LSN of last checkpoint"]
	set ckp_offset [lindex $ckp_lsn 1]

	# Fork child process on client.  It should process whatever
	# it finds in the message queue -- just the checkpoint record,
	# for now.  It's run in the background so the parent can
	# test for whether we're checkpointing at the same time.
	#
	puts "\tRep$tnum.c: Fork child process on client."
	set pid [exec $tclsh_path $test_path/wrap.tcl \
	    rep017script.tcl $testdir/repscript.log \
	    $masterdir $clientdir $rep_verbose $verbose_type &]


	# We need to wait until we know that the client is processing a
	# checkpoint.  The checkpoint will consist of some DBREG records
	# followed by the actual checkpoint.  So, if we've gotten records
	# later than the last LSN when the master took the checkpoint, we've
	# begin the checkpoint.  By test design, we should not finish the
	# checkpoint until this process has at least had a chance to run.
	# 
	# In order to do this, we have handles open on the message
	# queue from both this process and its child.  This is not 
	# normally legal behavior for an application using Berkeley DB, 
	# but this test depends on the parent process doing things while
	# the child is pausing in the middle of the checkpoint.  We are
	# very careful to control which process is handling which 
	# messages.

 	puts "\tRep$tnum.d: Test whether client is in checkpoint."
	while { 1 } {
		set client_off \
		    [stat_field $clientenv log_stat "Current log file offset"]

		if { $client_off > $pre_ckp_offset } {
			if { $client_off > $ckp_offset } {
				# We already completed the checkpoint and
				# never got out of here.  That's a bug in
				# in the test.
				error_check_good checkpoint_test \
				    not_in_checkpoint should_be_in_checkpoint
			} else {
				break;
			}
		} else {
			# Not yet up to checkpoint
			tclsleep 1
		}
	}

	# Main client processes checkpoint 2nd time and should get NOTPERM.
	puts "\tRep$tnum.e: Commit and checkpoint return NOTPERM from client"
	incr niter
	set t [$masterenv txn]
	error_check_good db_put [eval $db put \
	    -txn $t $niter [chop_data $method data$niter]] 0
	error_check_good txn_commit [$t commit] 0
	error_check_good checkpoint [$masterenv txn_checkpoint] 0
	set ckp2_lsn [stat_field $masterenv txn_stat "LSN of last checkpoint"]

	process_msgs "{$clientenv 2}" 1

	# Check that the checkpoint record got a NOTPERM
	# Find the ckp LSN of the Master and then look for the response
	# from that message in the client
	set ckp_result ""
	foreach i $perm_response_list {
		# Everything in the list should be NOTPERM
		if { [llength $i] == 0 } {
			# Check for sentinel at beginning of list
			continue;
		}
		set ckp_result [lindex $i 0]
		error_check_good NOTPERM [is_substr $ckp_result NOTPERM] 1
		if { [lindex $i 1] == $ckp2_lsn } {
			break
		}
	}
	error_check_bad perm_response $ckp_result ""

	puts "\tRep$tnum.f: Waiting for child ..."
	# Watch until the checkpoint is done.
	watch_procs $pid 5

	# Verify that the checkpoint is now complete on the client and
	# that all later messages have been applied.
	process_msgs "{$clientenv 2}" 1
	set client_ckp [stat_field $clientenv txn_stat "LSN of last checkpoint"]
	error_check_good matching_ckps $client_ckp $ckp2_lsn

	set m_end [stat_field $masterenv log_stat "Current log file offset"]
	set c_end [stat_field $clientenv log_stat "Current log file offset"]
	error_check_good matching_lsn $c_end $m_end

	# Finally, now that checkpoints are complete; perform another
	# perm operation and make sure that it returns ISPERM.
	puts "\tRep$tnum.g: No pending ckp; check for ISPERM"
	incr niter
	set t [$masterenv txn]
	error_check_good db_put [eval $db put \
	    -txn $t $niter [chop_data $method data$niter]] 0
	error_check_good txn_commit [$t commit] 0
	error_check_good checkpoint [$masterenv txn_checkpoint] 0
	set ckp3_lsn [stat_field $masterenv txn_stat "LSN of last checkpoint"]

	process_msgs "{$clientenv 2}" 1

	# Check that the checkpoint and commit records got a ISPERM
	# Find the ckp LSN of the Master and then look for the response
	# from that message in the client
	set ckp_result ""
	foreach i $perm_response_list {
		if { [llength $i] == 0 } {
			# Check for sentinel at beginning of list
			continue;
		}

		# Everything in the list should be ISPERM
		set ckp_result [lindex $i 0]
		error_check_good ISPERM [is_substr $ckp_result ISPERM] 1
		if { [lindex $i 1] == $ckp3_lsn } {
			break
		}
	}
	error_check_bad perm_response $ckp_result ""

	# Clean up.
	error_check_good db_close [$db close] 0
	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0

	replclose $testdir/MSGQUEUEDIR
}
