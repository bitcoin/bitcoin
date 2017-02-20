# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#

# TEST	log004
# TEST	Make sure that if we do PREVs on a log, but the beginning of the
# TEST	log has been truncated, we do the right thing.
proc log004 { } {
	foreach inmem { 1 0 } {
		log004_body $inmem
	}
}

proc log004_body { inmem } {
	source ./include.tcl

	puts "Log004: Prev on log when beginning of log has been truncated."
	# Use archive test to populate log
	env_cleanup $testdir
	puts "\tLog004.a: Call archive to populate log."
	archive $inmem

	# Delete all log files under 100
	puts "\tLog004.b: Delete all log files under 100."
	set ret [catch { glob $testdir/log.00000000* } result]
	if { $ret == 0 } {
		eval fileremove -f $result
	}

	# Now open the log and get the first record and try a prev
	puts "\tLog004.c: Open truncated log, attempt to access missing portion."
	set env [berkdb_env -create -log -home $testdir]
	error_check_good envopen [is_valid_env $env] TRUE

	set logc [$env log_cursor]
	error_check_good log_cursor [is_valid_logc $logc $env] TRUE

	set ret [$logc get -first]
	error_check_bad log_get [llength $ret] 0

	# This should give DB_NOTFOUND which is a ret of length 0
	catch {$logc get -prev} ret
	error_check_good log_get_prev [string length $ret] 0

	puts "\tLog004.d: Close log and environment."
	error_check_good log_cursor_close [$logc close] 0
	error_check_good log_close [$env close] 0
}
