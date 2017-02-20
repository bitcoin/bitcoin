# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rsrc001
# TEST	Recno backing file test.  Try different patterns of adding
# TEST	records and making sure that the corresponding file matches.
proc rsrc001 { } {
	source ./include.tcl

	puts "Rsrc001: Basic recno backing file writeback tests"

	# We run this test essentially twice, once with a db file
	# and once without (an in-memory database).
	set rec1 "This is record 1"
	set rec2 "This is record 2 This is record 2"
	set rec3 "This is record 3 This is record 3 This is record 3"
	set rec4 [replicate "This is record 4 " 512]

	foreach testfile { "$testdir/rsrc001.db" "" } {

		cleanup $testdir NULL

		if { $testfile == "" } {
			puts "Rsrc001: Testing with in-memory database."
		} else {
			puts "Rsrc001: Testing with disk-backed database."
		}

		# Create backing file for the empty-file test.
		set oid1 [open $testdir/rsrc.txt w]
		fconfigure $oid1 -translation binary
		close $oid1

		puts "\tRsrc001.a: Put to empty file."
		set db [eval {berkdb_open -create -mode 0644\
		    -recno -source $testdir/rsrc.txt} $testfile]
		error_check_good dbopen [is_valid_db $db] TRUE
		set txn ""

		set ret [eval {$db put} $txn {1 $rec1}]
		error_check_good put_to_empty $ret 0
		error_check_good db_close [$db close] 0

		# Now fill out the backing file and create the check file.
		set oid1 [open $testdir/rsrc.txt a]
		set oid2 [open $testdir/check.txt w]
		fconfigure $oid1 -translation binary 
		fconfigure $oid2 -translation binary 

		# This one was already put into rsrc.txt.
		puts $oid2 $rec1

		# These weren't.
		puts $oid1 $rec2
		puts $oid2 $rec2
		puts $oid1 $rec3
		puts $oid2 $rec3
		puts $oid1 $rec4
		puts $oid2 $rec4
		close $oid1
		close $oid2

		puts -nonewline "\tRsrc001.b: Read file, rewrite last record;"
		puts " write it out and diff"
		set db [eval {berkdb_open -create -mode 0644\
		    -recno -source $testdir/rsrc.txt} $testfile]
		error_check_good dbopen [is_valid_db $db] TRUE

		# Read the last record; replace it (but we won't change it).
		# Then close the file and diff the two files.
		set dbc [eval {$db cursor} $txn]
		error_check_good db_cursor [is_valid_cursor $dbc $db] TRUE

		set rec [$dbc get -last]
		error_check_good get_last [llength [lindex $rec 0]] 2
		set key [lindex [lindex $rec 0] 0]
		set data [lindex [lindex $rec 0] 1]

		# Get the last record from the text file
		set oid [open $testdir/rsrc.txt]
		fconfigure $oid -translation binary 
		set laststr ""
		while { [gets $oid str] != -1 } {
			set laststr $str
		}
		close $oid
		error_check_good getlast $data $laststr

		set ret [eval {$db put} $txn {$key $data}]
		error_check_good replace_last $ret 0

		error_check_good curs_close [$dbc close] 0
		error_check_good db_sync [$db sync] 0
		error_check_good db_sync [$db sync] 0
		error_check_good \
		    Rsrc001:diff($testdir/rsrc.txt,$testdir/check.txt) \
		    [filecmp $testdir/rsrc.txt $testdir/check.txt] 0

		puts -nonewline "\tRsrc001.c: "
		puts "Append some records in tree and verify in file."
		set oid [open $testdir/check.txt a]
		fconfigure $oid -translation binary
		for {set i 1} {$i < 10} {incr i} {
			set rec [replicate "New Record $i" $i]
			puts $oid $rec
			incr key
			set ret [eval {$db put} $txn {-append $rec}]
			error_check_good put_append $ret $key
		}
		error_check_good db_sync [$db sync] 0
		error_check_good db_sync [$db sync] 0
		close $oid
		set ret [filecmp $testdir/rsrc.txt $testdir/check.txt]
		error_check_good \
		    Rsrc001:diff($testdir/{rsrc.txt,check.txt}) $ret 0

		puts "\tRsrc001.d: Append by record number"
		set oid [open $testdir/check.txt a]
		fconfigure $oid -translation binary
		for {set i 1} {$i < 10} {incr i} {
			set rec [replicate "New Record (set 2) $i" $i]
			puts $oid $rec
			incr key
			set ret [eval {$db put} $txn {$key $rec}]
			error_check_good put_byno $ret 0
		}

		error_check_good db_sync [$db sync] 0
		error_check_good db_sync [$db sync] 0
		close $oid
		set ret [filecmp $testdir/rsrc.txt $testdir/check.txt]
		error_check_good \
		    Rsrc001:diff($testdir/{rsrc.txt,check.txt}) $ret 0

		puts "\tRsrc001.e: Put beyond end of file."
		set oid [open $testdir/check.txt a]
		fconfigure $oid -translation binary
		for {set i 1} {$i < 10} {incr i} {
			puts $oid ""
			incr key
		}
		set rec "Last Record"
		puts $oid $rec
		incr key

		set ret [eval {$db put} $txn {$key $rec}]
		error_check_good put_byno $ret 0

		puts "\tRsrc001.f: Put beyond end of file, after reopen."

		error_check_good db_close [$db close] 0
		set db [eval {berkdb_open -create -mode 0644\
		    -recno -source $testdir/rsrc.txt} $testfile]
		error_check_good dbopen [is_valid_db $db] TRUE

		set rec "Last record with reopen"
		puts $oid $rec

		incr key
		set ret [eval {$db put} $txn {$key $rec}]
		error_check_good put_byno_with_reopen $ret 0

		puts "\tRsrc001.g:\
		    Put several beyond end of file, after reopen with snapshot."
		error_check_good db_close [$db close] 0
		set db [eval {berkdb_open -create -mode 0644\
		    -snapshot -recno -source $testdir/rsrc.txt} $testfile]
		error_check_good dbopen [is_valid_db $db] TRUE

		set rec "Really really last record with reopen"
		puts $oid ""
		puts $oid ""
		puts $oid ""
		puts $oid $rec

		incr key
		incr key
		incr key
		incr key

		set ret [eval {$db put} $txn {$key $rec}]
		error_check_good put_byno_with_reopen $ret 0

		error_check_good db_sync [$db sync] 0
		error_check_good db_sync [$db sync] 0

		close $oid
		set ret [filecmp $testdir/rsrc.txt $testdir/check.txt]
		error_check_good \
		    Rsrc001:diff($testdir/{rsrc.txt,check.txt}) $ret 0

		puts "\tRsrc001.h: Verify proper syncing of changes on close."
		error_check_good Rsrc001:db_close [$db close] 0
		set db [eval {berkdb_open -create -mode 0644 -recno \
		    -source $testdir/rsrc.txt} $testfile]
		set oid [open $testdir/check.txt a]
		fconfigure $oid -translation binary 
		for {set i 1} {$i < 10} {incr i} {
			set rec [replicate "New Record $i" $i]
			puts $oid $rec
			set ret [eval {$db put} $txn {-append $rec}]
			# Don't bother checking return;  we don't know what
			# the key number is, and we'll pick up a failure
			# when we compare.
		}
		error_check_good Rsrc001:db_close [$db close] 0
		close $oid
		set ret [filecmp $testdir/rsrc.txt $testdir/check.txt]
		error_check_good Rsrc001:diff($testdir/{rsrc,check}.txt) $ret 0
	}
}

