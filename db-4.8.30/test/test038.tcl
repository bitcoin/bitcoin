# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test038
# TEST	DB_GET_BOTH, DB_GET_BOTH_RANGE on deleted items
# TEST
# TEST  Use the first 10,000 entries from the dictionary.  Insert each with
# TEST	self as key and "ndups" duplicates.  For the data field, prepend the
# TEST	letters of the alphabet in a random order so we force the duplicate
# TEST	sorting code to do something.  By setting ndups large, we can make
# TEST	this an off-page test
# TEST
# TEST	Test the DB_GET_BOTH and DB_GET_BOTH_RANGE functionality by retrieving
# TEST	each dup in the file explicitly.  Then remove each duplicate and try
# TEST	the retrieval again.
proc test038 { method {nentries 10000} {ndups 5} {tnum "038"} args } {
	global alphabet
	global rand_init
	source ./include.tcl

	berkdb srand $rand_init

	set args [convert_args $method $args]
	set checkargs [split_partition_args $args]

	# The checkdb is of type hash so it can't use compression.
	set checkargs [strip_compression_args $checkargs]

	set omethod [convert_method $method]

	if { [is_record_based $method] == 1 || \
	    [is_rbtree $method] == 1 } {
		puts "Test$tnum skipping for method $method"
		return
	}
	# Create the database and open the dictionary
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test$tnum.db
		set checkdb $testdir/checkdb.db
		set env NULL
	} else {
		set testfile test$tnum.db
		set checkdb checkdb.db
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
			append checkargs " -auto_commit "
			#
			# If we are using txns and running with the
			# default, set the default down a bit.
			#
			if { $nentries == 10000 } {
				set nentries 100
			}
			reduce_dups nentries ndups
		}
		set testdir [get_home $env]
	}
	set t1 $testdir/t1
	set t2 $testdir/t2
	set t3 $testdir/t3
	cleanup $testdir $env

	puts "Test$tnum: \
	    $method ($args) $nentries small sorted dup key/data pairs"
	set db [eval {berkdb_open -create -mode 0644 \
		$omethod -dup -dupsort} $args {$testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE
	set did [open $dict]

	set check_db [eval {berkdb_open \
	     -create -mode 0644 -hash} $checkargs {$checkdb}]
	error_check_good dbopen:check_db [is_valid_db $check_db] TRUE

	set pflags ""
	set gflags ""
	set txn ""
	set count 0

	# Here is the loop where we put and get each key/data pair
	puts "\tTest$tnum.a: Put/get loop"
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good cursor_open [is_valid_cursor $dbc $db] TRUE
	while { [gets $did str] != -1 && $count < $nentries } {
		set dups ""
		for { set i 1 } { $i <= $ndups } { incr i } {
			set pref \
			    [string index $alphabet [berkdb random_int 0 25]]
			set pref $pref[string \
			    index $alphabet [berkdb random_int 0 25]]
			while { [string first $pref $dups] != -1 } {
				set pref [string toupper $pref]
				if { [string first $pref $dups] != -1 } {
					set pref [string index $alphabet \
					    [berkdb random_int 0 25]]
					set pref $pref[string index $alphabet \
					    [berkdb random_int 0 25]]
				}
			}
			if { [string length $dups] == 0 } {
				set dups $pref
			} else {
				set dups "$dups $pref"
			}
			set datastr $pref:$str
			set ret [eval {$db put} \
			    $txn $pflags {$str [chop_data $method $datastr]}]
			error_check_good put $ret 0
		}
		set ret [eval {$check_db put} \
		    $txn $pflags {$str [chop_data $method $dups]}]
		error_check_good checkdb_put $ret 0

		# Now retrieve all the keys matching this key
		set x 0
		set lastdup ""
		for {set ret [$dbc get -set $str]} \
		    {[llength $ret] != 0} \
		    {set ret [$dbc get -nextdup] } {
			set k [lindex [lindex $ret 0] 0]
			if { [string compare $k $str] != 0 } {
				break
			}
			set datastr [lindex [lindex $ret 0] 1]
			if {[string length $datastr] == 0} {
				break
			}
			if {[string compare $lastdup $datastr] > 0} {
				error_check_good sorted_dups($lastdup,$datastr)\
				    0 1
			}
			incr x
			set lastdup $datastr
		}
		error_check_good "Test$tnum:ndups:$str" $x $ndups
		incr count
	}
	error_check_good cursor_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	close $did

	# Now check the duplicates, then delete then recheck
	puts "\tTest$tnum.b: Checking and Deleting duplicates"
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good cursor_open [is_valid_cursor $dbc $db] TRUE
	set check_c [eval {$check_db cursor} $txn]
	error_check_good cursor_open [is_valid_cursor $check_c $check_db] TRUE

	for {set ndx 0} {$ndx < $ndups} {incr ndx} {
		for {set ret [$check_c get -first]} \
		    {[llength $ret] != 0} \
		    {set ret [$check_c get -next] } {
			set k [lindex [lindex $ret 0] 0]
			set d [lindex [lindex $ret 0] 1]
			error_check_bad data_check:$d [string length $d] 0

			set nn [expr $ndx * 3]
			set pref [string range $d $nn [expr $nn + 1]]
			set data $pref:$k
			set ret [$dbc get -get_both $k $data]
			error_check_good \
			    get_both_key:$k [lindex [lindex $ret 0] 0] $k
			error_check_good \
			    get_both_data:$k [lindex [lindex $ret 0] 1] $data

			set ret [$dbc get -get_both_range $k $pref]
			error_check_good \
			    get_both_key:$k [lindex [lindex $ret 0] 0] $k
			error_check_good \
			    get_both_data:$k [lindex [lindex $ret 0] 1] $data

			set ret [$dbc del]
			error_check_good del $ret 0

			set ret [eval {$db get} $txn {-get_both $k $data}]
			error_check_good error_case:$k [llength $ret] 0

			# We should either not find anything (if deleting the
			# largest duplicate in the set) or a duplicate that
			# sorts larger than the one we deleted.
			set ret [$dbc get -get_both_range $k $pref]
			if { [llength $ret] != 0 } {
				set datastr [lindex [lindex $ret 0] 1]]
				if {[string compare \
				    $pref [lindex [lindex $ret 0] 1]] >= 0} {
					error_check_good \
				error_case_range:sorted_dups($pref,$datastr) 0 1
				}
			}

			if {$ndx != 0} {
				set n [expr ($ndx - 1) * 3]
				set pref [string range $d $n [expr $n + 1]]
				set data $pref:$k
				set ret \
				    [eval {$db get} $txn {-get_both $k $data}]
				error_check_good error_case:$k [llength $ret] 0
			}
		}
	}

	error_check_good check_c:close [$check_c close] 0
	error_check_good dbc_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}

	error_check_good check_db:close [$check_db close] 0
	error_check_good db_close [$db close] 0
}
