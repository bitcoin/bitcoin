# See the file LICENSE for redistribution information.
#
# Copyright (c) 2005-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test113
# TEST	Test database compaction with duplicates.
# TEST
# TEST	This is essentially test111 with duplicates.
# TEST	To make it simple we use numerical keys all the time.
# TEST
# TEST	Dump and save contents.  Compact the database, dump again,
# TEST	and make sure we still have the same contents.
# TEST  Add back some entries, delete more entries (this time by
# TEST	cursor), dump, compact, and do the before/after check again.

proc test113 { method {nentries 10000} {ndups 5} {tnum "113"} args } {
	source ./include.tcl
	global alphabet

	# Compaction and duplicates can occur only with btree.
	if { [is_btree $method] != 1 } {
		puts "Skipping test$tnum for method $method."
		return
	}

        # Skip for specified pagesizes.  This test uses a small
	# pagesize to generate a deep tree.
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Test$tnum: Skipping for specific pagesizes"
		return
	}

	set args [convert_args $method $args]
	set omethod [convert_method $method]
	if  { [is_partition_callback $args] == 1 } {
		set nodump 1
	} else {
		set nodump 0
	}

	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	set eindex [lsearch -exact $args "-env"]
	set txnenv 0
	set txn ""
	if { $eindex == -1 } {
		set testfile $testdir/test$tnum.db
		set env NULL
	} else {
		set testfile test$tnum.db
		incr eindex
		set env [lindex $args $eindex]
		set rpcenv [is_rpcenv $env]
		if { $rpcenv == 1 } {
			puts "Test$tnum: skipping for RPC"
			return
		}
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set testdir [get_home $env]
	}

	puts "Test$tnum: $method ($args)\
	    Database compaction with duplicates."

	set t1 $testdir/t1
	set t2 $testdir/t2
	cleanup $testdir $env

	set db [eval {berkdb_open -create -pagesize 512 \
	    -dup -dupsort -mode 0644} $args $omethod $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	puts "\tTest$tnum.a: Populate database with dups."
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	for { set i 1 } { $i <= $nentries } { incr i } {
		set key $i
		for { set j 1 } { $j <= $ndups } { incr j } {
			set str $i.$j.$alphabet
			set ret [eval \
			    {$db put} $txn {$key [chop_data $method $str]}]
			error_check_good put $ret 0
		}
	}
	if { $txnenv == 1 } {
		error_check_good txn_commit [$t commit] 0
	}
	error_check_good db_sync [$db sync] 0

	if { $env != "NULL" } {
		set testdir [get_home $env]
		set filename $testdir/$testfile
	} else {
		set filename $testfile
	}
	set size1 [file size $filename]
	set free1 [stat_field $db stat "Pages on freelist"]

	puts "\tTest$tnum.b: Delete most entries from database."
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	for { set i $nentries } { $i >= 0 } { incr i -1 } {
		set key $i

		# Leave every n'th item.
		set n 7
		if { [expr $i % $n] != 0 } {
			set ret [eval {$db del} $txn {$key}]
			error_check_good del $ret 0
		}
	}
	if { $txnenv == 1 } {
		error_check_good txn_commit [$t commit] 0
	}
	error_check_good db_sync [$db sync] 0

	puts "\tTest$tnum.c: Do a dump_file on contents."
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	dump_file $db $txn $t1
	if { $txnenv == 1 } {
		error_check_good txn_commit [$t commit] 0
	}

	puts "\tTest$tnum.d: Compact database."
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
		error_check_good verify_dir \
		    [ verify_dir $testdir "" 0 0 $nodump] 0
	}

	set size2 [file size $filename]
	set free2 [stat_field $db stat "Pages on freelist"]
	error_check_good pages_freed [expr $free2 > $free1] 1
#### We should look at the partitioned files #####
if { [is_partitioned $args] == 0 } {
	set reduction .80
	error_check_good file_size [expr [expr $size1 * $reduction] > $size2] 1
}

	puts "\tTest$tnum.e: Check that contents are the same after compaction."
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	dump_file $db $txn $t2
	if { $txnenv == 1 } {
		error_check_good txn_commit [$t commit] 0
	}
	error_check_good filecmp [filecmp $t1 $t2] 0

	puts "\tTest$tnum.f: Add more entries to database."
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	for { set i 1 } { $i <= $nentries } { incr i } {
		set key $i
		for { set j 1 } { $j <= $ndups } { incr j } {
			set str $i.$j.$alphabet.extra
			set ret [eval \
			    {$db put} $txn {$key [chop_data $method $str]}]
			error_check_good put $ret 0
		}
	}
	if { $txnenv == 1 } {
		error_check_good txn_commit [$t commit] 0
	}
	error_check_good db_sync [$db sync] 0

	set size3 [file size $filename]
	set free3 [stat_field $db stat "Pages on freelist"]

	puts "\tTest$tnum.g: Remove more entries, this time by cursor."
	set i 0
	set n 11
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]

	for { set dbt [$dbc get -first] } { [llength $dbt] > 0 }\
	    { set dbt [$dbc get -next] ; incr i } {

		if { [expr $i % $n] != 0 } {
			error_check_good dbc_del [$dbc del] 0
		}
	}
	error_check_good cursor_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn_commit [$t commit] 0
	}
	error_check_good db_sync [$db sync] 0

	puts "\tTest$tnum.h: Save contents."
	dump_file $db "" $t1

	puts "\tTest$tnum.i: Compact database again."
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
		error_check_good verify_dir \
		    [ verify_dir $testdir "" 0 0 $nodump] 0
	}

	set size4 [file size $filename]
	set free4 [stat_field $db stat "Pages on freelist"]
	error_check_good pages_freed [expr $free4 > $free3] 1
#### We should look at the partitioned files #####
if { [is_partitioned $args] == 0 } {
	error_check_good file_size [expr [expr $size3 * $reduction] > $size4] 1
}

	puts "\tTest$tnum.j: Check that contents are the same after compaction."
	dump_file $db "" $t2
	error_check_good filecmp [filecmp $t1 $t2] 0

	error_check_good db_close [$db close] 0
}
