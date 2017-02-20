# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rsrc003
# TEST	Recno backing file test.  Try different patterns of adding
# TEST	records and making sure that the corresponding file matches.
proc rsrc003 { } {
	source ./include.tcl
	global fixed_len

	puts "Rsrc003: Basic recno backing file writeback tests fixed length"

	# We run this test essentially twice, once with a db file
	# and once without (an in-memory database).
	#
	# Then run with big fixed-length records
	set rec1 "This is record 1"
	set rec2 "This is record 2"
	set rec3 "This is record 3"
	set bigrec1 [replicate "This is record 1 " 512]
	set bigrec2 [replicate "This is record 2 " 512]
	set bigrec3 [replicate "This is record 3 " 512]

	set orig_fixed_len $fixed_len
	set rlist {
	{{$rec1 $rec2 $rec3} "small records" }
	{{$bigrec1 $bigrec2 $bigrec3} "large records" }}

	foreach testfile { "$testdir/rsrc003.db" "" } {

		foreach rec $rlist {
			cleanup $testdir NULL

			set recs [lindex $rec 0]
			set msg [lindex $rec 1]

			# Create the starting files
			# Note that for the rest of the test, we are going
			# to append a LF when we 'put' via DB to maintain
			# file structure and allow us to use 'gets'.
			set oid1 [open $testdir/rsrc.txt w]
			set oid2 [open $testdir/check.txt w]
			fconfigure $oid1 -translation binary
			fconfigure $oid2 -translation binary
			foreach record $recs {
				set r [subst $record]
				set fixed_len [string length $r]
				puts $oid1 $r
				puts $oid2 $r
			}
			close $oid1
			close $oid2

			set reclen [expr $fixed_len + 1]
			if { $reclen > [string length $rec1] } {
				set repl 512
			} else {
				set repl 2
			}
			if { $testfile == "" } {
				puts \
"Rsrc003: Testing with in-memory database with $msg."
			} else {
				puts \
"Rsrc003: Testing with disk-backed database with $msg."
			}

			puts -nonewline \
			    "\tRsrc003.a: Read file, rewrite last record;"
			puts " write it out and diff"
			set db [eval {berkdb_open -create -mode 0644 -recno \
			    -len $reclen -source $testdir/rsrc.txt} $testfile]
			error_check_good dbopen [is_valid_db $db] TRUE

			# Read the last record; replace it (don't change it).
			# Then close the file and diff the two files.
			set txn ""
			set dbc [eval {$db cursor} $txn]
			error_check_good db_cursor \
			    [is_valid_cursor $dbc $db] TRUE

			set rec [$dbc get -last]
			error_check_good get_last [llength [lindex $rec 0]] 2
			set key [lindex [lindex $rec 0] 0]
			set data [lindex [lindex $rec 0] 1]

			# Get the last record from the text file
			set oid [open $testdir/rsrc.txt]
			fconfigure $oid -translation binary
			set laststr ""
			while { [gets $oid str] != -1 } {
				append str \12
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
			    diff1($testdir/rsrc.txt,$testdir/check.txt) \
			    [filecmp $testdir/rsrc.txt $testdir/check.txt] 0

			puts -nonewline "\tRsrc003.b: "
			puts "Append some records in tree and verify in file."
			set oid [open $testdir/check.txt a]
			fconfigure $oid -translation binary
			for {set i 1} {$i < 10} {incr i} {
				set rec [chop_data -frecno [replicate \
				    "This is New Record $i" $repl]]
				puts $oid $rec
				append rec \12
				incr key
				set ret [eval {$db put} $txn {-append $rec}]
				error_check_good put_append $ret $key
			}
			error_check_good db_sync [$db sync] 0
			error_check_good db_sync [$db sync] 0
			close $oid
			set ret [filecmp $testdir/rsrc.txt $testdir/check.txt]
			error_check_good \
			    diff2($testdir/{rsrc.txt,check.txt}) $ret 0

			puts "\tRsrc003.c: Append by record number"
			set oid [open $testdir/check.txt a]
			fconfigure $oid -translation binary
			for {set i 1} {$i < 10} {incr i} {
				set rec [chop_data -frecno [replicate \
				    "New Record (set 2) $i" $repl]]
				puts $oid $rec
				append rec \12
				incr key
				set ret [eval {$db put} $txn {$key $rec}]
				error_check_good put_byno $ret 0
			}

			error_check_good db_sync [$db sync] 0
			error_check_good db_sync [$db sync] 0
			close $oid
			set ret [filecmp $testdir/rsrc.txt $testdir/check.txt]
			error_check_good \
			    diff3($testdir/{rsrc.txt,check.txt}) $ret 0

			puts \
"\tRsrc003.d: Verify proper syncing of changes on close."
			error_check_good Rsrc003:db_close [$db close] 0
			set db [eval {berkdb_open -create -mode 0644 -recno \
			    -len $reclen -source $testdir/rsrc.txt} $testfile]
			set oid [open $testdir/check.txt a]
			fconfigure $oid -translation binary
			for {set i 1} {$i < 10} {incr i} {
				set rec [chop_data -frecno [replicate \
				    "New Record (set 3) $i" $repl]]
				puts $oid $rec
				append rec \12
				set ret [eval {$db put} $txn {-append $rec}]
				# Don't bother checking return;
				# we don't know what
				# the key number is, and we'll pick up a failure
				# when we compare.
			}
			error_check_good Rsrc003:db_close [$db close] 0
			close $oid
			set ret [filecmp $testdir/rsrc.txt $testdir/check.txt]
			error_check_good \
			    diff5($testdir/{rsrc,check}.txt) $ret 0
		}
	}
	set fixed_len $orig_fixed_len
	return
}
