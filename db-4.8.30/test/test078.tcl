# See the file LICENSE for redistribution information.
#
# Copyright (c) 2000-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test078
# TEST	Test of DBC->c_count(). [#303]
proc test078 { method { nkeys 100 } { pagesize 512 } { tnum "078" } args } {
	source ./include.tcl
	global alphabet
	global is_je_test
	global rand_init

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	puts "Test$tnum ($method): Test of key counts."

	berkdb srand $rand_init

	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	if { $eindex != -1 } {
		incr eindex
	}

	if { $eindex == -1 } {
		set testfile $testdir/test$tnum-a.db
		set env NULL
	} else {
		set testfile test$tnum-a.db
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			set nkeys 50
			append args " -auto_commit "
		}
		set testdir [get_home $env]
	}
	cleanup $testdir $env

	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Test078: skipping for specific pagesizes"
		return
	}
	puts "\tTest$tnum.a: No duplicates, trivial answer."
	puts "\t\tTest$tnum.a.1: Populate database, verify dup counts."
	set db [eval {berkdb_open -create -mode 0644\
	    -pagesize $pagesize} $omethod $args {$testfile}]
	error_check_good db_open [is_valid_db $db] TRUE
	set txn ""

	for { set i 1 } { $i <= $nkeys } { incr i } {
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval {$db put} $txn {$i\
		    [pad_data $method $alphabet$i]}]
		error_check_good put.a($i) $ret 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
		error_check_good count.a [$db count $i] 1
	}

	if { [is_rrecno $method] == 1 } {
		error_check_good db_close.a [$db close] 0
		puts "\tTest$tnum.a2: Skipping remainder of test078 for -rrecno."
		return
	}

	puts "\t\tTest$tnum.a.2: Delete items, verify dup counts again."
	for { set i 1 } { $i <= $nkeys } { incr i } {
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval {$db del} $txn $i]
		error_check_good del.a($i) $ret 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
		error_check_good count.a [$db count $i] 0
	}


	error_check_good db_close.a [$db close] 0

	if { [is_record_based $method] == 1 || [is_rbtree $method] == 1 } {
		puts \
	    "\tTest$tnum.b: Duplicates not supported in $method, skipping."
		return
	}

	foreach {let descrip dupopt} \
	    {b sorted "-dup -dupsort" c unsorted "-dup"} {

		if { [is_compressed $args] } {
			if { $dupopt == "-dup" } {
				continue
			}
		}
		if { $eindex == -1 } {
			set testfile $testdir/test$tnum-b.db
			set env NULL
		} else {
			set testfile test$tnum-b.db
			set env [lindex $args $eindex]
			if { $is_je_test } {
				if { $dupopt == "-dup" } {
					continue
				}
			}
			set testdir [get_home $env]
		}
		cleanup $testdir $env

		puts "\tTest$tnum.$let: Duplicates ($descrip)."
		puts "\t\tTest$tnum.$let.1: Populating database."

		set db [eval {berkdb_open -create -mode 0644\
		    -pagesize $pagesize} $dupopt $omethod $args {$testfile}]
		error_check_good db_open [is_valid_db $db] TRUE

		for { set i 1 } { $i <= $nkeys } { incr i } {
			for { set j 0 } { $j < $i } { incr j } {
				if { $txnenv == 1 } {
					set t [$env txn]
					error_check_good txn \
					    [is_valid_txn $t $env] TRUE
					set txn "-txn $t"
				}
				set ret [eval {$db put} $txn {$i\
				    [pad_data $method $j$alphabet]}]
				error_check_good put.$let,$i $ret 0
				if { $txnenv == 1 } {
					error_check_good txn [$t commit] 0
				}
			}
		}

		puts -nonewline "\t\tTest$tnum.$let.2: "
		puts "Verifying duplicate counts."
$db sync
		for { set i 1 } { $i <= $nkeys } { incr i } {
			error_check_good count.$let,$i \
			    [$db count $i] $i
		}

		puts -nonewline "\t\tTest$tnum.$let.3: "
		puts "Delete every other dup by cursor, verify counts."

		# Delete every other item by cursor and check counts.
 		for { set i 1 } { $i <= $nkeys } { incr i } {
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			set c [eval {$db cursor} $txn]
			error_check_good db_cursor [is_valid_cursor $c $db] TRUE
			set j 0

			for { set ret [$c get -first]} { [llength $ret] > 0 } \
			    { set ret [$c get -next]} {
				set key [lindex [lindex $ret 0] 0]
				if { $key == $i } {
					set data [lindex [lindex $ret 0 ] 1]
					set num [string range $data 0 \
					    end-[string length $alphabet]]
					if { [expr $num % 2] == 0 } {
						error_check_good \
						    c_del [$c del] 0
						incr j
					}
					if { $txnenv == 0 } {
						error_check_good count.$let.$i-$j \
						    [$db count $i] [expr $i - $j]
					}
				}
			}
			error_check_good curs_close [$c close] 0
			if { $txnenv == 1 } {
				error_check_good txn_commit [$t commit] 0
			}
			error_check_good count.$let.$i-$j \
			    [$db count $i] [expr $i - $j]
		}

		puts -nonewline "\t\tTest$tnum.$let.4: "
		puts "Delete all items by cursor, verify counts."
		for { set i 1 } { $i <= $nkeys } { incr i } {
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			set c [eval {$db cursor} $txn]
			error_check_good db_cursor [is_valid_cursor $c $db] TRUE
			for { set ret [$c get -first]} { [llength $ret] > 0 } \
			    { set ret [$c get -next]} {
				set key [lindex [lindex $ret 0] 0]
				if { $key == $i } {
					error_check_good c_del [$c del] 0
				}
			}
			error_check_good curs_close [$c close] 0
			if { $txnenv == 1 } {
				error_check_good txn_commit [$t commit] 0
			}
			error_check_good db_count_zero [$db count $i] 0
		}

		puts -nonewline "\t\tTest$tnum.$let.5: "
		puts "Add back one item, verify counts."
		for { set i 1 } { $i <= $nkeys } { incr i } {
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			set ret [eval {$db put} $txn {$i\
			    [pad_data $method $alphabet]}]
			error_check_good put.$let,$i $ret 0
			if { $txnenv == 1 } {
				error_check_good txn [$t commit] 0
			}
			error_check_good add_one [$db count $i] 1
		}

		puts -nonewline "\t\tTest$tnum.$let.6: "
		puts "Delete remaining entries, verify counts."
		for { set i 1 } { $i <= $nkeys } { incr i } {
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			error_check_good db_del [eval {$db del} $txn {$i}] 0
			if { $txnenv == 1 } {
				error_check_good txn [$t commit] 0
			}
			error_check_good count.$let.$i [$db count $i] 0
		}
		error_check_good db_close.$let [$db close] 0
	}
}
