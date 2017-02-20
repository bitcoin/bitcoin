# See the file LICENSE for redistribution information.
#
# Copyright (c) 2000-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test083
# TEST	Test of DB->key_range.
proc test083 { method {pgsz 512} {maxitems 5000} {step 2} args} {
	source ./include.tcl

	global rand_init
	error_check_good set_random_seed [berkdb srand $rand_init] 0

	set omethod [convert_method $method]
	set args [convert_args $method $args]

	puts "Test083 $method ($args): Test of DB->key_range"
	if { [is_btree $method] != 1 } {
		puts "\tTest083: Skipping for method $method."
		return
	}
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Test083: skipping for specific pagesizes"
		return
	}

	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	if { $eindex == -1 } {
		set testfile $testdir/test083.db
		set env NULL
	} else {
		set testfile test083.db
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set testdir [get_home $env]
	}

	# We assume that numbers will be at most six digits wide
	error_check_bad maxitems_range [expr $maxitems > 999999] 1

	# We want to test key_range on a variety of sizes of btree.
	# Start at ten keys and work up to $maxitems keys, at each step
	# multiplying the number of keys by $step.
	for { set nitems 10 } { $nitems <= $maxitems }\
	    { set nitems [expr $nitems * $step] } {

		puts "\tTest083.a: Opening new database"
		if { $env != "NULL"} {
			set testdir [get_home $env]
		}
		cleanup $testdir $env
		set db [eval {berkdb_open -create -mode 0644} \
		    -pagesize $pgsz $omethod $args $testfile]
		error_check_good dbopen [is_valid_db $db] TRUE

		t83_build $db $nitems $env $txnenv
		t83_test $db $nitems $env $txnenv $args

		error_check_good db_close [$db close] 0
	}
}

proc t83_build { db nitems env txnenv } {
	source ./include.tcl

	puts "\tTest083.b: Populating database with $nitems keys"

	set keylist {}
	puts "\t\tTest083.b.1: Generating key list"
	for { set i 0 } { $i < $nitems } { incr i } {
		lappend keylist $i
	}

	# With randomly ordered insertions, the range of errors we
	# get from key_range can be unpredictably high [#2134].  For now,
	# just skip the randomization step.
	#puts "\t\tTest083.b.2: Randomizing key list"
	#set keylist [randomize_list $keylist]
	#puts "\t\tTest083.b.3: Populating database with randomized keys"

	puts "\t\tTest083.b.2: Populating database"
	set data [repeat . 50]
	set txn ""
	foreach keynum $keylist {
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval {$db put} $txn {key[format %6d $keynum] $data}]
		error_check_good db_put $ret 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
	}
}

proc t83_test { db nitems env txnenv args} {
	# Look at the first key, then at keys about 1/4, 1/2, 3/4, and
	# all the way through the database.  Make sure the key_ranges
	# aren't off by more than 10%.

	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	} else {
		set txn ""
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good dbc [is_valid_cursor $dbc $db] TRUE

	puts "\tTest083.c: Verifying ranges..."

	# Wild guess.  "Tolerance" tests how close the key is to 
	# its expected position.  "Sumtol" tests the sum of the 
	# "less than", "equal to", and "more than", which is 
	# expected to be around 1.  

	if { [is_compressed $args] == 1 } {
		set tolerance 0.5
		set sumtol 0.3
	} elseif { $nitems < 500 || [is_partitioned $args] } {
		set tolerance 0.3
		set sumtol 0.05
	} elseif { $nitems > 500 } {
		set tolerance 0.2
		set sumtol 0.05
	}

	for { set i 0 } { $i < $nitems } \
	    { incr i [expr $nitems / [berkdb random_int 3 16]] } {
		puts -nonewline "\t\t...key $i"
		error_check_bad key0 [llength [set dbt [$dbc get -first]]] 0

		for { set j 0 } { $j < $i } { incr j } {
			error_check_bad key$j \
			    [llength [set dbt [$dbc get -next]]] 0
		}

		set ranges [$db keyrange [lindex [lindex $dbt 0] 0]]
		#puts "ranges is $ranges"
		error_check_good howmanyranges [llength $ranges] 3

		set lessthan [lindex $ranges 0]
		set morethan [lindex $ranges 2]

		puts -nonewline " ... sum of ranges"
		set rangesum [expr $lessthan + [lindex $ranges 1] + $morethan]
		roughly_equal $rangesum 1 $sumtol

		puts "... position of key."
		roughly_equal $lessthan [expr $i * 1.0 / $nitems] $tolerance

	}

	error_check_good dbc_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
}

proc roughly_equal { a b tol } {
	error_check_good "$a =~ $b" [expr abs($a - $b) < $tol] 1
}
