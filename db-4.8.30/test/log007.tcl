# See the file LICENSE for redistribution information.
#
# Copyright (c) 2005-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	log007
# TEST	Test of in-memory logging bugs. [#11505]
# TEST
# TEST	Test db_printlog with in-memory logs.
#
proc log007 { } {
	global testdir
	global util_path
	set tnum "007"

	puts "Log$tnum: Test in-memory logs with db_printlog."

	# Log size is small so we quickly create more than one.
	# Since we are in-memory the buffer is larger than the
	# file size.
	set pagesize 4096
	append args " -pagesize $pagesize "
	set log_max [expr $pagesize * 2]
	set log_buf [expr $log_max * 2]

	# We have 13-byte records.  We want to fill slightly more
	# than one virtual log file on each iteration.  The first
	# record always has an offset of 28.
	#
	set recsize 13
	set recsperfile [expr [expr $log_max - 28] / $recsize]
	set nrecs [expr $recsperfile + 1]

	# Open environment.
	env_cleanup $testdir
	set flags " -create -txn -home $testdir \
	    -log_inmemory -log_buffer $log_buf -log_max $log_max"
	set env [eval {berkdb_env} $flags]
	error_check_good env_open [is_valid_env $env] TRUE

	set iter 15
	set lastfile 1
	for { set i 0 } { $i < $iter } { incr i } {
		puts "\tLog$tnum.a.$i: Writing $nrecs 13-byte log records."
		set lsn_list {}
		for { set j 0 } { $j < $nrecs } { incr j } {
			set rec "1"
			# Make the first record one byte larger for each
			# successive log file so we hit the end of the
			# log file at each of the 13 possibilities.
			set nentries [expr [expr $i * $nrecs] + $j]
			if { [expr $nentries % 628] == 0 } {
				append firstrec a
				set ret [$env log_put $firstrec]
			} else {
				set ret [$env log_put $rec]
			}
			error_check_bad log_put [is_substr $ret log_cmd] 1
			lappend lsn_list $ret
		}

		# Open a log cursor.
		set m_logc [$env log_cursor]
		error_check_good m_logc [is_valid_logc $m_logc $env] TRUE

		# Check that we're in the expected virtual log file.
		set first [$m_logc get -first]
		error_check_good first_lsn [lindex $first 0] "[expr $i + 1] 28"
		set last [$m_logc get -last]

		puts "\tLog$tnum.b.$i: Read log records sequentially."
		set j 0
		for { set logrec [$m_logc get -first] } \
		    { [llength $logrec] != 0 } \
		    { set logrec [$m_logc get -next]} {
			set file [lindex [lindex $logrec 0] 0]
			if { $file != $lastfile } {
				# We have entered a new virtual log file.
				set lastfile $file
			}
			set offset [lindex [lindex $logrec 0] 1]
			set lsn($j) "\[$file\]\[$offset\]"
			incr j
		}
		error_check_good cursor_close [$m_logc close] 0

		puts "\tLog$tnum.c.$i: Compare printlog to log records."
		set stat [catch {eval exec $util_path/db_printlog \
		    -h $testdir > $testdir/prlog} result]
		error_check_good stat_prlog $stat 0

		# Make sure the results of printlog contain all the same
		# LSNs we saved when walking the files with the log cursor.
		set j 0
		set fd [open $testdir/prlog r]
		while { [gets $fd record] >= 0 } {
			# A log record begins with "[".
			if { [string match {\[*} $record] == 1 } {
				error_check_good \
				    check_prlog [is_substr $record $lsn($j)] 1
				incr j
			}
		}
		close $fd
	}

	error_check_good env_close [$env close] 0
	error_check_good env_remove [berkdb envremove -home $testdir] 0
}
