# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	lock003
# TEST	Exercise multi-process aspects of lock.  Generate a bunch of parallel
# TEST	testers that try to randomly obtain locks;  make sure that the locks
# TEST	correctly protect corresponding objects.
proc lock003 { {iter 500} {max 1000} {procs 5} } {
	source ./include.tcl
	global lock_curid
	global lock_maxid

	set ldegree 5
	set objs 75
	set reads 65
	set wait 1
	set conflicts { 0 0 0 0 0 1 0 1 1}
	set seeds {}

	puts "Lock003: Multi-process random lock test"

	# Clean up after previous runs
	env_cleanup $testdir

	# Open/create the lock region
	puts "\tLock003.a: Create environment"
	set e [berkdb_env -create -lock -home $testdir]
	error_check_good env_open [is_substr $e env] 1
	$e lock_id_set $lock_curid $lock_maxid

	error_check_good env_close [$e close] 0

	# Now spawn off processes
	set pidlist {}

	for { set i 0 } {$i < $procs} {incr i} {
		if { [llength $seeds] == $procs } {
			set s [lindex $seeds $i]
		}
#		puts "$tclsh_path\
#		    $test_path/wrap.tcl \
#		    lockscript.tcl $testdir/$i.lockout\
#		    $testdir $iter $objs $wait $ldegree $reads &"
		set p [exec $tclsh_path $test_path/wrap.tcl \
		    lockscript.tcl $testdir/lock003.$i.out \
		    $testdir $iter $objs $wait $ldegree $reads &]
		lappend pidlist $p
	}

	puts "\tLock003.b: $procs independent processes now running"
	watch_procs $pidlist 30 10800

	# Check for test failure
	set errstrings [eval findfail [glob $testdir/lock003.*.out]]
	foreach str $errstrings {
		puts "FAIL: error message in .out file: $str"
	}

	# Remove log files
	for { set i 0 } {$i < $procs} {incr i} {
		fileremove -f $testdir/lock003.$i.out
	}
}

# Create and destroy flag files to show we have an object locked, and
# verify that the correct files exist or don't exist given that we've
# just read or write locked a file.
proc lock003_create { rw obj } {
	source ./include.tcl

	set pref $testdir/L3FLAG
	set f [open $pref.$rw.[pid].$obj w]
	close $f
}

proc lock003_destroy { obj } {
	source ./include.tcl

	set pref $testdir/L3FLAG
	set f [glob -nocomplain $pref.*.[pid].$obj]
	error_check_good l3_destroy [llength $f] 1
	fileremove $f
}

proc lock003_vrfy { rw obj } {
	source ./include.tcl

	set pref $testdir/L3FLAG
	if { [string compare $rw "write"] == 0 } {
		set fs [glob -nocomplain $pref.*.*.$obj]
		error_check_good "number of other locks on $obj" [llength $fs] 0
	} else {
		set fs [glob -nocomplain $pref.write.*.$obj]
		error_check_good "number of write locks on $obj" [llength $fs] 0
	}
}

