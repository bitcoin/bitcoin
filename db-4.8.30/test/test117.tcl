# See the file LICENSE for redistribution information.
#
# Copyright (c) 2005-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test117
# TEST	Test database compaction with requested fill percent.
# TEST
# TEST	Populate a database.  Remove a high proportion of entries.
# TEST	Dump and save contents.  Compact the database, requesting
# TEST	fill percentages starting at 10% and working our way up to
# TEST	100.  On each cycle, make sure we still have the same contents.
# TEST
# TEST	Unlike the other compaction tests, this one does not
# TEST	use -freespace.

proc test117 { method {nentries 10000} {tnum "117"} args } {
	source ./include.tcl

	# Compaction is an option for btree and recno databases only.
	if { [is_hash $method] == 1 || [is_queue $method] == 1 } {
		puts "Skipping test$tnum for method $method."
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
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	if { $eindex == -1 } {
		set basename $testdir/test$tnum
		set env NULL
	} else {
		set basename test$tnum
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
	puts "Test$tnum: ($method $args) Database compaction and fillpercent."
	set t1 $testdir/t1
	set t2 $testdir/t2
	set splitopts { "" "-revsplitoff" }
	set txn ""

	if { [is_record_based $method] == 1 } {
		set checkfunc test001_recno.check
	} else {
		set checkfunc test001.check
	}

	foreach splitopt $splitopts {
		set testfile $basename.db
		if { $splitopt == "-revsplitoff" } {
			set testfile $basename.rev.db
	 		if { [is_record_based $method] == 1 } {
				puts "Skipping\
				    -revsplitoff option for method $method."
				continue
			}
		}
		set did [open $dict]
		if { $env != "NULL" } {
			set testdir [get_home $env]
		}
		cleanup $testdir $env

		puts "\tTest$tnum.a: Create and populate database ($splitopt)."
		set db [eval {berkdb_open -create \
		    -mode 0644} $splitopt $args $omethod $testfile]
		error_check_good dbopen [is_valid_db $db] TRUE

		set count 0
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		while { [gets $did str] != -1 && $count < $nentries } {
			global kvals

			if { [is_record_based $method] == 1 } {
				set key [expr $count + 1]
				set kvals($key) [pad_data $method $str]
			} else {
				set key $str
				set str [reverse $str]
			}

			set ret [eval \
			    {$db put} $txn {$key [chop_data $method $str]}]
			error_check_good put $ret 0
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

		puts "\tTest$tnum.b: Delete most entries from database."
		set did [open $dict]
		set count [expr $nentries - 1]
		set n 17

		# Leave every nth item.  Since rrecno renumbers, we
		# delete starting at nentries and working down to 0.
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		while { [gets $did str] != -1 && $count > 0 } {
			if { [is_record_based $method] == 1 } {
				set key [expr $count + 1]
			} else {
				set key $str
			}

			if { [expr $count % $n] != 0 } {
				set ret [eval {$db del} $txn {$key}]
				error_check_good del $ret 0
			}
			incr count -1
		}
		if { $txnenv == 1 } {
			error_check_good t_commit [$t commit] 0
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

		# Start by compacting pages filled less than 10% and
		# work up to 100%.
		for { set fillpercent 10 } { $fillpercent <= 100 }\
		    { incr fillpercent 10 } {

			puts "\tTest$tnum.d: Compact and verify database\
			    with fillpercent $fillpercent."
			set ret [$db compact -fillpercent $fillpercent]
			error_check_good db_sync [$db sync] 0
			set size2 [file size $filename]
			error_check_good verify_dir \
			     [verify_dir $testdir "" 0 0 $nodump] 0
			set free2 [stat_field $db stat "Pages on freelist"]

			# The number of free pages should never decline.
			error_check_good pages_freed [expr $free2 >= $free1] 1
			error_check_good file_size [expr $size2 <= $size1] 1

			puts "\tTest$tnum.e:\
			    Contents are the same after compaction."
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
			set free1 $free2
			set size1 $size2
		}
		error_check_good db_close [$db close] 0
		close $did
	}
}
