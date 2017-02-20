# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# Code to load up the tests in to the Queue database
# $Id$
proc load_queue { file  {dbdir RUNQUEUE} nitems } {
	global serial_tests
	global num_serial
	global num_parallel

	puts -nonewline "Loading run queue with $nitems items..."
	flush stdout

	set env [berkdb_env -create -lock -home $dbdir]
	error_check_good dbenv [is_valid_env $env] TRUE

	# Open two databases, one for tests that may be run
	# in parallel, the other for tests we want to run
	# while only a single process is testing.
	set db [eval {berkdb_open -env $env -create \
            -mode 0644 -len 200 -queue queue.db} ]
        error_check_good dbopen [is_valid_db $db] TRUE
	set serialdb [eval {berkdb_open -env $env -create \
            -mode 0644 -len 200 -queue serialqueue.db} ]
        error_check_good dbopen [is_valid_db $serialdb] TRUE

	set fid [open $file]

	set count 0

        while { [gets $fid str] != -1 } {
		set testarr($count) $str
		incr count
	}

	# Randomize array of tests.
	set rseed [pid]
	berkdb srand $rseed
	puts -nonewline "randomizing..."
	flush stdout
	for { set i 0 } { $i < $count } { incr i } {
		set tmp $testarr($i)

		# RPC test is very long so force it to run first
		# in full runs.  If we find 'r rpc' as we walk the
		# array, arrange to put it in slot 0 ...
		if { [is_substr $tmp "r rpc"] == 1 && \
		    [string match $nitems ALL] } {
			set j 0
		} else {
			set j [berkdb random_int $i [expr $count - 1]]
		}
		# ... and if 'r rpc' is selected to be swapped with the
		# current item in the array, skip the swap.  If we
		# did the swap and moved to the next item, "r rpc" would
		# never get moved to slot 0.
		if { [is_substr $testarr($j) "r rpc"] && \
		    [string match $nitems ALL] } {
			continue
		}

		set testarr($i) $testarr($j)
		set testarr($j) $tmp
	}

	if { [string compare ALL $nitems] != 0 } {
		set maxload $nitems
	} else {
		set maxload $count
	}

	puts "loading..."
	flush stdout
	set num_serial 0
	set num_parallel 0
	for { set i 0 } { $i < $maxload } { incr i } {
		set str $testarr($i)
		# Push serial tests into serial testing db, others
		# into parallel db.
		if { [is_serial $str] } {
			set ret [eval {$serialdb put -append $str}]
			error_check_good put:serialdb [expr $ret > 0] 1
			incr num_serial
		} else {
			set ret [eval {$db put -append $str}]
			error_check_good put:paralleldb [expr $ret > 0] 1
			incr num_parallel
		}
        }

	error_check_good maxload $maxload [expr $num_serial + $num_parallel]
	puts "Loaded $maxload records: $num_serial in serial,\
	    $num_parallel in parallel."
	close $fid
	$db close
	$serialdb close
	$env close
}

proc init_runqueue { {dbdir RUNQUEUE} nitems list} {

	if { [file exists $dbdir] != 1 } {
		file mkdir $dbdir
	}
	puts "Creating test list..."
	$list ALL -n
	load_queue ALL.OUT $dbdir $nitems
	file delete TEST.LIST
	file rename ALL.OUT TEST.LIST
}

proc run_parallel { nprocs {list run_all} {nitems ALL} } {
	global num_serial
	global num_parallel

	# Forcibly remove stuff from prior runs, if it's still there.
	fileremove -f ./RUNQUEUE
	set dirs [glob -nocomplain ./PARALLEL_TESTDIR.*]
	set files [glob -nocomplain ALL.OUT.*]
	foreach file $files {
		fileremove -f $file
	}
	foreach dir $dirs {
		fileremove -f $dir
	}

	set basename ./PARALLEL_TESTDIR
	set queuedir ./RUNQUEUE
	source ./include.tcl

	mkparalleldirs $nprocs $basename $queuedir

	init_runqueue $queuedir $nitems $list

	set basedir [pwd]
	set queuedir ../../[string range $basedir \
	    [string last "/" $basedir] end]/$queuedir

	# Run serial tests in parallel testdir 0.
	run_queue 0 $basename.0 $queuedir serial $num_serial

	set pidlist {}
	# Run parallel tests in testdirs 1 through n.
	for { set i 1 } { $i <= $nprocs } { incr i } {
		set ret [catch {
			set p [exec $tclsh_path << \
			    "source $test_path/test.tcl; run_queue $i \
			    $basename.$i $queuedir parallel $num_parallel" &]
			lappend pidlist $p
			set f [open $testdir/begin.$p w]
			close $f
		} res]
	}
	watch_procs $pidlist 300 1000000

	set failed 0
	for { set i 0 } { $i <= $nprocs } { incr i } {
		if { [file exists ALL.OUT.$i] == 1 } {
			puts -nonewline "Checking output from ALL.OUT.$i ... "
			if { [check_output ALL.OUT.$i] == 1 } {
				set failed 1
			}
			puts " done."
		}
	}
	if { $failed == 0 } {
		puts "Regression tests succeeded."
	} else {
		puts "Regression tests failed."
		puts "Review UNEXPECTED OUTPUT lines above for errors."
		puts "Complete logs found in ALL.OUT.x files"
	}
}

proc run_queue { i rundir queuedir {qtype parallel} {nitems 0} } {
	set builddir [pwd]
	file delete $builddir/ALL.OUT.$i
	cd $rundir

	puts "Starting $qtype run_queue process $i (pid [pid])."

	source ./include.tcl
	global env

	set dbenv [berkdb_env -create -lock -home $queuedir]
	error_check_good dbenv [is_valid_env $dbenv] TRUE

	if { $qtype == "parallel" } {
		set db [eval {berkdb_open -env $dbenv \
     	 	    -mode 0644 -queue queue.db} ]
		error_check_good dbopen [is_valid_db $db] TRUE
	} elseif { $qtype == "serial" } {
		set db [eval {berkdb_open -env $dbenv \
		    -mode 0644 -queue serialqueue.db} ]
		error_check_good serialdbopen [is_valid_db $db] TRUE
	} else {
		puts "FAIL: queue type $qtype not recognized"
	}

	set dbc [eval $db cursor]
        error_check_good cursor [is_valid_cursor $dbc $db] TRUE

	set count 0
	set waitcnt 0
	set starttime [timestamp -r]

	while { $waitcnt < 5 } {
		set line [$db get -consume]
		if { [ llength $line ] > 0 } {
			set cmd [lindex [lindex $line 0] 1]
			set num [lindex [lindex $line 0] 0]
			set o [open $builddir/ALL.OUT.$i a]
			puts $o "\nExecuting record $num ([timestamp -w]):\n"
			set tdir "TESTDIR.$i"
			regsub -all {TESTDIR} $cmd $tdir cmd
			puts $o $cmd
			close $o
			if { [expr {$num % 10} == 0] && $nitems != 0 } {
				puts -nonewline \
				    "Starting test $num of $nitems $qtype items.  "
				set now [timestamp -r]
				set elapsed_secs [expr $now - $starttime]
				set secs_per_test [expr $elapsed_secs / $num]
				set esttotal [expr $nitems * $secs_per_test]
				set remaining [expr $esttotal - $elapsed_secs]
				if { $remaining < 3600 } {
					puts "\tRough guess: less than 1\
					    hour left."
				} else {
					puts "\tRough guess: \
					[expr $remaining / 3600] hour(s) left."
				}
			}
#			puts "Process $i, record $num:\n$cmd"
			set env(PURIFYOPTIONS) \
	"-log-file=./test$num.%p -follow-child-processes -messages=first"
			set env(PURECOVOPTIONS) \
	"-counts-file=./cov.pcv -log-file=./cov.log -follow-child-processes"
			if [catch {exec $tclsh_path \
			     << "source $test_path/test.tcl; $cmd" \
			     >>& $builddir/ALL.OUT.$i } res] {
                                set o [open $builddir/ALL.OUT.$i a]
                                puts $o "FAIL: '$cmd': $res"
                                close $o
                        }
			env_cleanup $testdir
			set o [open $builddir/ALL.OUT.$i a]
			puts $o "\nEnding record $num ([timestamp])\n"
			close $o
			incr count
		} else {
			incr waitcnt
			tclsleep 1
		}
	}

	set now [timestamp -r]
	set elapsed [expr $now - $starttime]
	puts "Process $i: $count commands executed in [format %02u:%02u \
	    [expr $elapsed / 3600] [expr ($elapsed % 3600) / 60]]"

	error_check_good close_parallel_cursor_$i [$dbc close] 0
	error_check_good close_parallel_db_$i [$db close] 0
	error_check_good close_parallel_env_$i [$dbenv close] 0

	#
	# We need to put the pid file in the builddir's idea
	# of testdir, not this child process' local testdir.
	# Therefore source builddir's include.tcl to get its
	# testdir.
	# !!! This resets testdir, so don't do anything else
	# local to the child after this.
	source $builddir/include.tcl

	set f [open $builddir/$testdir/end.[pid] w]
	close $f
	cd $builddir
}

proc mkparalleldirs { nprocs basename queuedir } {
	source ./include.tcl
	set dir [pwd]

	if { $is_windows_test != 1 } {
	        set EXE ""
	} else {
		set EXE ".exe"
        }
	for { set i 0 } { $i <= $nprocs } { incr i } {
		set destdir $basename.$i
		catch {file mkdir $destdir}
		puts "Created $destdir"
		if { $is_windows_test == 1 } {
			catch {file mkdir $destdir/$buildpath}
			catch {eval file copy \
			    [eval glob {$dir/$buildpath/*.dll}] $destdir/$buildpath}
			catch {eval file copy \
			    [eval glob {$dir/$buildpath/db_{checkpoint,deadlock}$EXE} \
			    {$dir/$buildpath/db_{dump,load,printlog,recover,stat,upgrade}$EXE} \
			    {$dir/$buildpath/db_{archive,verify,hotbackup}$EXE}] \
			    {$dir/$buildpath/dbkill$EXE} \
			    $destdir/$buildpath}
			catch {eval file copy \
			    [eval glob -nocomplain {$dir/$buildpath/db_{reptest,repsite}$EXE}] \
			    $destdir/$buildpath}
		}
		catch {eval file copy \
		    [eval glob {$dir/{.libs,include.tcl}}] $destdir}
		# catch {eval file copy $dir/$queuedir $destdir}
		catch {eval file copy \
		    [eval glob {$dir/db_{checkpoint,deadlock}$EXE} \
		    {$dir/db_{dump,load,printlog,recover,stat,upgrade}$EXE} \
		    {$dir/db_{archive,verify,hotbackup}$EXE}] \
		    $destdir}
		catch {eval file copy \
		    [eval glob -nocomplain {$dir/db_{reptest,repsite}$EXE}] $destdir}

		# Create modified copies of include.tcl in parallel
		# directories so paths still work.

		set infile [open ./include.tcl r]
		set d [read $infile]
		close $infile

		regsub {test_path } $d {test_path ../} d
		regsub {src_root } $d {src_root ../} d
		set tdir "TESTDIR.$i"
		regsub -all {TESTDIR} $d $tdir d
		set outfile [open $destdir/include.tcl w]
		puts $outfile $d
		close $outfile

		global svc_list
		foreach svc_exe $svc_list {
			if { [file exists $dir/$svc_exe] } {
				catch {eval file copy $dir/$svc_exe $destdir}
			}
		}
	}
}

proc run_ptest { nprocs test args } {
	global parms
	global valid_methods
	set basename ./PARALLEL_TESTDIR
	set queuedir NULL
	source ./include.tcl

	mkparalleldirs $nprocs $basename $queuedir

	if { [info exists parms($test)] } {
		foreach method $valid_methods {
			if { [eval exec_ptest $nprocs $basename \
			    $test $method $args] != 0 } {
				break
			}
		}
	} else {
		eval exec_ptest $nprocs $basename $test $args
	}
}

proc exec_ptest { nprocs basename test args } {
	source ./include.tcl

	set basedir [pwd]
	set pidlist {}
	puts "Running $nprocs parallel runs of $test"
	for { set i 1 } { $i <= $nprocs } { incr i } {
		set outf ALL.OUT.$i
		fileremove -f $outf
		set ret [catch {
			set p [exec $tclsh_path << \
		 	    "cd $basename.$i;\
		            source ../$test_path/test.tcl;\
		            $test $args" >& $outf &]
			lappend pidlist $p
			set f [open $testdir/begin.$p w]
			close $f
		} res]
	}
	watch_procs $pidlist 30 36000
	set failed 0
	for { set i 1 } { $i <= $nprocs } { incr i } {
		if { [check_output ALL.OUT.$i] == 1 } {
			set failed 1
			puts "Test $test failed in process $i."
		}
	}
	if { $failed == 0 } {
		puts "Test $test succeeded all processes"
		return 0
	} else {
		puts "Test failed: stopping"
		return 1
	}
}
