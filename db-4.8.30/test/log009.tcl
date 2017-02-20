# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	log009
# TEST	Test of logging and getting log file version information.
# TEST	Each time we cross a log file boundary verify we can
# TEST	get the version via the log cursorlag.
# TEST	Do this both forward and backward.
#
proc log009 { } {
	source ./include.tcl
	global errorInfo

	env_cleanup $testdir
	set niter 200
	set method btree

	puts "Log009: Retrieve log version using log cursor."

	# Log size is small so we quickly create more than one.
	# The documentation says that the log file must be at least
	# four times the size of the in-memory log buffer.
	set pagesize 4096
	append largs " -pagesize $pagesize "
	set log_buf [expr $pagesize * 2]
	set log_max [expr $log_buf * 4]

	# Open an env.
	set envcmd "berkdb_env_noerr -create \
	    -log_buffer $log_buf -log_max $log_max -txn -home $testdir"
	set env [eval $envcmd]
	error_check_good env [is_valid_env $env] TRUE

	set stop 0
	set start 0
	#
	# Loop until we have at least 3 log files.
	#
	while { $stop == 0 } {
		puts "\tLog009.a: Running test in to generate log files."
	 	eval rep_test \
		    $method $env NULL $niter $start $start 0 0 $largs
		incr start $niter

		set last_log [get_logfile $env last]
		if { $last_log >= 3 } {
			set stop 1
		}
	}

	# We now have at least 3 log files.  Walk a cursor both ways
	# through the log and make sure we can get the version when we
	# cross a log file boundary.
	set curfile 0
	set logc [$env log_cursor]
	error_check_good logc [is_valid_logc $logc $env] TRUE

	puts "\tLog009.b: Try to get version on unset cursor."
	set stat [catch {eval $logc version} ret]
	error_check_bad stat $stat 0
	error_check_good err [is_substr $ret "unset cursor"] 1

	# Check walking forward through logs looking for log
	# file boundaries.
	#
	puts "\tLog009.c: Walk log forward checking persist."
	for { set logrec [$logc get -first] } \
	    { [llength $logrec] != 0 } \
	    { set logrec [$logc get -next] } {
		set lsn [lindex $logrec 0]
		set lsnfile [lindex $lsn 0]
		if { $curfile != $lsnfile } {
			log009_check $logc $logrec
			set curfile $lsnfile
		}
	}
	error_check_good logclose [$logc close] 0

	set curfile 0
	set logc [$env log_cursor]
	error_check_good logc [is_valid_logc $logc $env] TRUE
	#
	# Check walking backward through logs looking for log
	# file boundaries.
	#
	puts "\tLog009.d: Walk log backward checking persist."
	for { set logrec [$logc get -last] } \
	    { [llength $logrec] != 0 } \
	    { set logrec [$logc get -prev] } {
		set lsn [lindex $logrec 0]
		set lsnfile [lindex $lsn 0]
		if { $curfile != $lsnfile } {
			log009_check $logc $logrec
			set curfile $lsnfile
		}
	}
	error_check_good logclose [$logc close] 0
	error_check_good env_close [$env close] 0
}

proc log009_check { logc logrec } {
	set version [$logc version]
	#
	# We don't have ready access to the current log
	# version, but make sure it is something reasonable.
	#
	# !!!
	# First readable log is 8, current log version
	# is pretty far from 20.
	#
	set reasonable [expr $version > 7 && $version < 20]
	error_check_good persist $reasonable 1
	#
	# Verify that getting the version doesn't move
	# or change the log cursor in any way.
	#
	set logrec1 [$logc get -current]
	error_check_good current $logrec $logrec1
}
