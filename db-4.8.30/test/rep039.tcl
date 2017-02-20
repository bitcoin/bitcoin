# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep039
# TEST	Test of interrupted internal initialization.  The
# TEST	interruption is due to a changed master, or the client crashing,
# TEST	or both.
# TEST
# TEST	One master, two clients.
# TEST	Generate several log files. Remove old master log files.
# TEST	Restart client, optionally having "cleaned" client env dir.  Either
# TEST	way, this has the effect of forcing an internal init.
# TEST	Interrupt the internal init.
# TEST	Vary the number of times we process messages to make sure
# TEST	the interruption occurs at varying stages of the first internal
# TEST	initialization.
# TEST
# TEST	Run for btree and queue only because of the number of permutations.
# TEST
proc rep039 { method { niter 200 } { tnum "039" } args } {

	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory

	# Run for btree and queue methods only.
	if { $checking_valid_methods } {
		set test_methods {}
		foreach method $valid_methods {
			if { [is_btree $method] == 1 || \
			    [is_queue $method] == 1 } {
				lappend test_methods $method
			}
		}
		return $test_methods
	}
	if { [is_btree $method] == 0 && [is_queue $method] == 0 } {
		puts "Rep$tnum: skipping for non-btree, non-queue method."
		return
	}

	# Skip for mixed-mode logging -- this test has a very large
	# set of iterations already.
	global mixed_mode_logging
	if { $mixed_mode_logging > 0 } {
		puts "Rep$tnum: Skipping for mixed mode logging."
		return
	}

	# This test needs to set its own pagesize.
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Rep$tnum: skipping for specific pagesizes"
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

	# Run the body of the test with and without recovery,
	# and with and without cleaning.
	set cleanopts { noclean clean }
	set archopts { archive noarchive }
	set nummsgs 4
	set announce {puts "Rep$tnum ($method $r $clean $a $crash $l $args):\
            Test of internal init. $i message iters. \
	    Test $cnt of $maxtest tests $with recovery $msg $msg2."}
	foreach r $test_recopts {
		if { $r == "-recover" && ! $is_windows_test && ! $is_hp_test } {
			set crashopts { master_change client_crash both }
		} else {
			set crashopts { master_change }
		}
		# Only one of the three sites in the replication group needs to
		# be tested with in-memory logs: the "client under test".
		#
		if { $r == "-recover" } {
			set cl_logopts { on-disk }
			set with "with"
		} else {
			set cl_logopts { on-disk in-memory }
			set with "without"
		}
		set maxtest [expr [llength $crashopts] * \
		    [llength $cleanopts] * \
		    [llength $archopts] * \
		    [llength $cl_logopts] * \
		    [expr $nummsgs]]
		set cnt 1
		foreach crash $crashopts {
			foreach clean $cleanopts {
				foreach a $archopts {
					foreach l $cl_logopts {
						for { set i 1 } \
						    { $i <= $nummsgs } \
						    { incr i } {
							eval $announce
							rep039_sub $method \
							    $niter $tnum $r \
							    $clean $a $crash \
							    $l $i $args
							incr cnt
						}
					}
				}
			}
		}
	}
}

proc rep039_sub \
    { method niter tnum recargs clean archive crash cl_logopt pmsgs largs } {
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

	set master_change false
	set client_crash false
	if { $crash == "master_change" } {
		set master_change true
	} elseif { $crash == "client_crash" } {
		set client_crash true
	} elseif { $crash == "both" } {
		set master_change true
		set client_crash true
	} else {
		error "FAIL:[timestamp] '$crash' is an unrecognized crash type"
	}

	env_cleanup $testdir

	replsetup $testdir/MSGQUEUEDIR

	# This test has three replication sites: a master, a client whose
	# behavior is under test, and another client.  We'll call them
	# "A", "B" and "C".  At one point during the test, we may (depending on
	# the setting of $master_change) switch roles between the master and the
	# other client.
	#
	# The initial site/role assignments are as follows:
	#
	#     A = master
	#     B = client under test
	#     C = other client
	#
	# In the case where we do switch roles, the roles become:
	#
	#     A = other client
	#     B = client under test (no change here)
	#     C = master
	#
	# Although the real names are A, B, and C, we'll use mnemonic names
	# whenever possible.  In particular, this means that we'll have to
	# re-jigger the mnemonic names after the role switch.

	file mkdir [set dirs(A) $testdir/SITE_A]
	file mkdir [set dirs(B) $testdir/SITE_B]
	file mkdir [set dirs(C) $testdir/SITE_C]

	# Log size is small so we quickly create more than one.
	# The documentation says that the log file must be at least
	# four times the size of the in-memory log buffer.
	set pagesize 4096
	append largs " -pagesize $pagesize "
	set log_buf [expr $pagesize * 2]
	set log_max [expr $log_buf * 4]

	# Set up the three sites: A, B, and C will correspond to EID's
	# 1, 2, and 3 in the obvious way.  As we start out, site A is always the
	# master.
	#
	repladd 1
	set env_A_cmd "berkdb_env_noerr -create -txn nosync \
	    $verbargs $repmemargs \
	    -log_buffer $log_buf -log_max $log_max -errpfx SITE_A \
	    -home $dirs(A) -rep_transport \[list 1 replsend\]"
	set envs(A) [eval $env_A_cmd $recargs -rep_master]

	# Open a client
	repladd 2
	set txn_arg [adjust_txnargs $cl_logopt]
	set log_arg [adjust_logargs $cl_logopt]
        if { $cl_logopt == "on-disk" } {
		# Override in this case, because we want to specify log_buffer.
		set log_arg "-log_buffer $log_buf"
	}
	set env_B_cmd "berkdb_env_noerr -create $txn_arg \
	    $verbargs $repmemargs \
	    $log_arg -log_max $log_max -errpfx SITE_B \
	    -home $dirs(B) -rep_transport \[list 2 replsend\]"
	set envs(B) [eval $env_B_cmd $recargs -rep_client]

	# Open 2nd client
	repladd 3
	set env_C_cmd "berkdb_env_noerr -create -txn nosync \
	    $verbargs $repmemargs \
	    -log_buffer $log_buf -log_max $log_max -errpfx SITE_C \
	    -home $dirs(C) -rep_transport \[list 3 replsend\]"
	set envs(C) [eval $env_C_cmd $recargs -rep_client]

	# Turn off throttling for this test.
	foreach site [array names envs] {
		$envs($site) rep_limit 0 0
	}

	# Bring the clients online by processing the startup messages.
	set envlist "{$envs(A) 1} {$envs(B) 2} {$envs(C) 3}"
	process_msgs $envlist

	# Set up the (indirect) mnemonic role names for the first part of the
	# test.
	set master A
	set test_client B
	set other C

	# Clobber replication's 30-second anti-archive timer, which will have
	# been started by client sync-up internal init, so that we can do a
	# log_archive in a moment.
	#
	$envs($master) test force noarchive_timeout

	# Run rep_test in the master (and update client).
	puts "\tRep$tnum.a: Running rep_test in replicated env."
	eval rep_test $method $envs($master) NULL $niter 0 0 0 $largs
	process_msgs $envlist

	puts "\tRep$tnum.b: Close client."
	error_check_good client_close [$envs($test_client) close] 0

	set res [eval exec $util_path/db_archive -l -h $dirs($test_client)]
	set last_client_log [lindex [lsort $res] end]

	set stop 0
	while { $stop == 0 } {
		# Run rep_test in the master (don't update client).
		puts "\tRep$tnum.c: Running rep_test in replicated env."
		eval rep_test $method $envs($master) NULL $niter 0 0 0 $largs
		#
		# Clear messages for first client.  We want that site
		# to get far behind.
		#
		replclear 2
		puts "\tRep$tnum.d: Run db_archive on master."
		set res [eval exec $util_path/db_archive -d -h $dirs($master)]
		set res [eval exec $util_path/db_archive -l -h $dirs($master)]
		if { [lsearch -exact $res $last_client_log] == -1 } {
			set stop 1
		}
	}

	set envlist "{$envs($master) 1} {$envs($other) 3}"
	process_msgs $envlist

	if { $archive == "archive" } {
		puts "\tRep$tnum.d: Run db_archive on other client."
		set res [eval exec $util_path/db_archive -l -h $dirs($other)]
		error_check_bad \
		    log.1.present [lsearch -exact $res log.0000000001] -1
		set res [eval exec $util_path/db_archive -d -h $dirs($other)]
		set res [eval exec $util_path/db_archive -l -h $dirs($other)]
		error_check_good \
		    log.1.gone [lsearch -exact $res log.0000000001] -1
	} else {
		puts "\tRep$tnum.d: Skipping db_archive on other client."
	}

	puts "\tRep$tnum.e: Reopen test client ($clean)."
	if { $clean == "clean" } {
		env_cleanup $dirs($test_client)
	}

	# (The test client is always site B, EID 2.)
	#
	set envs(B) [eval $env_B_cmd $recargs -rep_client]
	error_check_good client_env [is_valid_env $envs(B)] TRUE
	$envs(B) rep_limit 0 0

	# Hold an open database handle while doing internal init, to make sure
	# no back lock interactions are happening.  But only do so some of the
	# time, and of course only if it's reasonable to expect the database to
	# exist at this point.  (It won't, if we're using in-memory databases
	# and we've just started the client with recovery, since recovery blows
	# away the mpool.)  Set up database as in-memory or on-disk first.
	#
	if { $databases_in_memory } {
		set dbname { "" "test.db" }
		set have_db [expr {$recargs != "-recover"}]
	} else { 
		set dbname "test.db"
		set have_db true
	} 

	if {$clean == "noclean" && $have_db && [berkdb random_int 0 1] == 1} {
		puts "\tRep$tnum.g: Hold open db handle from client app."
		set cdb [eval\
		    {berkdb_open_noerr -env} $envs($test_client) $dbname]
		error_check_good dbopen [is_valid_db $cdb] TRUE
		set ccur [$cdb cursor]
		error_check_good curs [is_valid_cursor $ccur $cdb] TRUE
		set ret [$ccur get -first]
		set kd [lindex $ret 0]
		set key [lindex $kd 0]
		error_check_good cclose [$ccur close] 0
	} else {
		puts "\tRep$tnum.g: (No client app handle will be held.)"
		set cdb "NONE"
	}

	set envlist "{$envs(A) 1} {$envs(B) 2} {$envs(C) 3}"
	proc_msgs_once $envlist

	#
	# We want to simulate a master continually getting new
	# records while an update is going on.
	#
	set entries 10
	eval rep_test $method $envs($master) NULL $entries $niter 0 0 $largs
	#
	# We call proc_msgs_once N times to get us into page recovery:
	# 1.  Send master messages and client finds master.
	# 2.  Master replies and client does verify.
	# 3.  Master gives verify_fail and client does update_req.
	# 4.  Master send update info and client does page_req.
	#
	# We vary the number of times we call proc_msgs_once (via pmsgs)
	# so that we test switching master at each point in the
	# internal initialization processing.
	#
	set nproced 0
	puts "\tRep$tnum.f: Get partially through initialization ($pmsgs iters)"
	for { set i 1 } { $i < $pmsgs } { incr i } {
		incr nproced [proc_msgs_once $envlist]
	}

	if { [string is true $master_change] } {
		replclear 1
		replclear 3
		puts "\tRep$tnum.g: Downgrade/upgrade master."

		# Downgrade the existing master to a client, switch around the
		# roles, and then upgrade the newly appointed master.
		error_check_good downgrade [$envs($master) rep_start -client] 0

		set master C
		set other A

		error_check_good upgrade [$envs($master) rep_start -master] 0
	}

	# Simulate a client crash: simply abandon the handle without closing it.
	# Note that this doesn't work on Windows, because there you can't remove
	# a file if anyone (including yourself) has it open.  This also does not
	# work on HP-UX, because there you are not allowed to open a second 
	# handle on an env. 
	#
	# Note that crashing only makes sense with "-recover".
	#
	if { [string is true $client_crash] } {
		error_check_good assert [string compare $recargs "-recover"] 0

		set abandoned_env $envs($test_client)
		set abandoned true

		set envs($test_client) [eval $env_B_cmd $recargs -rep_client]
		$envs($test_client) rep_limit 0 0

		# Again, remember: whatever the current roles, a site and its EID
		# stay linked always.
		#
		set envlist "{$envs(A) 1} {$envs(B) 2} {$envs(C) 3}"
	} else {
		set abandoned false
	}

	process_msgs $envlist
	#
	# Now simulate continual updates to the new master.  Each
	# time through we just process messages once before
	# generating more updates.
	#
	set niter 10
	for { set i 0 } { $i < $niter } { incr i } {
		set nproced 0
		set start [expr $i * $entries]
		eval rep_test $method $envs($master) NULL $entries $start \
		    $start 0 $largs
		incr nproced [proc_msgs_once $envlist]
		error_check_bad nproced $nproced 0
	}
	set start [expr $i * $entries]
	process_msgs $envlist

	puts "\tRep$tnum.h: Verify logs and databases"
	# Whether or not we've switched roles, it's always site A that may have
	# had its logs archived away.  When the $init_test flag is turned on,
	# rep_verify allows the site in the second position to have
	# (more-)archived logs, so we have to abuse the calling signature a bit
	# here to get this to work.  (I.e., even when A is still master and C is
	# still the other client, we have to pass things in this order so that
	# the $init_test different-sized-logs trick can work.)
	#
	set init_test 1
	rep_verify $dirs(C) $envs(C) $dirs(A) $envs(A) $init_test

	# Process messages again in case we are running with debug_rop.
	process_msgs $envlist
	rep_verify $dirs($master) $envs($master) \
	    $dirs($test_client) $envs($test_client) $init_test

	# Add records to the master and update client.
	puts "\tRep$tnum.i: Add more records and check again."
	set entries 10
	eval rep_test $method $envs($master) NULL $entries $start \
	    $start 0 $largs
	process_msgs $envlist 0 NONE err

	# Check again that everyone is identical.
	rep_verify $dirs(C) $envs(C) $dirs(A) $envs(A) $init_test
	process_msgs $envlist
	rep_verify $dirs($master) $envs($master) \
	    $dirs($test_client) $envs($test_client) $init_test

	if {$cdb != "NONE"} {
		if {$abandoned} {
			# The $cdb was opened in an env which was then
			# abandoned, recovered, marked panic'ed.  We don't
			# really care; we're just trying to clean up resources.
			# 
			catch {$cdb close}
		} else {
			error_check_good clientdb_close [$cdb close] 0
		}
	}
	error_check_good masterenv_close [$envs($master) close] 0
	error_check_good clientenv_close [$envs($test_client) close] 0
	error_check_good clientenv2_close [$envs($other) close] 0
	if { $abandoned } {
		catch {$abandoned_env close}
	}
	replclose $testdir/MSGQUEUEDIR
}
