# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	log005
# TEST	Check that log file sizes can change on the fly.
proc log005 { } {

	# Skip the test for HP-UX, where we can't do the second
	# env open.
	global is_hp_test
	if { $is_hp_test == 1 } {
		puts "Log005: Skipping for HP-UX."
		return
	}

	foreach inmem { 1 0 } {
		log005_body $inmem
	}
}
proc log005_body { inmem } {
	source ./include.tcl
	env_cleanup $testdir

	puts -nonewline "Log005: Check that log file sizes can change"
	if { $inmem == 0 } {
		puts " (on-disk logging)."
	} else {
		puts " (in-memory logging)."
	}

	# Open the environment, set and check the log file size.
	puts "\tLog005.a: open, set and check the log file size."
	set logargs ""
	if { $inmem == 1 } {
		set lbuf [expr 1024 * 1024]
		set logargs "-log_inmemory -log_buffer $lbuf"
	}
	set env [eval {berkdb_env} -create -home $testdir \
	    $logargs -log_max 1000000 -txn]
	error_check_good envopen [is_valid_env $env] TRUE
	set db [berkdb_open \
	    -env $env -create -mode 0644 -btree -auto_commit a.db]
	error_check_good dbopen [is_valid_db $db] TRUE

	# Get the current log file maximum.
	set max [log005_stat $env "Current log file size"]
	error_check_good max_set $max 1000000

	# Reset the log file size using a second open, and make sure
	# it changes.
	puts "\tLog005.b: reset during open, check the log file size."
	set envtmp [berkdb_env -home $testdir -log_max 900000 -txn]
	error_check_good envtmp_open [is_valid_env $envtmp] TRUE
	error_check_good envtmp_close [$envtmp close] 0

	set tmp [log005_stat $env "Current log file size"]
	error_check_good max_changed 900000 $tmp

	puts "\tLog005.c: fill in the current log file size."
	# Fill in the current log file.
	set new_lsn 0
	set data [repeat "a" 1024]
	for { set i 1 } \
	    { [log005_stat $env "Current log file number"] != 2 } \
	    { incr i } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set ret [$db put -txn $t $i $data]
		error_check_good put $ret 0
		error_check_good txn [$t commit] 0

		set last_lsn $new_lsn
		set new_lsn [log005_stat $env "Current log file offset"]
	}

	# The last LSN in the first file should be more than our new
	# file size.
	error_check_good "lsn check < 900000" [expr 900000 < $last_lsn] 1

	# Close down the environment.
	error_check_good db_close [$db close] 0
	error_check_good env_close [$env close] 0

	if { $inmem == 1 } {
		puts "Log005: Skipping remainder of test for in-memory logging."
		return
	}

	puts "\tLog005.d: check the log file size is unchanged after recovery."
	# Open again, running recovery.  Verify the log file size is as we
	# left it.
	set env [berkdb_env -create -home $testdir -recover -txn]
	error_check_good env_open [is_valid_env $env] TRUE

	set tmp [log005_stat $env "Current log file size"]
	error_check_good after_recovery 900000 $tmp

	error_check_good env_close [$env close] 0
}

# log005_stat --
#	Return the current log statistics.
proc log005_stat { env s } {
	set stat [$env log_stat]
	foreach statpair $stat {
		set statmsg [lindex $statpair 0]
		set statval [lindex $statpair 1]
		if {[is_substr $statmsg $s] != 0} {
			return $statval
		}
	}
	puts "FAIL: log005: stat string $s not found"
	return 0
}
