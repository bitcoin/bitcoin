# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# Options are:
# -checkrec <checkpoint frequency"
# -dir <dbhome directory>
# -maxfilesize <maxsize of log file>
proc archive { { inmem 0 } args } {
	global alphabet
	source ./include.tcl

	# Set defaults
	if { $inmem == 1 } {
		set maxbsize [expr 8 * [expr 1024 * 1024]]
		set desc "in-memory"
	} else {
		set maxbsize [expr 8 * 1024]
		set desc "on-disk"
	}
	set maxfile [expr 32 * 1024]
	set checkrec 500
	for { set i 0 } { $i < [llength $args] } {incr i} {
		switch -regexp -- [lindex $args $i] {
			-c.* { incr i; set checkrec [lindex $args $i] }
			-d.* { incr i; set testdir [lindex $args $i] }
			-m.* { incr i; set maxfile [lindex $args $i] }
			default {
				puts "FAIL:[timestamp] archive usage"
	puts "usage: archive -checkrec <checkpt freq> \
	    -dir <directory> -maxfilesize <max size of log files>"
				return
			}
		}
	}

	# Clean out old log if it existed
	puts "Archive: Log archive test (using $desc logging)."
	puts "Unlinking log: error message OK"
	env_cleanup $testdir

	# Now run the various functionality tests
	if { $inmem == 0 } {
		set eflags "-create -txn -home $testdir \
		    -log_buffer $maxbsize -log_max $maxfile"
	} else {
		set eflags "-create -txn -home $testdir -log_inmemory \
		    -log_buffer $maxbsize -log_max $maxfile"
	}
	set dbenv [eval {berkdb_env} $eflags]
	error_check_good dbenv [is_valid_env $dbenv] TRUE

	set logc [$dbenv log_cursor]
	error_check_good log_cursor [is_valid_logc $logc $dbenv] TRUE

	# The basic test structure here is that we write a lot of log
	# records (enough to fill up 100 log files; each log file it
	# small).  We start with three txns and open a database in
	# each transaction.  Then, in a loop, we take periodic
	# checkpoints.  Between each pair of checkpoints, we end one
	# transaction; when no transactions are left, we start up three
	# new ones, letting them overlap checkpoints as well.
	#
	# The pattern that we create is:
	# 1.  Create TXN1, TXN2, TXN3 and open dbs within the txns.
	# 2.  Write a bunch of additional log records.
	# 3.  Checkpoint.
	# 4.  Archive, checking that we list the right files.
	# 5.  Commit one transaction.
	# 6.  If no txns left, start 3 new ones.
	# 7.  Until we've gone through enough records, return to step 2.

	set baserec "1:$alphabet:2:$alphabet:3:$alphabet:4:$alphabet"
	puts "\tArchive.a: Writing log records; checkpoint every $checkrec records"
	set nrecs $maxfile
	set rec 0:$baserec

	# Begin 1st transaction and record current log file.  Open
	# a database in the transaction; the log file won't be
	# removable until the transaction is aborted or committed.
	set t1 [$dbenv txn]
	error_check_good t1:txn_begin [is_valid_txn $t1 $dbenv] TRUE

	set l1 [lindex [lindex [$logc get -last] 0] 0]
	set lsnlist [list $l1]

	set tdb1 [eval {berkdb_open -create -mode 0644} \
	    -env $dbenv -txn $t1 -btree tdb1.db]
	error_check_good dbopen [is_valid_db $tdb1] TRUE

	# Do the same for a 2nd and 3rd transaction.
	set t2 [$dbenv txn]
	error_check_good t2:txn_begin [is_valid_txn $t2 $dbenv] TRUE
	set l2 [lindex [lindex [$logc get -last] 0] 0]
	lappend lsnlist $l2
	set tdb2 [eval {berkdb_open -create -mode 0644} \
	    -env $dbenv -txn $t2 -btree tdb2.db]
	error_check_good dbopen [is_valid_db $tdb2] TRUE

	set t3 [$dbenv txn]
	error_check_good t3:txn_begin [is_valid_txn $t3 $dbenv] TRUE
	set l3 [lindex [lindex [$logc get -last] 0] 0]
	lappend lsnlist $l3
	set tdb3 [eval {berkdb_open -create -mode 0644} \
	    -env $dbenv -txn $t3 -btree tdb3.db]
	error_check_good dbopen [is_valid_db $tdb3] TRUE

	# Keep a list of active transactions and databases opened
	# within those transactions.
	set txnlist [list "$t1 $tdb1" "$t2 $tdb2" "$t3 $tdb3"]

	# Loop through a large number of log records, checkpointing
	# and checking db_archive periodically.
	for { set i 1 } { $i <= $nrecs } { incr i } {
		set rec $i:$baserec
		set lsn [$dbenv log_put $rec]
		error_check_bad log_put [llength $lsn] 0
		if { [expr $i % $checkrec] == 0 } {

			# Take a checkpoint
			$dbenv txn_checkpoint
			set ckp_file [lindex [lindex [$logc get -last] 0] 0]
			catch { archive_command -h $testdir -a } res_log_full
			if { [string first db_archive $res_log_full] == 0 } {
				set res_log_full ""
			}
			catch { archive_command -h $testdir } res_log
			if { [string first db_archive $res_log] == 0 } {
				set res_log ""
			}
			catch { archive_command -h $testdir -l } res_alllog
			catch { archive_command -h $testdir -a -s } \
			    res_data_full
			catch { archive_command -h $testdir -s } res_data

			if { $inmem == 0 } {
				error_check_good nlogfiles [llength $res_alllog] \
				    [lindex [lindex [$logc get -last] 0] 0]
			} else {
				error_check_good nlogfiles [llength $res_alllog] 0
			}

			error_check_good logs_match [llength $res_log_full] \
			    [llength $res_log]
			error_check_good data_match [llength $res_data_full] \
			    [llength $res_data]

			# Check right number of log files
			if { $inmem == 0 } {
				set expected [min $ckp_file [expr [lindex $lsnlist 0] - 1]]
				error_check_good nlogs [llength $res_log] $expected
			}

			# Check that the relative names are a subset of the
			# full names
			set n 0
			foreach x $res_log {
				error_check_bad log_name_match:$res_log \
				    [string first $x \
				    [lindex $res_log_full $n]] -1
				incr n
			}

			set n 0
			foreach x $res_data {
				error_check_bad log_name_match:$res_data \
				    [string first $x \
				    [lindex $res_data_full $n]] -1
				incr n
			}

			# Commit a transaction and close the associated db.
			set t [lindex [lindex $txnlist 0] 0]
			set tdb [lindex [lindex $txnlist 0] 1]
			if { [string length $t] != 0 } {
				error_check_good txn_commit:$t [$t commit] 0
				error_check_good tdb_close:$tdb [$tdb close] 0
				set txnlist [lrange $txnlist 1 end]
				set lsnlist [lrange $lsnlist 1 end]
			}

			# If we're down to no transactions, start some new ones.
			if { [llength $txnlist] == 0 } {
				set t1 [$dbenv txn]
				error_check_bad tx_begin $t1 NULL
				error_check_good \
				    tx_begin [is_substr $t1 $dbenv] 1
				set tdb1 [eval {berkdb_open -create -mode 0644} \
				    -env $dbenv -txn $t1 -btree tdb1.db]
				error_check_good dbopen [is_valid_db $tdb1] TRUE
				set l1 [lindex [lindex [$logc get -last] 0] 0]
				lappend lsnlist $l1

				set t2 [$dbenv txn]
				error_check_bad tx_begin $t2 NULL
				error_check_good \
				    tx_begin [is_substr $t2 $dbenv] 1
				set tdb2 [eval {berkdb_open -create -mode 0644} \
				    -env $dbenv -txn $t2 -btree tdb2.db]
				error_check_good dbopen [is_valid_db $tdb2] TRUE
				set l2 [lindex [lindex [$logc get -last] 0] 0]
				lappend lsnlist $l2

				set t3 [$dbenv txn]
				error_check_bad tx_begin $t3 NULL
				error_check_good \
				    tx_begin [is_substr $t3 $dbenv] 1
				set tdb3 [eval {berkdb_open -create -mode 0644} \
				    -env $dbenv -txn $t3 -btree tdb3.db]
				error_check_good dbopen [is_valid_db $tdb3] TRUE
				set l3 [lindex [lindex [$logc get -last] 0] 0]
				lappend lsnlist $l3

				set txnlist [list "$t1 $tdb1" "$t2 $tdb2" "$t3 $tdb3"]
			}
		}
	}
	# Commit any transactions still running.
	puts "\tArchive.b: Commit any transactions still running."
	foreach pair $txnlist {
		set t [lindex $pair 0]
		set tdb [lindex $pair 1]
		error_check_good txn_commit:$t [$t commit] 0
		error_check_good tdb_close:$tdb [$tdb close] 0
	}

	# Close and unlink the file
	error_check_good log_cursor_close [$logc close] 0
	reset_env $dbenv
}

proc archive_command { args } {
	source ./include.tcl

	# Catch a list of files output by db_archive.
	catch { eval exec $util_path/db_archive $args } output

	if { $is_windows_test == 1 || 1 } {
		# On Windows, convert all filenames to use forward slashes.
		regsub -all {[\\]} $output / output
	}

	# Output the [possibly-transformed] list.
	return $output
}

proc min { a b } {
	if {$a < $b} {
		return $a
	} else {
		return $b
	}
}
