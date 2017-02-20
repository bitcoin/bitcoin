# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# Txn003 script - outstanding child prepare script
# Usage: txnscript envcmd dbcmd gidf key data
# envcmd: command to open env
# dbfile: name of database file
# gidf: name of global id file
# key: key to use
# data: new data to use

source ./include.tcl
source $test_path/test.tcl
source $test_path/testutils.tcl

set usage "txnscript envcmd dbfile gidfile key data"

# Verify usage
if { $argc != 5 } {
	puts stderr "FAIL:[timestamp] Usage: $usage"
	exit
}

# Initialize arguments
set envcmd [ lindex $argv 0 ]
set dbfile [ lindex $argv 1 ]
set gidfile [ lindex $argv 2 ]
set key [ lindex $argv 3 ]
set data [ lindex $argv 4 ]

set dbenv [eval $envcmd]
error_check_good envopen [is_valid_env $dbenv] TRUE

set usedb 1
set db [berkdb_open -auto_commit -env $dbenv $dbfile]
error_check_good dbopen [is_valid_db $db] TRUE

puts "\tTxnscript.a: begin parent and child txn"
set parent [$dbenv txn]
error_check_good parent [is_valid_txn $parent $dbenv] TRUE
set child [$dbenv txn -parent $parent]
error_check_good parent [is_valid_txn $child $dbenv] TRUE

puts "\tTxnscript.b: Modify data"
error_check_good db_put [$db put -txn $child $key $data] 0

set gfd [open $gidfile w+]
set gid [make_gid txnscript:$parent]
puts $gfd $gid
puts "\tTxnscript.c: Prepare parent only"
error_check_good txn_prepare:$parent [$parent prepare $gid] 0
close $gfd

puts "\tTxnscript.d: Check child handle"
set stat [catch {$child abort} ret]
error_check_good child_handle $stat 1
error_check_good child_h2 [is_substr $ret "invalid command name"] 1

#
# We do not close the db or env, but exit with the txns outstanding.
#
puts "\tTxnscript completed successfully"
flush stdout
