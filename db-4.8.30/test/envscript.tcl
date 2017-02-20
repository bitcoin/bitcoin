# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# $Id$
#
# Envscript -- for use with env012, DB_REGISTER test.
# Usage: envscript testdir testfile putget key data recover failchk wait
# testdir: directory containing the env we are joining.
# testfile: file name for database.
# putget: What to do in the db: put, get, or loop.
# key: key to store or get
# data: data to store or get
# recover: include or omit the -recover flag in opening the env.
# failchk: include or omit the -failchk flag in opening the env. 2 options
#     here, one with just -failchk and one with both -failchk & -isalive
# wait: how many seconds to wait before closing env at end of test.

source ./include.tcl
source $test_path/testutils.tcl

set usage "envscript testdir testfile putget key data recover failchk wait"

# Verify usage
if { $argc != 8 } {
	puts stderr "FAIL:[timestamp] Usage: $usage"
	exit
}

# Initialize arguments
set testdir [ lindex $argv 0 ]
set testfile [ lindex $argv 1 ]
set putget [lindex $argv 2 ]
set key [ lindex $argv 3 ]
set data [ lindex $argv 4 ]
set recover [ lindex $argv 5 ]
set failchk [lindex $argv 6 ]
set wait [ lindex $argv 7 ]

set flag1 {}
if { $recover == "RECOVER" } {
	set flag1 " -recover "
}

set flag2 {}
if {$failchk == "FAILCHK0" } {
	set flag2 " -failchk "
}
if {$failchk == "FAILCHK1"} {
	set flag2 " -failchk -isalive my_isalive -reg_timeout 100 "
}

# Open and register environment.
if {[catch {eval {berkdb_env} \
    -create -home $testdir -txn -register $flag1 $flag2} dbenv]} {
    	puts "FAIL: opening env returned $dbenv"
}
error_check_good envopen [is_valid_env $dbenv] TRUE

# Open database, put or get, close database.
if {[catch {eval {berkdb_open} \
    -create -auto_commit -btree -env $dbenv $testfile} db]} {
	puts "FAIL: opening db returned $db"
}
error_check_good dbopen [is_valid_db $db] TRUE

switch $putget {
	PUT {
		set txn [$dbenv txn]
		error_check_good db_put [eval {$db put} -txn $txn $key $data] 0
		error_check_good txn_commit [$txn commit] 0
	}
	GET {
		set ret [$db get $key]
		error_check_good db_get [lindex [lindex $ret 0] 1] $data
	}
	LOOP {
		while { 1 } {
			set txn [$dbenv txn]
			error_check_good db_put \
			    [eval {$db put} -txn $txn $key $data] 0
			error_check_good txn_commit [$txn commit] 0
			tclsleep 1
		}
	}
	default {
		puts "FAIL: Unrecognized putget value $putget"
	}
}

error_check_good db_close [$db close] 0

# Wait.
while { $wait > 0 } {
puts "waiting ... wait is $wait"
	tclsleep 1
	incr wait -1
}

error_check_good env_close [$dbenv close] 0
