# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep014
# TEST	Replication and multiple replication handles.
# TEST	Test multiple client handles, opening and closing to
# TEST	make sure we get the right openfiles.
#
proc rep014 { method { niter 10 } { tnum "014" } args } {

	source ./include.tcl
	global databases_in_memory 
	global repfiles_in_memory

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}

	# Run for all access methods.
	if { $checking_valid_methods } {
		return "ALL"
	}

	# We can't open two envs on HP-UX, so just skip the
	# whole test since that is at the core of it.
	if { $is_hp_test == 1 } {
		puts "Rep$tnum: Skipping for HP-UX."
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
	foreach r $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Rep$tnum: Skipping\
				    for in-memory logs with -recover."
				continue
			}
			puts "Rep$tnum ($method $r): Replication\
			    and openfiles $msg $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep014_sub $method $niter $tnum $l $r $args
		}
	}
}

proc rep014_sub { method niter tnum logset recargs largs } {
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
	set orig_tdir $testdir

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
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs $m_logargs \
	    $verbargs -errpfx MASTER -home $masterdir $repmemargs \
	    -rep_transport \[list 1 replsend\]"
	set env0 [eval $ma_envcmd $recargs -rep_master]
	set masterenv $env0

	# Open a client.
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs $c_logargs \
	    $verbargs -errpfx CLIENT1 -home $clientdir $repmemargs \
	    -rep_transport \[list 2 replsend\]"
	set env1 [eval $cl_envcmd $recargs]
	error_check_good client_env [is_valid_env $env1] TRUE
	set env2 [eval $cl_envcmd]
	error_check_good client_env [is_valid_env $env2] TRUE

	error_check_good e1_cl [$env1 rep_start -client] 0

	# Set up databases for in-memory or on-disk.
	if { $databases_in_memory } {
		set testfile { "" "test.db" }
	} else { 
		set testfile "test.db"
	} 
	
	set omethod [convert_method $method]
	set env0db [eval {berkdb_open_noerr -env $env0 -auto_commit \
	     -create -mode 0644} $largs $omethod $testfile]
	set masterdb $env0db
	error_check_good dbopen [is_valid_db $env0db] TRUE

	# Bring the clients online by processing the startup messages.
	set envlist "{$env0 1} {$env1 2}"
	process_msgs $envlist

	# Run a modified test001 in the master (and update clients).
	puts "\tRep$tnum.a: Running test001 in replicated env."
	eval rep_test $method $masterenv $masterdb $niter 0 0 0 $largs
	process_msgs $envlist

	puts "\tRep$tnum.b: Close and reopen client env."
	error_check_good env1_close [$env1 close] 0
	set env1 [eval $cl_envcmd]
	error_check_good client_env [is_valid_env $env1] TRUE
	error_check_good e1_cl [$env1 rep_start -client] 0

	puts "\tRep$tnum.c: Run test in master again."
	set start $niter
	eval rep_test $method $masterenv $masterdb $niter $start 0 0 $largs
	set envlist "{$env0 1} {$env1 2}"
	process_msgs $envlist

	puts "\tRep$tnum.d: Start and close 2nd client env."
	error_check_good e2_pfx [$env2 errpfx CLIENT2] 0
	error_check_good e2_cl [$env2 rep_start -client] 0
	error_check_good env2_close [$env2 close] 0

	puts "\tRep$tnum.e: Run test in master again."
	set start [expr $start + $niter]
	error_check_good e1_pfx [$env1 errpfx CLIENT1] 0
	eval rep_test $method $masterenv $masterdb $niter $start 0 0 $largs
	process_msgs $envlist

	puts "\tRep$tnum.f: Open env2, close env1, use env2."
	set env2 [eval $cl_envcmd]
	error_check_good client_env [is_valid_env $env2] TRUE
	error_check_good e1_pfx [$env2 errpfx CLIENT2] 0
	error_check_good e2_cl [$env2 rep_start -client] 0
	error_check_good e1_pfx [$env1 errpfx CLIENT1] 0

	# Check for on-disk or in-memory while we have all 3 envs.
	check_db_location $masterenv
	check_db_location $env1
	check_db_location $env2

	error_check_good env1_close [$env1 close] 0

	puts "\tRep$tnum.g: Run test in master again."
	set start [expr $start + $niter]
	error_check_good e1_pfx [$env2 errpfx CLIENT2] 0
	eval rep_test $method $masterenv $masterdb $niter $start 0 0 $largs
	set envlist "{$env0 1} {$env2 2}"
	process_msgs $envlist

	puts "\tRep$tnum.h: Closing"
	error_check_good env0db [$env0db close] 0
	error_check_good env0_close [$env0 close] 0
	error_check_good env2_close [$env2 close] 0
	replclose $testdir/MSGQUEUEDIR
	set testdir $orig_tdir
	return
}
