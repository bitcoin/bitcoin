# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test008
# TEST	Small keys/large data
# TEST		Put/get per key
# TEST		Loop through keys by steps (which change)
# TEST		    ... delete each key at step
# TEST		    ... add each key back
# TEST		    ... change step
# TEST		Confirm that overflow pages are getting reused
# TEST
# TEST	Take the source files and dbtest executable and enter their names as
# TEST	the key with their contents as data.  After all are entered, begin
# TEST	looping through the entries; deleting some pairs and then readding them.
proc test008 { method {reopen "008"} {debug 0} args} {
	source ./include.tcl

	set tnum test$reopen
	set args [convert_args $method $args]
	set omethod [convert_method $method]

	if { [is_record_based $method] == 1 } {
		puts "Test$reopen skipping for method $method"
		return
	}

	puts -nonewline "$tnum: $method filename=key filecontents=data pairs"
	if {$reopen == "009"} {
		puts "(with close)"
	} else {
		puts ""
	}

	# Create the database and open the dictionary
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/$tnum.db
		set env NULL
	} else {
		set testfile $tnum.db
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set testdir [get_home $env]
	}
	set t1 $testdir/t1
	set t2 $testdir/t2
	set t3 $testdir/t3
	set t4 $testdir/t4

	cleanup $testdir $env

	set db [eval {berkdb_open -create -mode 0644} \
	    $args {$omethod $testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE

	set pflags ""
	set gflags ""
	set txn ""

	# Here is the loop where we put and get each key/data pair
	set file_list [get_file_list]

	set count 0
	puts "\tTest$reopen.a: Initial put/get loop"
	foreach f $file_list {
		set names($count) $f
		set key $f

		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		put_file $db $txn $pflags $f
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}

		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		get_file $db $txn $gflags $f $t4
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}

		error_check_good Test$reopen:diff($f,$t4) \
		    [filecmp $f $t4] 0

		incr count
	}

	if {$reopen == "009"} {
		error_check_good db_close [$db close] 0

		set db [eval {berkdb_open} $args $testfile]
		error_check_good dbopen [is_valid_db $db] TRUE
	}

	# Now we will get step through keys again (by increments) and
	# delete all the entries, then re-insert them.

	puts "\tTest$reopen.b: Delete re-add loop"
	foreach i "1 2 4 8 16" {
		for {set ndx 0} {$ndx < $count} { incr ndx $i} {
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			set r [eval {$db del} $txn {$names($ndx)}]
			error_check_good db_del:$names($ndx) $r 0
			if { $txnenv == 1 } {
				error_check_good txn [$t commit] 0
			}
		}
		for {set ndx 0} {$ndx < $count} { incr ndx $i} {
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			put_file $db $txn $pflags $names($ndx)
			if { $txnenv == 1 } {
				error_check_good txn [$t commit] 0
			}
		}
	}

	if {$reopen == "009"} {
		error_check_good db_close [$db close] 0
		set db [eval {berkdb_open} $args $testfile]
		error_check_good dbopen [is_valid_db $db] TRUE
	}

	# Now, reopen the file and make sure the key/data pairs look right.
	puts "\tTest$reopen.c: Dump contents forward"
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	dump_bin_file $db $txn $t1 test008.check
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}

	set oid [open $t2.tmp w]
	foreach f $file_list {
		puts $oid $f
	}
	close $oid
	filesort $t2.tmp $t2
	fileremove $t2.tmp
	filesort $t1 $t3

	error_check_good Test$reopen:diff($t3,$t2) \
	    [filecmp $t3 $t2] 0

	# Now, reopen the file and run the last test again in reverse direction.
	puts "\tTest$reopen.d: Dump contents backward"
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	dump_bin_file_direction $db $txn $t1 test008.check "-last" "-prev"
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}

	filesort $t1 $t3

	error_check_good Test$reopen:diff($t3,$t2) \
	    [filecmp $t3 $t2] 0
	error_check_good close:$db [$db close] 0
}

proc test008.check { binfile tmpfile } {
	global tnum
	source ./include.tcl

	error_check_good diff($binfile,$tmpfile) \
	    [filecmp $binfile $tmpfile] 0
}
