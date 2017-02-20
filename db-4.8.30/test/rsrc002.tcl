# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rsrc002
# TEST	Recno backing file test #2: test of set_re_delim.  Specify a backing
# TEST	file with colon-delimited records, and make sure they are correctly
# TEST	interpreted.
proc rsrc002 { } {
	source ./include.tcl

	puts "Rsrc002: Alternate variable-length record delimiters."

	# We run this test essentially twice, once with a db file
	# and once without (an in-memory database).
	foreach testfile { "$testdir/rsrc002.db" "" } {

		cleanup $testdir NULL

		# Create the starting files
		set oid1 [open $testdir/rsrc.txt w]
		set oid2 [open $testdir/check.txt w]
		puts -nonewline $oid1 "ostrich:emu:kiwi:moa:cassowary:rhea:"
		puts -nonewline $oid2 "ostrich:emu:kiwi:penguin:cassowary:rhea:"
		close $oid1
		close $oid2

		if { $testfile == "" } {
			puts "Rsrc002: Testing with in-memory database."
		} else {
			puts "Rsrc002: Testing with disk-backed database."
		}

		puts "\tRsrc002.a: Read file, verify correctness."
		set db [eval {berkdb_open -create -mode 0644 -delim 58 \
		    -recno -source $testdir/rsrc.txt} $testfile]
		error_check_good dbopen [is_valid_db $db] TRUE

		# Read the last record; replace it (but we won't change it).
		# Then close the file and diff the two files.
		set txn ""
		set dbc [eval {$db cursor} $txn]
		error_check_good db_cursor [is_valid_cursor $dbc $db] TRUE

		set rec [$dbc get -first]
		error_check_good get_first $rec [list [list 1 "ostrich"]]
		set rec [$dbc get -next]
		error_check_good get_next $rec [list [list 2 "emu"]]

		puts "\tRsrc002.b: Write record, verify correctness."

		eval {$dbc get -set 4}
		set ret [$dbc put -current "penguin"]
		error_check_good dbc_put $ret 0

		error_check_good dbc_close [$dbc close] 0
		error_check_good db_close [$db close] 0

		error_check_good \
		    Rsrc002:diff($testdir/rsrc.txt,$testdir/check.txt) \
		    [filecmp $testdir/rsrc.txt $testdir/check.txt] 0
	}
}
