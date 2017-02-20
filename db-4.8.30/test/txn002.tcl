# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	txn002
# TEST	Verify that  read-only transactions do not write log records.
proc txn002 { {tnum "002" } { max 1024 } { ntxns 50 } } {
	source ./include.tcl
	global txn_curid
	global txn_maxid

	puts -nonewline "Txn$tnum: Read-only transaction test ($max) ($ntxns)"

	if { $tnum != "002" } {
		puts " (with ID wrap)"
	} else {
		puts ""
	}

	env_cleanup $testdir
	set env [berkdb \
	    env -create -mode 0644 -txn -txn_max $max -home $testdir]
	error_check_good dbenv [is_valid_env $env] TRUE
	error_check_good txn_id_set \
	    [$env txn_id_set $txn_curid $txn_maxid ] 0

	# Save the current bytes in the log.
	set off_start [txn002_logoff $env]

	# We will create a bunch of transactions and commit them.
	set txn_list {}
	set tid_list {}
	puts "\tTxn$tnum.a: Beginning/Committing Transactions"
	for { set i 0 } { $i < $ntxns } { incr i } {
		set txn [$env txn]
		error_check_good txn_begin [is_valid_txn $txn $env] TRUE

		lappend txn_list $txn

		set tid [$txn id]
		error_check_good tid_check [lsearch $tid_list $tid] -1

		lappend tid_list $tid
	}
	foreach t $txn_list {
		error_check_good txn_commit:$t [$t commit] 0
	}

	# Make sure we haven't written any new log records except
	# potentially some recycle records if we were wrapping txnids.
	set off_stop [txn002_logoff $env]
	if { $off_stop != $off_start } {
		txn002_recycle_only $testdir
	}

	error_check_good env_close [$env close] 0
}

proc txn002_logoff { env } {
	set stat [$env log_stat]
	foreach i $stat {
		foreach {txt val} $i {break}
			if { [string compare \
			    $txt {Current log file offset}] == 0 } {
			return $val
		}
	}
}

# Make sure that the only log records found are txn_recycle records
proc txn002_recycle_only { dir } {
	global util_path

	set tmpfile $dir/printlog.out
	set stat [catch {exec $util_path/db_printlog -h $dir > $tmpfile} ret]
	error_check_good db_printlog $stat 0

	set f [open $tmpfile r]
	while { [gets $f record] >= 0 } {
		set r [regexp {\[[^\]]*\]\[[^\]]*\]([^\:]*)\:} $record whl name]
		if { $r == 1 } {
			error_check_good record_type __txn_recycle $name
		}
	}
	close $f
	fileremove $tmpfile
}
