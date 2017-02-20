# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	log003
# TEST	Verify that log_flush is flushing records correctly.
proc log003 { } {
	global rand_init
	error_check_good set_random_seed [berkdb srand $rand_init] 0

	# Even though log_flush doesn't do anything for in-memory
	# logging, we want to make sure calling it doesn't break
	# anything.
	foreach inmem { 1 0 } {
		log003_body $inmem
	}
}

proc log003_body { inmem } {
	source ./include.tcl

	puts -nonewline "Log003: Verify log_flush behavior"
	if { $inmem == 0 } {
		puts " (on-disk logging)."
	} else {
		puts " (in-memory logging)."
	}

	set max [expr 1024 * 128]
	env_cleanup $testdir
	set short_rec "abcdefghijklmnopqrstuvwxyz"
	set long_rec [repeat $short_rec 200]
	set very_long_rec [repeat $long_rec 4]

	foreach rec "$short_rec $long_rec $very_long_rec" {
		puts "\tLog003.a: Verify flush on [string length $rec] byte rec"

		set logargs ""
		if { $inmem == 1 } {
			set logargs "-log_inmemory -log_buffer [expr $max * 2]"
		}
		set env [eval {berkdb_env} -log -home $testdir -create \
		    -mode 0644 $logargs -log_max $max]
		error_check_good envopen [is_valid_env $env] TRUE

		set lsn [$env log_put $rec]
		error_check_bad log_put [lindex $lsn 0] "ERROR:"
		set ret [$env log_flush $lsn]
		error_check_good log_flush $ret 0

		# Now, we want to crash the region and recheck.  Closing the
		# log does not flush any records, so we'll use a close to
		# do the "crash"
		set ret [$env close]
		error_check_good log_env:close $ret 0

		# Now, remove the log region
		#set ret [berkdb envremove -home $testdir]
		#error_check_good env:remove $ret 0

		# Re-open the log and try to read the record.
		set env [berkdb_env -create -home $testdir \
				-log -mode 0644 -log_max $max]
		error_check_good envopen [is_valid_env $env] TRUE

		set logc [$env log_cursor]
		error_check_good log_cursor [is_valid_logc $logc $env] TRUE

		set gotrec [$logc get -first]
		error_check_good lp_get [lindex $gotrec 1] $rec

		# Close and unlink the file
		error_check_good log_cursor:close:$logc [$logc close] 0
		error_check_good env:close:$env [$env close] 0
		error_check_good envremove [berkdb envremove -home $testdir] 0
		log_cleanup $testdir
	}

	if { $inmem == 1 } {
		puts "Log003: Skipping remainder of test for in-memory logging."
		return
	}

	foreach rec "$short_rec $long_rec $very_long_rec" {
		puts "\tLog003.b: \
		    Verify flush on non-last record [string length $rec]"

		set env [berkdb_env -log -home $testdir \
		    -create -mode 0644 -log_max $max]

		error_check_good envopen [is_valid_env $env] TRUE

		# Put 10 random records
		for { set i 0 } { $i < 10 } { incr i} {
			set r [random_data 450 0 0]
			set lsn [$env log_put $r]
			error_check_bad log_put [lindex $lsn 0] "ERROR:"
		}

		# Put the record we are interested in
		set save_lsn [$env log_put $rec]
		error_check_bad log_put [lindex $save_lsn 0] "ERROR:"

		# Put 10 more random records
		for { set i 0 } { $i < 10 } { incr i} {
			set r [random_data 450 0 0]
			set lsn [$env log_put $r]
			error_check_bad log_put [lindex $lsn 0] "ERROR:"
		}

		# Now check the flush
		set ret [$env log_flush $save_lsn]
		error_check_good log_flush $ret 0

		# Now, we want to crash the region and recheck.  Closing the
		# log does not flush any records, so we'll use a close to
		# do the "crash".
		#
		# Now, close and remove the log region
		error_check_good env:close:$env [$env close] 0
		set ret [berkdb envremove -home $testdir]
		error_check_good env:remove $ret 0

		# Re-open the log and try to read the record.
		set env [berkdb_env -log -home $testdir \
		    -create -mode 0644 -log_max $max]
		error_check_good envopen [is_valid_env $env] TRUE

		set logc [$env log_cursor]
		error_check_good log_cursor [is_valid_logc $logc $env] TRUE

		set gotrec [$logc get -set $save_lsn]
		error_check_good lp_get [lindex $gotrec 1] $rec

		# Close and unlink the file
		error_check_good log_cursor:close:$logc [$logc close] 0
		error_check_good env:close:$env [$env close] 0
		error_check_good envremove [berkdb envremove -home $testdir] 0
		log_cleanup $testdir
	}
}
