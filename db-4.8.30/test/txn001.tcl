# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	txn001
# TEST	Begin, commit, abort testing.
proc txn001 { {tnum "001"} { max 1024 } { ntxns 50 } } {
	source ./include.tcl
	global txn_curid
	global txn_maxid

	puts -nonewline "Txn$tnum: Basic begin, commit, abort"

	if { $tnum != "001"} {
		puts " (with ID wrap)"
	} else {
		puts ""
	}

	# Open environment
	env_cleanup $testdir

	set env [eval {berkdb_env -create -mode 0644 -txn \
	    -txn_max $max -home $testdir}]
	error_check_good evn_open [is_valid_env $env] TRUE
	error_check_good txn_id_set \
	    [ $env txn_id_set $txn_curid $txn_maxid ] 0
	txn001_suba $ntxns $env $tnum
	txn001_subb $ntxns $env $tnum
	txn001_subc $ntxns $env $tnum
	# Close and unlink the file
	error_check_good env_close:$env [$env close] 0
}

proc txn001_suba { ntxns env tnum } {
	source ./include.tcl

	# We will create a bunch of transactions and commit them.
	set txn_list {}
	set tid_list {}
	puts "\tTxn$tnum.a: Beginning/Committing $ntxns Transactions in $env"
	for { set i 0 } { $i < $ntxns } { incr i } {
		set txn [$env txn]
		error_check_good txn_begin [is_valid_txn $txn $env] TRUE

		lappend txn_list $txn

		set tid [$txn id]
		error_check_good tid_check [lsearch $tid_list $tid] -1

		lappend tid_list $tid
	}

	# Now commit them all
	foreach t $txn_list {
		error_check_good txn_commit:$t [$t commit] 0
	}
}

proc txn001_subb { ntxns env tnum } {
	# We will create a bunch of transactions and abort them.
	set txn_list {}
	set tid_list {}
	puts "\tTxn$tnum.b: Beginning/Aborting Transactions"
	for { set i 0 } { $i < $ntxns } { incr i } {
		set txn [$env txn]
		error_check_good txn_begin [is_valid_txn $txn $env] TRUE

		lappend txn_list $txn

		set tid [$txn id]
		error_check_good tid_check [lsearch $tid_list $tid] -1

		lappend tid_list $tid
	}

	# Now abort them all
	foreach t $txn_list {
		error_check_good txn_abort:$t [$t abort] 0
	}
}

proc txn001_subc { ntxns env tnum } {
	# We will create a bunch of transactions and commit them.
	set txn_list {}
	set tid_list {}
	puts "\tTxn$tnum.c: Beginning/Prepare/Committing Transactions"
	for { set i 0 } { $i < $ntxns } { incr i } {
		set txn [$env txn]
		error_check_good txn_begin [is_valid_txn $txn $env] TRUE

		lappend txn_list $txn

		set tid [$txn id]
		error_check_good tid_check [lsearch $tid_list $tid] -1

		lappend tid_list $tid
	}

	# Now prepare them all
	foreach t $txn_list {
		error_check_good txn_prepare:$t \
		    [$t prepare [make_gid global:$t]] 0
	}

	# Now commit them all
	foreach t $txn_list {
		error_check_good txn_commit:$t [$t commit] 0
	}

}

