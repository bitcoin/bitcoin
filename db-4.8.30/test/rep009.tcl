# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST  rep009
# TEST	Replication and DUPMASTERs
# TEST  Run test001 in a replicated environment.
# TEST
# TEST  Declare one of the clients to also be a master.
# TEST  Close a client, clean it and then declare it a 2nd master.
proc rep009 { method { niter 10 } { tnum "009" } args } {

	source ./include.tcl
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
		puts "Rep009: Skipping for method $method."
		return
	}

	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	set logsets [create_logsets 3]

	# Run the body of the test with and without recovery.
	foreach r $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Rep$tnum: Skipping\
				    for in-memory logs with -recover."
				continue
			}
			puts "Rep$tnum ($r): Replication DUPMASTER test $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client1 logs are [lindex $l 1]"
			puts "Rep$tnum: Client2 logs are [lindex $l 2]"
			rep009_sub $method $niter $tnum 0 $l $r $args
			rep009_sub $method $niter $tnum 1 $l $r $args
		}
	}
}

proc rep009_sub { method niter tnum clean logset recargs largs } {
	global testdir
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
	set clientdir2 $testdir/CLIENTDIR.2

	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $clientdir2

	set m_logtype [lindex $logset 0]
	set m_logargs [adjust_logargs $m_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]

	set c_logtype [lindex $logset 1]
	set c_logargs [adjust_logargs $c_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	set c2_logtype [lindex $logset 2]
	set c2_logargs [adjust_logargs $c2_logtype]
	set c2_txnargs [adjust_txnargs $c2_logtype]

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs $m_logargs \
	    -home $masterdir $verbargs -errpfx MASTER $repmemargs \
	    -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs -rep_master]

	# Open a client.
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs $c_logargs \
	    -home $clientdir $verbargs -errpfx CLIENT1 $repmemargs \
	    -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd $recargs -rep_client]

	# Open a second client.
	repladd 3
	set cl2_envcmd "berkdb_env_noerr -create $c2_txnargs $c2_logargs \
	    -home $clientdir2 $verbargs -errpfx CLIENT2 $repmemargs \
	    -rep_transport \[list 3 replsend\]"
	set cl2env [eval $cl2_envcmd $recargs -rep_client]

	# Bring the clients online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2} {$cl2env 3}"
	process_msgs $envlist

	# Run a modified test001 in the master (and update client).
	puts "\tRep$tnum.a: Running test001 in replicated env."
	eval test001 $method $niter 0 0 $tnum -env $masterenv $largs
	process_msgs $envlist

	puts "\tRep$tnum.b: Declare a client to be a master."
	if { $clean } {
		error_check_good clientenv_close [$clientenv close] 0
		env_cleanup $clientdir
		set clientenv [eval $cl_envcmd $recargs -rep_master]
		error_check_good client_env [is_valid_env $clientenv] TRUE
	} else {
		error_check_good client_master [$clientenv rep_start -master] 0
	}

	#
	# Process the messages to get them out of the db.
	#
	for { set i 1 } { $i <= 3 } { incr i } {
		set seen_dup($i) 0
	}
	while { 1 } {
		set nproced 0

		incr nproced [replprocessqueue \
		    $masterenv 1 0 NONE dup1 err1]
		incr nproced [replprocessqueue \
		    $clientenv 2 0 NONE dup2 err2]
		incr nproced [replprocessqueue \
		    $cl2env 3 0 NONE dup3 err3]
		if { $dup1 != 0 } {
			set seen_dup(1) 1
			error_check_good downgrade1 \
			    [$masterenv rep_start -client] 0
		}
		if { $dup2 != 0 } {
			set seen_dup(2) 1
			error_check_good downgrade1 \
			    [$clientenv rep_start -client] 0
		}
		#
		# We might get errors after downgrading as the former
		# masters might get old messages from other clients.
		# If we get an error make sure it is after downgrade.
		if { $err1 != 0 } {
			error_check_good seen_dup1_err $seen_dup(1) 1
			error_check_good err1str [is_substr \
			    $err1 "invalid argument"] 1
		}
		if { $err2 != 0 } {
			error_check_good seen_dup2_err $seen_dup(2) 1
			error_check_good err2str [is_substr \
			    $err2 "invalid argument"] 1
		}
		#
		# This should never happen.  We'll check below.
		#
		if { $dup3 != 0 } {
			set seen_dup(3) 1
		}

		if { $nproced == 0 } {
			break
		}
	}
	error_check_good seen_dup1 $seen_dup(1) 1
	error_check_good seen_dup2 $seen_dup(2) 1
	error_check_bad seen_dup3 $seen_dup(3) 1

	puts "\tRep$tnum.c: Close environments"
	error_check_good master_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	error_check_good cl2_close [$cl2env close] 0
	replclose $testdir/MSGQUEUEDIR
}
