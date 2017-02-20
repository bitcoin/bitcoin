# See the file LICENSE for redistribution information.
#
# Copyright (c) 2005-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test115
# TEST	Test database compaction with user-specified btree sort.
# TEST
# TEST	This is essentially test111 with the user-specified sort.
# TEST	Populate a database.  Remove a high proportion of entries.
# TEST	Dump and save contents.  Compact the database, dump again,
# TEST	and make sure we still have the same contents.
# TEST	Add back some entries, delete more entries (this time by
# TEST	cursor), dump, compact, and do the before/after check again.

proc test115 { method {nentries 10000} {tnum "115"} args } {
	source ./include.tcl
	global btvals
	global btvalsck
	global encrypt
	global passwd

	if { [is_btree $method] != 1 } {
		puts "Skipping test$tnum for method $method."
		return
	}

	# If a page size was specified, find out what it is.  Pages
	# might not be freed in the case of really large pages (64K)
	# but we still want to run this test just to make sure
	# nothing funny happens.
	set pagesize 0
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		incr pgindex
		set pagesize [lindex $args $pgindex]
	}

	set args [convert_args $method $args]
	set encargs ""
	set args [split_encargs $args encargs]
	set omethod [convert_method $method]

	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	if { $eindex == -1 } {
		set basename $testdir/test$tnum
		set env NULL
		set envargs ""
	} else {
		set basename test$tnum
		incr eindex
		set env [lindex $args $eindex]
		set envargs " -env $env "
		set rpcenv [is_rpcenv $env]
		if { $rpcenv == 1 } {
			puts "Test$tnum: skipping for RPC."
			return
		}
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set testdir [get_home $env]
	}

	puts "Test$tnum:\
	    ($method $args $encargs) Database compaction with user-specified sort."

	cleanup $testdir $env
	set t1 $testdir/t1
	set t2 $testdir/t2
	set splitopts { "" "-revsplitoff" }
	set txn ""

	set checkfunc test093_check

	foreach splitopt $splitopts {
		set testfile $basename.db
		if { $splitopt == "-revsplitoff" } {
			set testfile $basename.rev.db
		}
		set did [open $dict]

		puts "\tTest$tnum.a: Create and populate database ($splitopt)."
		set db [eval {berkdb_open -create -btcompare test093_cmp1 \
		    -mode 0644} $splitopt $args $encargs $omethod $testfile]
		error_check_good dbopen [is_valid_db $db] TRUE

		set count 0
		set btvals {}
		set btvalsck {}
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		while { [gets $did str] != -1 && $count < $nentries } {
			set key $str
			set str [reverse $str]

			set ret [eval \
			    {$db put} $txn {$key [chop_data $method $str]}]
			error_check_good put $ret 0
			lappend btvals $key
			incr count

		}
		if { $txnenv == 1 } {
			error_check_good txn_commit [$t commit] 0
		}
		close $did
		error_check_good db_sync [$db sync] 0

		if { $env != "NULL" } {
			set testdir [get_home $env]
			set filename $testdir/$testfile
		} else {
			set filename $testfile
		}
		set size1 [file size $filename]
		set free1 [stat_field $db stat "Pages on freelist"]
		set leaf1 [stat_field $db stat "Leaf pages"]
		set internal1 [stat_field $db stat "Internal pages"]

		puts "\tTest$tnum.b: Delete most entries from database."
		set did [open $dict]
		set count [expr $nentries - 1]
		set n 14

		# Leave every nth item.  Since rrecno renumbers, we
		# delete starting at nentries and working down to 0.
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		while { [gets $did str] != -1 && $count > 0 } {
			set key $str

			if { [expr $count % $n] != 0 } {
				set ret [eval {$db del} $txn {$key}]
				error_check_good del $ret 0
			}
			incr count -1
		}
		if { $txnenv == 1 } {
			error_check_good t_commit [$t commit] 0
		}
		close $did
		error_check_good db_sync [$db sync] 0

		puts "\tTest$tnum.c: Do a dump_file on contents."
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}

		dump_file $db $txn $t1 $checkfunc
		if { $txnenv == 1 } {
			error_check_good txn_commit [$t commit] 0
		}

		puts "\tTest$tnum.d: Compact and verify database."
		for {set commit 0} {$commit <= $txnenv} {incr commit} {
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			set ret [eval $db compact $txn -freespace]
			if { $txnenv == 1 } {
				if { $commit == 0 } {
					puts "\tTest$tnum.d: Aborting."
					error_check_good txn_abort [$t abort] 0
				} else {
					puts "\tTest$tnum.d: Committing."
					error_check_good txn_commit [$t commit] 0
				}
			}
			error_check_good db_sync [$db sync] 0
			if { [catch {eval \
			    {berkdb dbverify -btcompare test093_cmp1}\
			    $envargs $encargs {$testfile}} res] } {
				puts "FAIL: Verification failed with $res"
			}

		}

		set size2 [file size $filename]
		set free2 [stat_field $db stat "Pages on freelist"]
		set leaf2 [stat_field $db stat "Leaf pages"]
		set internal2 [stat_field $db stat "Internal pages"]

		# The sum of internal pages, leaf pages, and pages freed
		# should decrease on compaction, indicating that pages
		# have been freed to the file system.
		set sum1 [expr $free1 + $leaf1 + $internal1]
		set sum2 [expr $free2 + $leaf2 + $internal2]
		error_check_good pages_freed [expr $sum1 > $sum2] 1

		# Check for reduction in file size.
#### We should look at the partitioned files #####
if { [is_partitioned $args] == 0 } {
		set reduction .95
		error_check_good \
		    file_size [expr [expr $size1 * $reduction] > $size2] 1
}
		puts "\tTest$tnum.e: Contents are the same after compaction."
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		dump_file $db $txn $t2 $checkfunc
		if { $txnenv == 1 } {
			error_check_good txn_commit [$t commit] 0
		}

		error_check_good filecmp [filecmp $t1 $t2] 0

		puts "\tTest$tnum.f: Add more entries to database."
		set did [open $dict]
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		while { [gets $did str] != -1 && $count < $nentries } {
			set key $str
			set str [reverse $str]

			set ret [eval \
			    {$db put} $txn {$key [chop_data $method $str]}]
			error_check_good put $ret 0
			lappend btvals $key
			incr count
		}
		if { $txnenv == 1 } {
			error_check_good txn_commit [$t commit] 0
		}
		close $did
		error_check_good db_sync [$db sync] 0

		set size3 [file size $filename]
		set free3 [stat_field $db stat "Pages on freelist"]
		set leaf3 [stat_field $db stat "Leaf pages"]
		set internal3 [stat_field $db stat "Internal pages"]

		puts "\tTest$tnum.g: Remove more entries, this time by cursor."
		set count 0
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set dbc [eval {$db cursor} $txn]

		# Leave every nth item.
		for { set dbt [$dbc get -first] } { [llength $dbt] > 0 }\
		    { set dbt [$dbc get -next] ; incr count } {
			if { [expr $count % $n] != 0 } {
				error_check_good dbc_del [$dbc del] 0
			}
		}

		error_check_good cursor_close [$dbc close] 0
		if { $txnenv == 1 } {
			error_check_good t_commit [$t commit] 0
		}
		error_check_good db_sync [$db sync] 0

		puts "\tTest$tnum.h: Save contents."
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		dump_file $db $txn $t1 $checkfunc
		if { $txnenv == 1 } {
			error_check_good t_commit [$t commit] 0
		}

		puts "\tTest$tnum.i: Compact and verify database again."
		for {set commit 0} {$commit <= $txnenv} {incr commit} {
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			set ret [eval $db compact $txn -freespace]
			if { $txnenv == 1 } {
				if { $commit == 0 } {
					puts "\tTest$tnum.d: Aborting."
					error_check_good txn_abort [$t abort] 0
				} else {
					puts "\tTest$tnum.d: Committing."
					error_check_good txn_commit [$t commit] 0
				}
			}
			error_check_good db_sync [$db sync] 0
			if { [catch {eval \
			    {berkdb dbverify -btcompare test093_cmp1}\
			    $envargs $encargs {$testfile}} res] } {
				puts "FAIL: Verification failed with $res"
			}
		}

		set size4 [file size $filename]
		set free4 [stat_field $db stat "Pages on freelist"]
		set leaf4 [stat_field $db stat "Leaf pages"]
		set internal4 [stat_field $db stat "Internal pages"]

		# The sum of internal pages, leaf pages, and pages freed
		# should decrease on compaction, indicating that pages
		# have been freed to the file system.
		set sum3 [expr $free3 + $leaf3 + $internal3]
		set sum4 [expr $free4 + $leaf4 + $internal4]
		error_check_good pages_freed [expr $sum3 > $sum4] 1

		# Check for file size reduction.
#### We should look at the partitioned files #####
if { [is_partitioned $args] == 0 } {
		error_check_good\
		    file_size [expr [expr $size3 * $reduction] > $size4] 1
}

		puts "\tTest$tnum.j: Contents are the same after compaction."
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		dump_file $db $txn $t2 $checkfunc
		if { $txnenv == 1 } {
			error_check_good t_commit [$t commit] 0
		}
		error_check_good filecmp [filecmp $t1 $t2] 0

		error_check_good db_close [$db close] 0
		if { $env != "NULL" } {
			set testdir [get_home $env]
		}
		cleanup $testdir $env
	}

	# Clean up so the general verification (without the custom comparator)
	# doesn't fail.
	set eindex [lsearch -exact $args "-env"]
	if { $eindex == -1 } {
		set env NULL
	} else {
		incr eindex
		set env [lindex $args $eindex]
		set testdir [get_home $env]
	}
	cleanup $testdir $env
}
