# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#

# TEST	log001
# TEST	Read/write log records.
# TEST	Test with and without fixed-length, in-memory logging,
# TEST	and encryption.
proc log001 { } {
	global passwd
	global has_crypto
	global rand_init

	berkdb srand $rand_init
	set iter 1000

	set max [expr 1024 * 128]
	foreach fixedlength { 0 1 } {
		foreach inmem { 1 0 } {
			log001_body $max $iter $fixedlength $inmem
			log001_body $max [expr $iter * 15] $fixedlength $inmem

			# Skip encrypted tests if not supported.
			if { $has_crypto == 0 } {
				continue
			}
			log001_body $max\
			    $iter $fixedlength $inmem "-encryptaes $passwd"
			log001_body $max\
			    [expr $iter * 15] $fixedlength $inmem "-encryptaes $passwd"
		}
	}
}

proc log001_body { max nrecs fixedlength inmem {encargs ""} } {
	source ./include.tcl

	puts -nonewline "Log001: Basic put/get log records: "
	if { $fixedlength == 1 } {
		puts -nonewline "fixed-length ($encargs)"
	} else {
		puts -nonewline "variable-length ($encargs)"
	}

	# In-memory logging requires a large enough log buffer that
	# any active transaction can be aborted.
	if { $inmem == 1 } {
		set lbuf [expr 8 * [expr 1024 * 1024]]
		puts " with in-memory logging."
	} else {
		puts " with on-disk logging."
	}

	env_cleanup $testdir

	set logargs ""
	if { $inmem == 1 } {
		set logargs "-log_inmemory -log_buffer $lbuf"
	}
	set env [eval {berkdb_env -log -create -home $testdir -mode 0644} \
	    $encargs $logargs -log_max $max]
	error_check_good envopen [is_valid_env $env] TRUE

	# We will write records to the log and make sure we can
	# read them back correctly.  We'll use a standard pattern
	# repeated some number of times for each record.
	set lsn_list {}
	set rec_list {}
	puts "\tLog001.a: Writing $nrecs log records"
	for { set i 0 } { $i < $nrecs } { incr i } {
		set rec ""
		for { set j 0 } { $j < [expr $i % 10 + 1] } {incr j} {
			set rec $rec$i:logrec:$i
		}
		if { $fixedlength != 1 } {
			set rec $rec:[random_data 237 0 0]
		}
		set lsn [$env log_put $rec]
		error_check_bad log_put [is_substr $lsn log_cmd] 1
		lappend lsn_list $lsn
		lappend rec_list $rec
	}

	# Open a log cursor.
	set logc [$env log_cursor]
	error_check_good logc [is_valid_logc $logc $env] TRUE

	puts "\tLog001.b: Retrieving log records sequentially (forward)"
	set i 0
	for { set grec [$logc get -first] } { [llength $grec] != 0 } {
		set grec [$logc get -next]} {
		error_check_good log_get:seq [lindex $grec 1] \
						 [lindex $rec_list $i]
		incr i
	}

	puts "\tLog001.c: Retrieving log records sequentially (backward)"
	set i [llength $rec_list]
	for { set grec [$logc get -last] } { [llength $grec] != 0 } {
	    set grec [$logc get -prev] } {
		incr i -1
		error_check_good \
		    log_get:seq [lindex $grec 1] [lindex $rec_list $i]
	}

	puts "\tLog001.d: Retrieving log records sequentially by LSN"
	set i 0
	foreach lsn $lsn_list {
		set grec [$logc get -set $lsn]
		error_check_good \
		    log_get:seq [lindex $grec 1] [lindex $rec_list $i]
		incr i
	}

	puts "\tLog001.e: Retrieving log records randomly by LSN"
	set m [expr [llength $lsn_list] - 1]
	for { set i 0 } { $i < $nrecs } { incr i } {
		set recno [berkdb random_int 0 $m ]
		set lsn [lindex $lsn_list $recno]
		set grec [$logc get -set $lsn]
		error_check_good \
		    log_get:seq [lindex $grec 1] [lindex $rec_list $recno]
	}

	puts "\tLog001.f: Retrieving first/current, last/current log record"
	set grec [$logc get -first]
	error_check_good log_get:seq [lindex $grec 1] [lindex $rec_list 0]
	set grec [$logc get -current]
	error_check_good log_get:seq [lindex $grec 1] [lindex $rec_list 0]
	set i [expr [llength $rec_list] - 1]
	set grec [$logc get -last]
	error_check_good log_get:seq [lindex $grec 1] [lindex $rec_list $i]
	set grec [$logc get -current]
	error_check_good log_get:seq [lindex $grec 1] [lindex $rec_list $i]

	# Close and unlink the file
	error_check_good log_cursor:close:$logc [$logc close] 0
	error_check_good env:close [$env close] 0
	error_check_good envremove [berkdb envremove -home $testdir] 0
}
