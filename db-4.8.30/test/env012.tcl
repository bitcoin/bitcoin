# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	env012
# TEST	Test DB_REGISTER.
# TEST
# TEST	DB_REGISTER will fail on systems without fcntl.  If it
# TEST	fails, make sure we got the expected DB_OPNOTSUP return.
# TEST
# TEST	Then, the real tests:
# TEST	For each test, we start a process that opens an env with -register.
# TEST
# TEST	1. Verify that a 2nd process can enter the existing env with -register.
# TEST
# TEST	2. Kill the 1st process, and verify that the 2nd process can enter
# TEST	with "-register -recover".
# TEST
# TEST	3. Kill the 1st process, and verify that the 2nd process cannot
# TEST	enter with just "-register".
# TEST
# TEST	4. While the 1st process is still running, a 2nd process enters
# TEST	with "-register".  Kill the 1st process.  Verify that a 3rd process
# TEST	can enter with "-register -recover".  Verify that the 3rd process,
# TEST	entering, causes process 2 to fail with the message DB_RUNRECOVERY.
# TEST
# TEST	5. We had a bug where recovery was always run with -register
# TEST	if there were empty slots in the process registry file.  Verify
# TEST	that recovery doesn't automatically run if there is an empty slot.
# TEST
# TEST  6. Verify process cannot connect when specifying -failchk and an
# TEST  isalive function has not been declared.
# TEST
# TEST  7. Verify that a 2nd process can enter the existing env with -register
# TEST  and -failchk and having specified an isalive function
# TEST
# TEST  8. Kill the 1st process, and verify that the 2nd process can enter
# TEST  with "-register -failchk -recover"
# TEST 
# TEST  9. 2nd process enters with "-register -failchk".  Kill the 1st process.
# TEST  2nd process may get blocked on a mutex held by process one.  Verify
# TEST  3rd process can enter with "-register -recover -failchk".   3rd process
# TEST  should run failchk, clear out open txn/log from process 1.   It will
# TEST  enter env without need for any additional recovery.   We look for 
# TEST  "Freeing log information .." sentence in the log for 3rd process as 
# TEST  an indication that failchk ran.   If DB_RUNRECOVERY were returned 
# TEST  instead it would mean failchk could not recover. 

proc env012 { } {
	source ./include.tcl
	set tnum "012"

	puts "Env$tnum: Test of DB_REGISTER."

	puts "\tEnv$tnum.a: Platforms without fcntl fail with DB_OPNOTSUP."
	env_cleanup $testdir
	if {[catch {eval {berkdb_env} \
	    -create -home $testdir -txn -register -recover} env]} {
		error_check_good fail_OPNOTSUP [is_substr $env DB_OPNOTSUP] 1
		puts "Skipping env$tnum; DB_REGISTER is not supported."
	}
	error_check_good env_close [$env close] 0

	puts "\tEnv$tnum.b: Second process can join with -register."
	env_cleanup $testdir
	set testfile TESTFILE
	set key KEY
	set data DATA1

	puts "\t\tEnv$tnum.b1: Start process 1."
	set p1 [exec $tclsh_path $test_path/wrap.tcl envscript.tcl \
	    $testdir/env$tnum.log.p1 \
	    $testdir $testfile PUT $key $data RECOVER 0 10 &]

	# Wait a while so process 1 has a chance to get going.
	tclsleep 2

	puts "\t\tEnv$tnum.b2: Start process 2."
	set p2 [exec $tclsh_path $test_path/wrap.tcl envscript.tcl \
	    $testdir/env$tnum.log.p2 \
	    $testdir $testfile GET $key $data 0 0 0 &]

	watch_procs $p1 1 120
	watch_procs $p2 1 120

	# Check log files for failures.
	logcheck $testdir/env$tnum.log.p1
	logcheck $testdir/env$tnum.log.p2

	puts "\tEnv$tnum.c: Second process can join with -register\
	    -recover after first process is killed."
	env_cleanup $testdir

	puts "\t\tEnv$tnum.c1: Start process 1."
	set pids {}
	set p1 [exec $tclsh_path $test_path/wrap.tcl envscript.tcl \
	    $testdir/env$tnum.log.p1 \
	    $testdir $testfile PUT $key $data RECOVER 0 10 &]
	lappend pids $p1
	tclsleep 2

	puts "\t\tEnv$tnum.c2: Kill process 1."
	set pids [findprocessids $testdir $pids]
	foreach pid $pids {
		tclkill $pid
	}

	puts "\t\tEnv$tnum.c3: Start process 2."
	set p2 [exec $tclsh_path $test_path/wrap.tcl envscript.tcl \
	    $testdir/env$tnum.log.p2 \
	    $testdir $testfile GET $key $data RECOVER 0 0 &]

	watch_procs $p2 1 120

	# Check log files for failures.
	logcheck $testdir/env$tnum.log.p1
	logcheck $testdir/env$tnum.log.p2

	if { $is_windows_test == 1 } {
		puts "Skipping sections .d and .e on Windows platform."
	} else {
		puts "\tEnv$tnum.d: Second process cannot join without -recover\
		    after first process is killed."
		env_cleanup $testdir
	
		puts "\t\tEnv$tnum.d1: Start process 1."
		set pids {}
		set p1 [exec $tclsh_path $test_path/wrap.tcl envscript.tcl \
		    $testdir/env$tnum.log.p1 \
		    $testdir $testfile PUT $key $data RECOVER 0 10 &]
		lappend pids $p1
		tclsleep 2
	
		puts "\t\tEnv$tnum.d2: Kill process 1."
		set pids [findprocessids $testdir $pids]
		foreach pid $pids {
			tclkill $pid
		}
	
		puts "\t\tEnv$tnum.d3: Start process 2."
		set p2 [exec $tclsh_path $test_path/wrap.tcl envscript.tcl \
		    $testdir/env$tnum.log.p2 \
		    $testdir $testfile GET $key $data 0 0 0 &]
		tclsleep 2
		watch_procs $p2 1 120
	
		# Check log files.  Log p1 should be clean, but we
		# expect DB_RUNRECOVERY in log p2.
		logcheck $testdir/env$tnum.log.p1
		logcheckfails $testdir/env$tnum.log.p2 DB_RUNRECOVERY
	
		puts "\tEnv$tnum.e: Running registered process detects failure."
		env_cleanup $testdir
	
		puts "\t\tEnv$tnum.e1: Start process 1."
		set pids {}
		set p1 [exec $tclsh_path $test_path/wrap.tcl envscript.tcl \
		    $testdir/env$tnum.log.p1 \
		    $testdir $testfile PUT $key $data RECOVER 0 10 &]
		lappend pids $p1
		tclsleep 2
	
		# Identify child process to kill later.
		set pids [findprocessids $testdir $pids]
	
		puts "\t\tEnv$tnum.e2: Start process 2."
		set p2 [exec $tclsh_path $test_path/wrap.tcl envscript.tcl \
		    $testdir/env$tnum.log.p2 \
		    $testdir $testfile LOOP $key $data 0 0 10 &]
	
		puts "\t\tEnv$tnum.e3: Kill process 1."
		foreach pid $pids {
			tclkill $pid
		}
	
		puts "\t\tEnv$tnum.e4: Start process 3."
		set p3 [exec $tclsh_path $test_path/wrap.tcl envscript.tcl \
		    $testdir/env$tnum.log.p3 \
		    $testdir $testfile GET $key $data RECOVER 0 0 &]
		tclsleep 2
	
		watch_procs $p2 1 120
		watch_procs $p3 1 120
	
		# Check log files.  Logs p1 and p3 should be clean, but we
		# expect DB_RUNRECOVERY in log p2.
		logcheck $testdir/env$tnum.log.p1
		logcheckfails $testdir/env$tnum.log.p2 DB_RUNRECOVERY
		logcheck $testdir/env$tnum.log.p3
	}

	puts "\tEnv$tnum.f: Empty slot shouldn't cause automatic recovery."

	# Create 2 empty slots in the registry by letting two processes
	# run to completion.
	puts "\t\tEnv$tnum.f1: Start process 1."
	set p1 [exec $tclsh_path $test_path/wrap.tcl envscript.tcl \
	    $testdir/env$tnum.log.p1 \
	    $testdir $testfile PUT $key $data RECOVER 0 1 &]

	puts "\t\tEnv$tnum.f2: Start process 2."
	set p2 [exec $tclsh_path $test_path/wrap.tcl envscript.tcl \
	    $testdir/env$tnum.log.p2 \
	    $testdir $testfile GET $key $data 0 0 1 &]

	watch_procs $p1 1 60
	watch_procs $p2 1 60

	logcheck $testdir/env$tnum.log.p1
	logcheck $testdir/env$tnum.log.p2

	# Start two more process.  Neither should signal a need for recovery.
	puts "\t\tEnv$tnum.f3: Start process 3."
	set p3 [exec $tclsh_path $test_path/wrap.tcl envscript.tcl \
	    $testdir/env$tnum.log.p3 \
	    $testdir $testfile GET $key $data RECOVER 0 10 &]

	tclsleep 2

	puts "\t\tEnv$tnum.f4: Start process 4."
	set p4 [exec $tclsh_path $test_path/wrap.tcl envscript.tcl \
	    $testdir/env$tnum.log.p4 \
	    $testdir $testfile PUT $key $data 0 0 10 &]

	watch_procs $p3 1 120
	watch_procs $p4 1 120

	# Check log files: neither process should have returned DB_RUNRECOVERY.
	logcheck $testdir/env$tnum.log.p3
	logcheck $testdir/env$tnum.log.p4

	puts "\tEnv$tnum.g: One process with -register & -failchk & no isalive"

	# use -failchk only, test will fail as isalive function is needed
	puts "\t\tEnv$tnum.g1: Start process 1."
	env_cleanup $testdir

	set p1 [exec $tclsh_path $test_path/wrap.tcl envscript.tcl \
	    $testdir/env$tnum.log.p1 \
	    $testdir $testfile PUT $key $data RECOVER FAILCHK0 10 &]

	watch_procs $p1 1 60

	# Check log files for failures.  Expect to see a failure.
	logcheckfails $testdir/env$tnum.log.p1 DB_FAILCHK

	puts "\tEnv$tnum.h: Second process joins with -register and -failchk."
	env_cleanup $testdir

        # use -failchk and -isalive flags
	puts "\t\tEnv$tnum.h1: Start process 1."
	set p1 [exec $tclsh_path $test_path/wrap.tcl envscript.tcl \
	    $testdir/env$tnum.log.p1 \
	    $testdir $testfile PUT $key $data RECOVER FAILCHK1 10 &]

	# Wait a while so process 1 has a chance to get going.
	tclsleep 2

	puts "\t\tEnv$tnum.h2: Start process 2."
	set p2 [exec $tclsh_path $test_path/wrap.tcl envscript.tcl \
	    $testdir/env$tnum.log.p2 \
	    $testdir $testfile GET $key $data 0 FAILCHK1 0 &]

	watch_procs $p1 1 120
	watch_procs $p2 1 120

	# Check log files for failures.
	logcheck $testdir/env$tnum.log.p1
	logcheck $testdir/env$tnum.log.p2

	puts "\tEnv$tnum.i: Second process can join with -register\
	    -recover -failchk after first process is killed."
	env_cleanup $testdir

	puts "\t\tEnv$tnum.i1: Start process 1."
	set pids {}
	set p1 [exec $tclsh_path $test_path/wrap.tcl envscript.tcl \
	    $testdir/env$tnum.log.p1 \
	    $testdir $testfile PUT $key $data RECOVER FAILCHK1 10 &]
	lappend pids $p1
	tclsleep 2

	puts "\t\tEnv$tnum.i2: Kill process 1."
	set pids [findprocessids $testdir $pids]
	foreach pid $pids {
		tclkill $pid
	}

	puts "\t\tEnv$tnum.i3: Start process 2."
	set p2 [exec $tclsh_path $test_path/wrap.tcl envscript.tcl \
	    $testdir/env$tnum.log.p2 \
	    $testdir $testfile GET $key $data RECOVER FAILCHK1 0 &]

	watch_procs $p2 1 120

	# Check log files for failures.
	logcheck $testdir/env$tnum.log.p1
	logcheck $testdir/env$tnum.log.p2

	if { $is_windows_test == 1 } {
		puts "Skipping sections .j on Windows platform."
	} else {
		puts "\tEnv$tnum.j: Running registered process detects failure and recovers."
		env_cleanup $testdir
	
		puts "\t\tEnv$tnum.j1: Start process 1."
		set pids {}
		set p1 [exec $tclsh_path $test_path/wrap.tcl envscript.tcl \
		    $testdir/env$tnum.log.p1 \
		    $testdir $testfile LOOP $key $data RECOVER FAILCHK1 5 &]
		lappend pids $p1
		tclsleep 2
	
		# Identify child process to kill later.
		set pids [findprocessids $testdir $pids]
	
		puts "\t\tEnv$tnum.j2: Start process 2."
		set p2 [exec $tclsh_path $test_path/wrap.tcl envscript.tcl \
		    $testdir/env$tnum.log.p2 \
		    $testdir $testfile LOOP $key $data 0 0 10 &]
	
		puts "\t\tEnv$tnum.j3: Kill process 1."
		foreach pid $pids {
			tclkill $pid
		}

		#identify child process 2, do after process 1 has died 	
		lappend pids $p2                

		# Identify child process to kill later.
		set pids [findprocessids $testdir $pids]

		puts "\t\tEnv$tnum.j4: Start process 3."
		set p3 [exec $tclsh_path $test_path/wrap.tcl envscript.tcl \
		    $testdir/env$tnum.log.p3 \
		    $testdir $testfile GET $key $data RECOVER FAILCHK1 0 &]
		#sleep for approx 20 seconds -- process 2 should still be going
		tclsleep 20

		puts "\t\tEnv$tnum.j5: Kill process 2."
		foreach pid $pids {
			tclkill $pid
		}

		watch_procs $p3 1 30
	
		# Check log files.  Logs p1 and p2 should be clean, but we
		# expect failchk messages in p3 from cleanup
		logcheckfails $testdir/env$tnum.log.p3 Freeing
		logcheck $testdir/env$tnum.log.p2 
		logcheck $testdir/env$tnum.log.p1
	}
}

# Check log file and report failures with FAIL.  Use this when
# we don't expect failures.
proc logcheck { logname } {
	set errstrings [eval findfail $logname]
	foreach errstring $errstrings {
		puts "FAIL: error in $logname : $errstring"
	}
}

# When we expect a failure, verify we find the one we expect.
proc logcheckfails { logname message }  {
	set f [open $logname r]
	while { [gets $f line] >= 0 } {
		if { [is_substr $line $message] == 1 } {
			close $f
			return 0
		}
	}
	close $f
	puts "FAIL: Did not find expected error $message."
}

# The script wrap.tcl creates a parent and a child process.  We
# can't see the child pids, so find them by their sentinel files.
# This creates a list where the parent pid is always listed
# before the child pid.
proc findprocessids { testdir plist }  {
	set beginfiles [glob $testdir/begin.*]
	foreach b $beginfiles {
		regsub $testdir/begin. $b {} pid
		if { [lsearch -exact $plist $pid] == -1 } {
			lappend plist $pid
		}
	}
	return $plist
}

