# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rsrc004
# TEST	Recno backing file test for EOF-terminated records.
proc rsrc004 { } {
	source ./include.tcl

	foreach isfixed { 0 1 } {
		cleanup $testdir NULL

		# Create the backing text file.
		set oid1 [open $testdir/rsrc.txt w]
		if { $isfixed == 1 } {
			puts -nonewline $oid1 "record 1xxx"
			puts -nonewline $oid1 "record 2xxx"
		} else {
			puts $oid1 "record 1xxx"
			puts $oid1 "record 2xxx"
		}
		puts -nonewline $oid1 "record 3"
		close $oid1

		set args "-create -mode 0644 -recno -source $testdir/rsrc.txt"
		if { $isfixed == 1 } {
			append args " -len [string length "record 1xxx"]"
			set match "record 3   "
			puts "Rsrc004: EOF-terminated recs: fixed length"
		} else {
			puts "Rsrc004: EOF-terminated recs: variable length"
			set match "record 3"
		}

		puts "\tRsrc004.a: Read file, verify correctness."
		set db [eval berkdb_open $args "$testdir/rsrc004.db"]
		error_check_good dbopen [is_valid_db $db] TRUE

		# Read the last record
		set dbc [eval {$db cursor} ""]
		error_check_good db_cursor [is_valid_cursor $dbc $db] TRUE

		set rec [$dbc get -last]
		error_check_good get_last $rec [list [list 3 $match]]

		error_check_good dbc_close [$dbc close] 0
		error_check_good db_close [$db close] 0
	}
}
