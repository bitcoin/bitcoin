# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# Recd15 - lots of txns - txn prepare script
# Usage: recd15script envcmd dbcmd gidf numtxns
# envcmd: command to open env
# dbfile: name of database file
# gidf: name of global id file
# numtxns: number of txns to start

source ./include.tcl
source $test_path/test.tcl
source $test_path/testutils.tcl

set usage "recd15script envcmd dbfile gidfile numtxns"

# Verify usage
if { $argc != 4 } {
	puts stderr "FAIL:[timestamp] Usage: $usage"
	exit
}

# Initialize arguments
set envcmd [ lindex $argv 0 ]
set dbfile [ lindex $argv 1 ]
set gidfile [ lindex $argv 2 ]
set numtxns [ lindex $argv 3 ]

set txnmax [expr $numtxns + 5]
set dbenv [eval $envcmd]
error_check_good envopen [is_valid_env $dbenv] TRUE

set usedb 0
if { $dbfile != "NULL" } {
	set usedb 1
	set db [berkdb_open -auto_commit -env $dbenv $dbfile]
	error_check_good dbopen [is_valid_db $db] TRUE
}

puts "\tRecd015script.a: Begin $numtxns txns"
for {set i 0} {$i < $numtxns} {incr i} {
	set t [$dbenv txn]
	error_check_good txnbegin($i) [is_valid_txn $t $dbenv] TRUE
	set txns($i) $t
	if { $usedb } {
		set dbc [$db cursor -txn $t]
		error_check_good cursor($i) [is_valid_cursor $dbc $db] TRUE
		set curs($i) $dbc
	}
}

puts "\tRecd015script.b: Prepare $numtxns txns"
set gfd [open $gidfile w+]
for {set i 0} {$i < $numtxns} {incr i} {
	if { $usedb } {
		set dbc $curs($i)
		error_check_good dbc_close [$dbc close] 0
	}
	set t $txns($i)
	set gid [make_gid recd015script:$t]
	puts $gfd $gid
	error_check_good txn_prepare:$t [$t prepare $gid] 0
}
close $gfd

#
# We do not close the db or env, but exit with the txns outstanding.
#
puts "\tRecd015script completed successfully"
flush stdout
