# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	sdbtest002
# TEST	Tests multiple access methods in one subdb access by multiple
# TEST	processes.
# TEST		Open several subdbs, each with a different access method
# TEST		Small keys, small data
# TEST		Put/get per key per subdb
# TEST		Fork off several child procs to each delete selected
# TEST		    data from their subdb and then exit
# TEST		Dump file, verify contents of each subdb is correct
# TEST		Close, reopen per subdb
# TEST		Dump file, verify per subdb
# TEST
# TEST	Make several subdb's of different access methods all in one DB.
# TEST	Fork of some child procs to each manipulate one subdb and when
# TEST	they are finished, verify the contents of the databases.
# TEST	Use the first 10,000 entries from the dictionary.
# TEST	Insert each with self as key and data; retrieve each.
# TEST	After all are entered, retrieve all; compare output to original.
# TEST	Close file, reopen, do retrieve and re-verify.
proc sdbtest002 { {nentries 10000} } {
	source ./include.tcl

	puts "Subdbtest002: many different subdb access methods in one"

	# Create the database and open the dictionary
	set testfile $testdir/subdbtest002.db
	set t1 $testdir/t1
	set t2 $testdir/t2
	set t3 $testdir/t3
	set t4 $testdir/t4

	set txn ""
	set count 0

	# Set up various methods to rotate through
	set methods \
	    [list "-rbtree" "-recno" "-btree" "-btree" "-recno" "-rbtree"]
	cleanup $testdir NULL
	puts "\tSubdbtest002.a: create subdbs of different methods: $methods"
	set psize 4096
	set nsubdbs [llength $methods]
	set duplist ""
	for { set i 0 } { $i < $nsubdbs } { incr i } {
		lappend duplist -1
	}
	set newent [expr $nentries / $nsubdbs]

	#
	# XXX We need dict sorted to figure out what was deleted
	# since things are stored sorted in the btree.
	#
	filesort $dict $t4
	set dictorig $dict
	set dict $t4

	build_all_subdb $testfile $methods $psize $duplist $newent

	# Now we will get each key from the DB and compare the results
	# to the original.
	set pidlist ""
	puts "\tSubdbtest002.b: create $nsubdbs procs to delete some keys"
	for { set subdb 0 } { $subdb < $nsubdbs } { incr subdb } {
		puts "$tclsh_path\
		    $test_path/sdbscript.tcl $testfile \
		    $subdb $nsubdbs >& $testdir/subdb002.log.$subdb"
		set p [exec $tclsh_path $test_path/wrap.tcl \
		    sdbscript.tcl \
		    $testdir/subdb002.log.$subdb $testfile $subdb $nsubdbs &]
		lappend pidlist $p
	}
	watch_procs $pidlist 5

	for { set subdb 0 } { $subdb < $nsubdbs } { incr subdb } {
		set method [lindex $methods $subdb]
		set method [convert_method $method]
		if { [is_record_based $method] == 1 } {
			set checkfunc subdbtest002_recno.check
		} else {
			set checkfunc subdbtest002.check
		}

		puts "\tSubdbtest002.b: dump file sub$subdb.db"
		set db [berkdb_open -unknown $testfile sub$subdb.db]
		error_check_good db_open [is_valid_db $db] TRUE
		dump_file $db $txn $t1 $checkfunc
		error_check_good db_close [$db close] 0

		# Now compare the keys to see if they match the dictionary (or ints)
		if { [is_record_based $method] == 1 } {
			set oid [open $t2 w]
			for {set i 1} {$i <= $newent} {incr i} {
				set x [expr $i - $subdb]
				if { [expr $x % $nsubdbs] != 0 } {
					puts $oid [expr $subdb * $newent + $i]
				}
			}
			close $oid
			file rename -force $t1 $t3
		} else {
			set oid [open $t4 r]
			for {set i 1} {[gets $oid line] >= 0} {incr i} {
				set farr($i) $line
			}
			close $oid

			set oid [open $t2 w]
			for {set i 1} {$i <= $newent} {incr i} {
				# Sed uses 1-based line numbers
				set x [expr $i - $subdb]
				if { [expr $x % $nsubdbs] != 0 } {
					set beg [expr $subdb * $newent]
					set beg [expr $beg + $i]
					puts $oid $farr($beg)
				}
			}
			close $oid
			filesort $t1 $t3
		}

		error_check_good Subdbtest002:diff($t3,$t2) \
		    [filecmp $t3 $t2] 0

		puts "\tSubdbtest002.c: sub$subdb.db: close, open, and dump file"
		# Now, reopen the file and run the last test again.
		open_and_dump_subfile $testfile NULL $t1 $checkfunc \
		    dump_file_direction "-first" "-next" sub$subdb.db
		if { [string compare $method "-recno"] != 0 } {
			filesort $t1 $t3
		}

		error_check_good Subdbtest002:diff($t2,$t3) \
		    [filecmp $t2 $t3] 0

		# Now, reopen the file and run the last test again in the
		# reverse direction.
		puts "\tSubdbtest002.d: sub$subdb.db: close, open, and dump file in reverse direction"
		open_and_dump_subfile $testfile NULL $t1 $checkfunc \
		    dump_file_direction "-last" "-prev" sub$subdb.db

		if { [string compare $method "-recno"] != 0 } {
			filesort $t1 $t3
		}

		error_check_good Subdbtest002:diff($t3,$t2) \
		    [filecmp $t3 $t2] 0
	}
	set dict $dictorig
	return
}

# Check function for Subdbtest002; keys and data are identical
proc subdbtest002.check { key data } {
	error_check_good "key/data mismatch" $data $key
}

proc subdbtest002_recno.check { key data } {
global dict
global kvals
	error_check_good key"$key"_exists [info exists kvals($key)] 1
	error_check_good "key/data mismatch, key $key" $data $kvals($key)
}
