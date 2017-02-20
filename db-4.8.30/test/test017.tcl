# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test017
# TEST	Basic offpage duplicate test.
# TEST
# TEST	Run duplicates with small page size so that we test off page duplicates.
# TEST	Then after we have an off-page database, test with overflow pages too.
proc test017 { method {contents 0} {ndups 19} {tnum "017"} args } {
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	# Btree with compression does not support unsorted duplicates.
	if { [is_compressed $args] == 1 } {
		puts "Test$tnum skipping for btree with compression."
		return
	}

	if { [is_record_based $method] == 1 || [is_rbtree $method] == 1 } {
		puts "Test$tnum skipping for method $method"
		return
	}
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		incr pgindex
		if { [lindex $args $pgindex] > 8192 } {
			puts "Test$tnum: Skipping for large pagesizes"
			return
		}
	}

	# Create the database and open the dictionary
	set limit 0
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test$tnum.db
		set env NULL
	} else {
		set testfile test$tnum.db
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
			set limit 100
		}
		set testdir [get_home $env]
	}
	set t1 $testdir/t1
	set t2 $testdir/t2
	set t3 $testdir/t3
	set t4 $testdir/t4

	cleanup $testdir $env

	set db [eval {berkdb_open \
	     -create -mode 0644 -dup} $args {$omethod $testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE

	set pflags ""
	set gflags ""
	set txn ""
	set count 0

	set file_list [get_file_list 1]
	if { $txnenv == 1 } {
		if { [llength $file_list] > $limit } {
			set file_list [lrange $file_list 0 $limit]
		}
		set flen [llength $file_list]
		reduce_dups flen ndups
	}
	puts "Test$tnum: $method ($args) Off page duplicate tests\
	    with $ndups duplicates"

	set ovfl ""
	# Here is the loop where we put and get each key/data pair
	puts -nonewline "\tTest$tnum.a: Creating duplicates with "
	if { $contents != 0 } {
		puts "file contents as key/data"
	} else {
		puts "file name as key/data"
	}
	foreach f $file_list {
		if { $contents != 0 } {
			set fid [open $f r]
			fconfigure $fid -translation binary
			#
			# Prepend file name to guarantee uniqueness
			set filecont [read $fid]
			set str $f:$filecont
			close $fid
		} else {
			set str $f
		}
		for { set i 1 } { $i <= $ndups } { incr i } {
			set datastr $i:$str
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			set ret [eval {$db put} \
			    $txn $pflags {$str [chop_data $method $datastr]}]
			error_check_good put $ret 0
			if { $txnenv == 1 } {
				error_check_good txn [$t commit] 0
			}
		}

		#
		# Save 10% files for overflow test
		#
		if { $contents == 0 && [expr $count % 10] == 0 } {
			lappend ovfl $f
		}
		# Now retrieve all the keys matching this key
		set ret [$db get $str]
		error_check_bad $f:dbget_dups [llength $ret] 0
		error_check_good $f:dbget_dups1 [llength $ret] $ndups
		set x 1
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set dbc [eval {$db cursor} $txn]
		for {set ret [$dbc get "-set" $str]} \
		    {[llength $ret] != 0} \
		    {set ret [$dbc get "-next"] } {
			set k [lindex [lindex $ret 0] 0]
			if { [string compare $k $str] != 0 } {
				break
			}
			set datastr [lindex [lindex $ret 0] 1]
			set d [data_of $datastr]
			if {[string length $d] == 0} {
				break
			}
			error_check_good "Test$tnum:get" $d $str
			set id [ id_of $datastr ]
			error_check_good "Test$tnum:$f:dup#" $id $x
			incr x
		}
		error_check_good "Test$tnum:ndups:$str" [expr $x - 1] $ndups
		error_check_good cursor_close [$dbc close] 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
		incr count
	}

	# Now we will get each key from the DB and compare the results
	# to the original.
	puts "\tTest$tnum.b: Checking file for correct duplicates"
	set dlist ""
	for { set i 1 } { $i <= $ndups } {incr i} {
		lappend dlist $i
	}
	set oid [open $t2.tmp w]
	set o1id [open $t4.tmp w]
	foreach f $file_list {
		for {set i 1} {$i <= $ndups} {incr i} {
			puts $o1id $f
		}
		puts $oid $f
	}
	close $oid
	close $o1id
	filesort $t2.tmp $t2
	filesort $t4.tmp $t4
	fileremove $t2.tmp
	fileremove $t4.tmp

	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	dup_check $db $txn $t1 $dlist
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	if {$contents == 0} {
		filesort $t1 $t3

		error_check_good Test$tnum:diff($t3,$t2) [filecmp $t3 $t2] 0

		# Now compare the keys to see if they match the file names
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		dump_file $db $txn $t1 test017.check
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
		filesort $t1 $t3

		error_check_good Test$tnum:diff($t3,$t4) [filecmp $t3 $t4] 0
	}

	error_check_good db_close [$db close] 0
	set db [eval {berkdb_open} $args $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	puts "\tTest$tnum.c: Checking file for correct duplicates after close"
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	dup_check $db $txn $t1 $dlist
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}

	if {$contents == 0} {
		# Now compare the keys to see if they match the filenames
		filesort $t1 $t3
		error_check_good Test$tnum:diff($t3,$t2) [filecmp $t3 $t2] 0
	}
	error_check_good db_close [$db close] 0

	puts "\tTest$tnum.d: Verify off page duplicates and overflow status"
	set db [eval {berkdb_open} $args $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE
	set stat [$db stat]
	if { [is_btree $method] } {
		error_check_bad stat:offpage \
		    [is_substr $stat "{{Internal pages} 0}"] 1
	}
	if {$contents == 0} {
		# This check doesn't work in hash, since overflow
		# pages count extra pages in buckets as well as true
		# P_OVERFLOW pages.
		if { [is_hash $method] == 0 } {
			error_check_good overflow \
			    [is_substr $stat "{{Overflow pages} 0}"] 1
		}
	} else {
		if { [is_hash $method] } {
			error_check_bad overflow \
			    [is_substr $stat "{{Number of big pages} 0}"] 1
		} else {
			error_check_bad overflow \
			    [is_substr $stat "{{Overflow pages} 0}"] 1
		}
	}

	#
	# If doing overflow test, do that now.  Else we are done.
	# Add overflow pages by adding a large entry to a duplicate.
	#
	if { [llength $ovfl] == 0} {
		error_check_good db_close [$db close] 0
		return
	}

	puts "\tTest$tnum.e: Add overflow duplicate entries"
	set ovfldup [expr $ndups + 1]
	foreach f $ovfl {
		#
		# This is just like put_file, but prepends the dup number
		#
		set fid [open $f r]
		fconfigure $fid -translation binary
		set fdata [read $fid]
		close $fid
		set data $ovfldup:$fdata:$fdata:$fdata:$fdata

		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval {$db put} $txn $pflags {$f $data}]
		error_check_good ovfl_put $ret 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
	}

	puts "\tTest$tnum.f: Verify overflow duplicate entries"
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	dup_check $db $txn $t1 $dlist $ovfldup
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	filesort $t1 $t3
	error_check_good Test$tnum:diff($t3,$t2) [filecmp $t3 $t2] 0

	set stat [$db stat]
	if { [is_hash [$db get_type]] } {
		error_check_bad overflow1_hash [is_substr $stat \
		    "{{Number of big pages} 0}"] 1
	} else {
		error_check_bad \
		    overflow1 [is_substr $stat "{{Overflow pages} 0}"] 1
	}
	error_check_good db_close [$db close] 0
}

# Check function; verify data contains key
proc test017.check { key data } {
	error_check_good "data mismatch for key $key" $key [data_of $data]
}
