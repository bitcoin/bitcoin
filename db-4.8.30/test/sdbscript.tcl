# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# Usage: subdbscript testfile subdbnumber factor
# testfile: name of DB itself
# subdbnumber: n, subdb indicator, of form sub$n.db
# factor: Delete over factor'th + n'th from my subdb.
#
# I.e. if factor is 10, and n is 0, remove entries, 0, 10, 20, ...
# if factor is 10 and n is 1, remove entries 1, 11, 21, ...
source ./include.tcl
source $test_path/test.tcl

set usage "subdbscript testfile subdbnumber factor"

# Verify usage
if { $argc != 3 } {
	puts stderr "FAIL:[timestamp] Usage: $usage"
	exit
}

# Initialize arguments
set testfile [lindex $argv 0]
set n [ lindex $argv 1 ]
set factor [ lindex $argv 2 ]

set db [berkdb_open -unknown $testfile sub$n.db]
error_check_good db_open [is_valid_db $db] TRUE

set dbc [$db cursor]
error_check_good db_cursor [is_valid_cursor $dbc $db] TRUE
set i 1
for {set d [$dbc get -first]} {[llength $d] != 0} {set d [$dbc get -next]} {
	set x [expr $i - $n]
	if { $x >= 0 && [expr $x % $factor] == 0 } {
		puts  "Deleting $d"
		error_check_good dbc_del [$dbc del] 0
	}
	incr i
}
error_check_good db_close [$db close] 0

exit
