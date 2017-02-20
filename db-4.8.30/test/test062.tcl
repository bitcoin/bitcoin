# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test062
# TEST	Test of partial puts (using DB_CURRENT) onto duplicate pages.
# TEST	Insert the first 200 words into the dictionary 200 times each with
# TEST	self as key and <random letter>:self as data.  Use partial puts to
# TEST	append self again to data;  verify correctness.
proc test062 { method {nentries 200} {ndups 200} {tnum "062"} args } {
	global alphabet
	global rand_init
	source ./include.tcl

	berkdb srand $rand_init

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	if { [is_record_based $method] == 1 || [is_rbtree $method] == 1 } {
		puts "Test$tnum skipping for method $omethod"
		return
	}

	# Btree with compression does not support unsorted duplicates.
	if { [is_compressed $args] == 1 } {
		puts "Test$tnum skipping for btree with compression."
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
		set env NULL
	} else {
		set testfile test$tnum.db
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
			#
			# If we are using txns and running with the
			# default, set the default down a bit.
			#
			if { $nentries == 200 } {
				set nentries 100
			}
			reduce_dups nentries ndups
		}
		set testdir [get_home $env]
	}
	cleanup $testdir $env

	puts "Test$tnum:\
	    $method ($args) $nentries Partial puts and $ndups duplicates."
	set db [eval {berkdb_open -create -mode 0644 \
	    $omethod -dup} $args {$testfile} ]
	error_check_good dbopen [is_valid_db $db] TRUE
	set did [open $dict]

	set pflags ""
	set gflags ""
	set txn ""
	set count 0

	# Here is the loop where we put each key/data pair
	puts "\tTest$tnum.a: Put loop (initialize database)"
	while { [gets $did str] != -1 && $count < $nentries } {
		for { set i 1 } { $i <= $ndups } { incr i } {
			set pref \
			    [string index $alphabet [berkdb random_int 0 25]]
			set datastr $pref:$str
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
		set keys($count) $str

		incr count
	}
	close $did

	puts "\tTest$tnum.b: Partial puts."
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good cursor_open [is_substr $dbc $db] 1

	# Do a partial write to extend each datum in
	# the regular db by the corresponding dictionary word.
	# We have to go through each key's dup set using -set
	# because cursors are not stable in the hash AM and we
	# want to make sure we hit all the keys.
	for { set i 0 } { $i < $count } { incr i } {
		set key $keys($i)
		for {set ret [$dbc get -set $key]}  \
		    {[llength $ret] != 0} \
		    {set ret [$dbc get -nextdup]} {

			set k [lindex [lindex $ret 0] 0]
			set orig_d [lindex [lindex $ret 0] 1]
			set d [string range $orig_d 2 end]
			set doff [expr [string length $d] + 2]
			set dlen 0
			error_check_good data_and_key_sanity $d $k

			set ret [$dbc get -current]
			error_check_good before_sanity \
			    [lindex [lindex $ret 0] 0] \
			    [string range [lindex [lindex $ret 0] 1] 2 end]

			error_check_good partial_put [eval {$dbc put -current \
			    -partial [list $doff $dlen] $d}] 0

			set ret [$dbc get -current]
			error_check_good partial_put_correct \
			    [lindex [lindex $ret 0] 1] $orig_d$d
		}
	}

	puts "\tTest$tnum.c: Double-checking get loop."
	# Double-check that each datum in the regular db has
	# been appropriately modified.

	for {set ret [$dbc get -first]} \
	    {[llength $ret] != 0} \
	    {set ret [$dbc get -next]} {

		set k [lindex [lindex $ret 0] 0]
		set d [lindex [lindex $ret 0] 1]
		error_check_good modification_correct \
		    [string range $d 2 end] [repeat $k 2]
	}

	error_check_good dbc_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0
}
