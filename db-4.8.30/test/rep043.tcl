# See the file LICENSE for redistribution information.
#
# Copyright (c) 2005-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep043
# TEST
# TEST	Constant writes during upgrade/downgrade.
# TEST
# TEST	Three envs take turns being master.  Each env
# TEST	has a child process which does writes all the
# TEST	time.  They will succeed when that env is master
# TEST	and fail when it is not.

proc rep043 { method { rotations 25 } { tnum "043" } args } {

	source ./include.tcl
	global repfiles_in_memory

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}

	# Skip for record-based methods.
	if { $checking_valid_methods } {
		set test_methods {}
		foreach method $valid_methods {
			if { [is_record_based $method] != 1 } {
				lappend test_methods $method
			}
		}
		return $test_methods
	}
	if { [is_record_based $method] == 1 } {
		puts "Skipping rep$tnum for record-based methods."
		return
	}

	set args [convert_args $method $args]
	set logsets [create_logsets 3]

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
			puts "Rep$tnum ($method $r): Constant writes with \
			    rotating master $rotations times $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client 0 logs are [lindex $l 1]"
			puts "Rep$tnum: Client 1 logs are [lindex $l 2]"
			rep043_sub $method $rotations $tnum $l $r $args
		}
	}
}

proc rep043_sub { method rotations tnum logset recargs largs } {
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
	set orig_tdir $testdir

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/ENV0
	set clientdir $testdir/ENV1
	set clientdir2 $testdir/ENV2
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

	set niter 200
	set testfile rep043.db
	set omethod [convert_method $method]

	# Since we're constantly switching master in this test run
	# each with a different cache size just to verify that cachesize
	# doesn't matter for different sites.

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs $repmemargs \
	    $m_logargs -errpfx ENV0 -errfile /dev/stderr $verbargs \
	    -cachesize {0 4194304 3} -lock_detect default \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set env0 [eval $ma_envcmd $recargs -rep_master]

	# Open two clients
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs $repmemargs \
	    $c_logargs -errpfx ENV1 -errfile /dev/stderr $verbargs \
	    -cachesize {0 2097152 2} -lock_detect default \
	    -home $clientdir -rep_transport \[list 2 replsend\]"
	set env1 [eval $cl_envcmd $recargs -rep_client]

	repladd 3
	set cl2_envcmd "berkdb_env_noerr -create $c2_txnargs $repmemargs \
	    $c2_logargs -errpfx ENV2 -errfile /dev/stderr $verbargs \
	    -cachesize {0 1048576 1} -lock_detect default \
	    -home $clientdir2 -rep_transport \[list 3 replsend\]"
	set env2 [eval $cl2_envcmd $recargs -rep_client]

	# Bring the clients online by processing the startup messages.
	set envlist "{$env0 1} {$env1 2} {$env2 3}"
	process_msgs $envlist

	# Set up marker file.
	set markerenv [berkdb_env -create -home $testdir -txn]
	error_check_good marker_open [is_valid_env $markerenv] TRUE
	set marker [eval "berkdb_open \
	    -create -btree -auto_commit -env $markerenv marker.db"]

	# Start the 3 child processes: one for each env.
	set pids {}
	set dirlist "0 $masterdir 1 $clientdir 2 $clientdir2"
	foreach { writer dir } $dirlist {
		puts "\tRep$tnum.a: Fork child process WRITER$writer."
		set pid [exec $tclsh_path $test_path/wrap.tcl \
		    rep043script.tcl $testdir/rep043script.log.$writer \
		    $dir $writer &]
		lappend pids $pid
	}

	# For the first iteration, masterenv is $env0.
	set masterenv $env0
	set curdir $masterdir

	# Write $niter entries to master, then rotate.
	for { set i 0 } { $i < $rotations } { incr i } {

		# Identify current master, determine next master
		if { $masterenv == $env0 } {
			set nextmasterenv $env1
			set nextdir $clientdir
		} elseif { $masterenv == $env1 } {
			set nextmasterenv $env2
			set nextdir $clientdir2
		} elseif { $masterenv == $env2 } {
			set nextmasterenv $env0
			set nextdir $masterdir
		} else {
			puts "FAIL: could not identify current master"
			return
		}

		puts "\tRep$tnum.b.$i: Open master db in $curdir."
		set mdb [eval {berkdb_open_noerr} -env $masterenv -auto_commit \
		    -mode 0644 $omethod -create $testfile]
		error_check_good dbopen [is_valid_db $mdb] TRUE
		error_check_good marker_iter [$marker put ITER $i] 0

		puts "\t\tRep$tnum.c.$i: Put data to master."
		for { set j 0 } { $j < $niter } { incr j } {
			set key KEY.$i.$j
			set data DATA
			set t [$masterenv txn]
			set stat [catch \
			    {eval {$mdb put} -txn $t $key $data} ret]
			if { $ret == 0 } {
				error_check_good commit [$t commit] 0
			} else {
				error_check_good commit [$t abort] 0
			}
		}
		error_check_good mdb_close [$mdb close] 0

		# Checkpoint.
		error_check_good checkpoint [$masterenv txn_checkpoint] 0

		process_msgs $envlist

		puts "\t\tRep$tnum.d.$i: Downgrade current master."
		error_check_good downgrade [$masterenv rep_start -client] 0

		puts "\t\tRep$tnum.e.$i: Upgrade next master $nextdir."
		error_check_good upgrade [$nextmasterenv rep_start -master] 0
		set masterenv $nextmasterenv
		set curdir $nextdir

		process_msgs $envlist
	}


	puts "\tRep$tnum.f: Clean up."
	# Tell the child processes we are done.
	error_check_good marker_done [$marker put DONE DONE] 0
	error_check_good marker_close [$marker close] 0
	error_check_good markerenv_close [$markerenv close] 0

	error_check_good env0_close [$env0 close] 0
	error_check_good env1_close [$env1 close] 0
	error_check_good env2_close [$env2 close] 0

	# Make sure the child processes are done.
	watch_procs $pids 1

	# Check log files for failures.
	for { set n 0 } { $n < 3 } { incr n } {
		set file rep043script.log.$n
		set errstrings [eval findfail $testdir/$file]
		foreach str $errstrings {
			puts "FAIL: error message in file $file: $str"
		}
	}

	replclose $testdir/MSGQUEUEDIR
	set testdir $orig_tdir
	return
}
