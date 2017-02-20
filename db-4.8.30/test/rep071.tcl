# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep071
# TEST	Test of multiple simultaneous client env handles and
# TEST	upgrading/downgrading.  Tests use of temp db handle
# TEST	internally.
# TEST
# TEST	Open a master and 2 handles to the same client env.
# TEST	Run rep_test.
# TEST	Close master and upgrade client to master using one env handle.
# TEST	Run rep_test again, and then downgrade back to client.
#
proc rep071 { method { niter 10 } { tnum "071" } args } {

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
		puts "Rep$tnum: Skipping for method $method."
		return
	}

	# We can't open two envs on HP-UX, so just skip the
	# whole test since that is at the core of it.
	if { $is_hp_test == 1 } {
		puts "Rep$tnum: Skipping for HP-UX."
		return
	}

	# This test depends on copying logs, so can't be run with
	# in-memory logging.
	global mixed_mode_logging
	if { $mixed_mode_logging > 0 } {
		puts "Rep$tnum: Skipping for mixed-mode logging."
		return
	}

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
		puts "Rep$tnum ($method $r): Replication\
		    backup and synchronizing $msg $msg2."
		rep071_sub $method $niter $tnum $r $args
	}
}

proc rep071_sub { method niter tnum recargs largs } {
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

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create -txn nosync $verbargs \
	    -home $masterdir -errpfx MASTER $repmemargs \
	    -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs -rep_master]

	# Open a client
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create -txn nosync $verbargs \
	    -home $clientdir -errpfx CLIENT $repmemargs \
	    -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd $recargs -rep_client]
	error_check_good clenv [is_valid_env $clientenv] TRUE
	#
	# Open a 2nd client handle to the same client env.
	# This handle needs to be a full client handle so just
	# use the same env command for both.
	#
	set 2ndclientenv [eval $cl_envcmd -rep_client -errpfx 2ND]
	error_check_good cl2env [is_valid_env $2ndclientenv] TRUE

	# Bring the clients online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	# Run a modified test001 in the master (and update client).
	puts "\tRep$tnum.a: Running rep_test in replicated env."
	eval rep_test $method $masterenv NULL $niter 0 0 0 $largs
	process_msgs $envlist

	puts "\tRep$tnum.b: Downgrade master and upgrade client."
	error_check_good master_close [$masterenv rep_start -client] 0
	error_check_good client_close [$clientenv rep_start -master] 0

	puts "\tRep$tnum.b: Run rep_test."
	eval rep_test $method $clientenv NULL $niter 0 0 0 $largs
	process_msgs $envlist

	puts "\tRep$tnum.c: Downgrade back to client and upgrade master"
	#
	# The act of upgrading and downgrading an env, with another
	# handle open had issues with open internal db handles.
	# So, the existence of the 2nd client env handle is needed
	# even though we're not doing anything active with that handle.
	#
	error_check_good client_close [$clientenv rep_start -client] 0
	error_check_good master_close [$masterenv rep_start -master] 0

	puts "\tRep$tnum.d: Run rep_test in master."
	eval rep_test $method $masterenv NULL $niter 0 0 0 $largs
	process_msgs $envlist

	rep_verify $masterdir $masterenv $clientdir $clientenv

	error_check_good master_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	error_check_good clientenv_close [$2ndclientenv close] 0
	replclose $testdir/MSGQUEUEDIR
}
