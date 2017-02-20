# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	sdbtest001
# TEST	Tests multiple access methods in one subdb
# TEST		Open several subdbs, each with a different access method
# TEST		Small keys, small data
# TEST		Put/get per key per subdb
# TEST		Dump file, verify per subdb
# TEST		Close, reopen per subdb
# TEST		Dump file, verify per subdb
# TEST
# TEST	Make several subdb's of different access methods all in one DB.
# TEST	Rotate methods and repeat [#762].
# TEST	Use the first 10,000 entries from the dictionary.
# TEST	Insert each with self as key and data; retrieve each.
# TEST	After all are entered, retrieve all; compare output to original.
# TEST	Close file, reopen, do retrieve and re-verify.
proc sdbtest001 { {nentries 10000} } {
	source ./include.tcl

	puts "Subdbtest001: many different subdb access methods in one"

	# Create the database and open the dictionary
	set testfile $testdir/subdbtest001.db
	set t1 $testdir/t1
	set t2 $testdir/t2
	set t3 $testdir/t3
	set t4 $testdir/t4

	set txn ""
	set count 0

	# Set up various methods to rotate through
	lappend method_list [list "-rrecno" "-rbtree" "-hash" "-recno" "-btree"]
	lappend method_list [list "-recno" "-hash" "-btree" "-rbtree" "-rrecno"]
	lappend method_list [list "-btree" "-recno" "-rbtree" "-rrecno" "-hash"]
	lappend method_list [list "-hash" "-recno" "-rbtree" "-rrecno" "-btree"]
	lappend method_list [list "-rbtree" "-hash" "-btree" "-rrecno" "-recno"]
	lappend method_list [list "-rrecno" "-recno"]
	lappend method_list [list "-recno" "-rrecno"]
	lappend method_list [list "-hash" "-dhash"]
	lappend method_list [list "-dhash" "-hash"]
	lappend method_list [list "-rbtree" "-btree" "-dbtree" "-ddbtree"]
	lappend method_list [list "-btree" "-rbtree" "-ddbtree" "-dbtree"]
	lappend method_list [list "-dbtree" "-ddbtree" "-btree" "-rbtree"]
	lappend method_list [list "-ddbtree" "-dbtree" "-rbtree" "-btree"]
	set plist [list 512 8192 1024 4096 2048 16384]
	set mlen [llength $method_list]
	set plen [llength $plist]
	while { $plen < $mlen } {
		set plist [concat $plist $plist]
		set plen [llength $plist]
	}
	set pgsz 0
	foreach methods $method_list {
		cleanup $testdir NULL
		puts "\tSubdbtest001.a: create subdbs of different access methods:"
		puts "\tSubdbtest001.a: $methods"
		set nsubdbs [llength $methods]
		set duplist ""
		for { set i 0 } { $i < $nsubdbs } { incr i } {
			lappend duplist -1
		}
		set psize [lindex $plist $pgsz]
		incr pgsz
		set newent [expr $nentries / $nsubdbs]
		build_all_subdb $testfile $methods $psize $duplist $newent

		# Now we will get each key from the DB and compare the results
		# to the original.
		for { set subdb 0 } { $subdb < $nsubdbs } { incr subdb } {

			set method [lindex $methods $subdb]
			set method [convert_method $method]
			if { [is_record_based $method] == 1 } {
				set checkfunc subdbtest001_recno.check
			} else {
				set checkfunc subdbtest001.check
			}

			puts "\tSubdbtest001.b: dump file sub$subdb.db"
			set db [berkdb_open -unknown $testfile sub$subdb.db]
			dump_file $db $txn $t1 $checkfunc
			error_check_good db_close [$db close] 0

			# Now compare the keys to see if they match the
			# dictionary (or ints)
			if { [is_record_based $method] == 1 } {
				set oid [open $t2 w]
				for {set i 1} {$i <= $newent} {incr i} {
					puts $oid [expr $subdb * $newent + $i]
				}
				close $oid
				file rename -force $t1 $t3
			} else {
				# filehead uses 1-based line numbers
				set beg [expr $subdb * $newent]
				incr beg
				set end [expr $beg + $newent - 1]
				filehead $end $dict $t3 $beg
				filesort $t3 $t2
				filesort $t1 $t3
			}

			error_check_good Subdbtest001:diff($t3,$t2) \
			    [filecmp $t3 $t2] 0

			puts "\tSubdbtest001.c: sub$subdb.db: close, open, and dump file"
			# Now, reopen the file and run the last test again.
			open_and_dump_subfile $testfile NULL $t1 $checkfunc \
			    dump_file_direction "-first" "-next" sub$subdb.db
			if { [string compare $method "-recno"] != 0 } {
				filesort $t1 $t3
			}

			error_check_good Subdbtest001:diff($t2,$t3) \
			    [filecmp $t2 $t3] 0

			# Now, reopen the file and run the last test again in the
			# reverse direction.
			puts "\tSubdbtest001.d: sub$subdb.db: close, open, and dump file in reverse direction"
			open_and_dump_subfile $testfile NULL $t1 $checkfunc \
			    dump_file_direction "-last" "-prev" sub$subdb.db

			if { [string compare $method "-recno"] != 0 } {
				filesort $t1 $t3
			}

			error_check_good Subdbtest001:diff($t3,$t2) \
			    [filecmp $t3 $t2] 0
		}
	}
}

# Check function for Subdbtest001; keys and data are identical
proc subdbtest001.check { key data } {
	error_check_good "key/data mismatch" $data $key
}

proc subdbtest001_recno.check { key data } {
global dict
global kvals
	error_check_good key"$key"_exists [info exists kvals($key)] 1
	error_check_good "key/data mismatch, key $key" $data $kvals($key)
}
