# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	log002
# TEST	Tests multiple logs
# TEST		Log truncation
# TEST		LSN comparison and file functionality.
proc log002 { } {
	global rand_init
	error_check_good set_random_seed [berkdb srand $rand_init] 0

	foreach inmem { 1 0 } {
		log002_body $inmem
	}
}

proc log002_body { inmem } {
	source ./include.tcl

	puts "Log002: Multiple log test w/trunc, file, compare functionality"

	env_cleanup $testdir

	set max [expr 1024 * 128]

	set logargs ""
	if { $inmem == 0 } {
		puts "Log002: Using on-disk logging."
	} else {
		puts "Log002: Using in-memory logging."
		set lbuf [expr 8 * [expr 1024 * 1024]]
		set logargs "-log_inmemory -log_buffer $lbuf"
	}
	set env [eval {berkdb_env} -create -home $testdir -log \
	    -mode 0644 $logargs -log_max $max]
	error_check_good envopen [is_valid_env $env] TRUE

	# We'll record every hundredth record for later use
	set info_list {}

	puts "\tLog002.a: Writing log records"
	set i 0
	for {set s 0} { $s <  [expr 3 * $max] } { incr s $len } {
		set rec [random_data 120 0 0]
		set len [string length $rec]
		set lsn [$env log_put $rec]

		if { [expr $i % 100 ] == 0 } {
			lappend info_list [list $lsn $rec]
		}
		incr i
	}

	puts "\tLog002.b: Checking log_compare"
	set last {0 0}
	foreach p $info_list {
		set l [lindex $p 0]
		if { [llength $last] != 0 } {
			error_check_good \
			    log_compare [$env log_compare $l $last] 1
			error_check_good \
			    log_compare [$env log_compare $last $l] -1
			error_check_good \
			    log_compare [$env log_compare $l $l] 0
		}
		set last $l
	}

	puts "\tLog002.c: Checking log_file"
	if { $inmem == 0 } {
		set flist [glob $testdir/log*]
		foreach p $info_list {
			set lsn [lindex $p 0]
			set f [$env log_file $lsn]

			# Change backslash separators on Windows to forward
			# slashes, as the rest of the test suite expects.
			regsub -all {\\} $f {/} f
			error_check_bad log_file:$f [lsearch $flist $f] -1
		}
	}

	puts "\tLog002.d: Verifying records"

	set logc [$env log_cursor]
	error_check_good log_cursor [is_valid_logc $logc $env] TRUE

	for {set i [expr [llength $info_list] - 1] } { $i >= 0 } { incr i -1} {
		set p [lindex $info_list $i]
		set grec [$logc get -set [lindex $p 0]]
		error_check_good log_get:$env [lindex $grec 1] [lindex $p 1]
	}

	# Close and unlink the file
	error_check_good log_cursor:close:$logc [$logc close] 0
	error_check_good env:close [$env close] 0
	error_check_good envremove [berkdb envremove -home $testdir] 0
}
