# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test030
# TEST	Test DB_NEXT_DUP Functionality.
proc test030 { method {nentries 10000} args } {
	global rand_init
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	# Btree with compression does not support unsorted duplicates.
	if { [is_compressed $args] == 1 } {
		puts "Test030 skipping for btree with compression."
		return
	}
	if { [is_record_based $method] == 1 ||
	    [is_rbtree $method] == 1 } {
		puts "Test030 skipping for method $method"
		return
	}
	berkdb srand $rand_init

	# Create the database and open the dictionary
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test030.db
		set cntfile $testdir/cntfile.db
		set env NULL
	} else {
		set testfile test030.db
		set cntfile cntfile.db
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
			#
			# If we are using txns and running with the
			# default, set the default down a bit.
			#
			if { $nentries == 10000 } {
				set nentries 100
			}
		}
		set testdir [get_home $env]
	}

	puts "Test030: $method ($args) $nentries DB_NEXT_DUP testing"
	set t1 $testdir/t1
	set t2 $testdir/t2
	set t3 $testdir/t3
	cleanup $testdir $env

	set db [eval {berkdb_open -create \
		-mode 0644 -dup} $args {$omethod $testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE

	# Use a second DB to keep track of how many duplicates
	# we enter per key

	set cntdb [eval {berkdb_open -create \
		-mode 0644} $args {-btree $cntfile}]
	error_check_good dbopen:cntfile [is_valid_db $db] TRUE

	set pflags ""
	set gflags ""
	set txn ""
	set count 0

	# Here is the loop where we put and get each key/data pair
	# We will add between 1 and 10 dups with values 1 ... dups
	# We'll verify each addition.

	set did [open $dict]
	puts "\tTest030.a: put and get duplicate keys."
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]

	while { [gets $did str] != -1 && $count < $nentries } {
		set ndup [berkdb random_int 1 10]

		for { set i 1 } { $i <= $ndup } { incr i 1 } {
			set ctxn ""
			if { $txnenv == 1 } {
				set ct [$env txn]
				error_check_good txn \
				    [is_valid_txn $ct $env] TRUE
				set ctxn "-txn $ct"
			}
			set ret [eval {$cntdb put} \
			    $ctxn $pflags {$str [chop_data $method $ndup]}]
			error_check_good put_cnt $ret 0
			if { $txnenv == 1 } {
				error_check_good txn [$ct commit] 0
			}
			set datastr $i:$str
			set ret [eval {$db put} \
			    $txn $pflags {$str [chop_data $method $datastr]}]
			error_check_good put $ret 0
		}

		# Now retrieve all the keys matching this key
		set x 0
		for {set ret [$dbc get -set $str]} \
		    {[llength $ret] != 0} \
		    {set ret [$dbc get -nextdup] } {

			if { [llength $ret] == 0 } {
				break
			}
			incr x

			set k [lindex [lindex $ret 0] 0]
			if { [string compare $k $str] != 0 } {
				break
			}

			set datastr [lindex [lindex $ret 0] 1]
			set d [data_of $datastr]
			error_check_good Test030:put $d $str

			set id [ id_of $datastr ]
			error_check_good Test030:dup# $id $x
		}
		error_check_good Test030:numdups $x $ndup

		# Now retrieve them backwards
		for {set ret [$dbc get -prev]} \
		    {[llength $ret] != 0} \
		    {set ret [$dbc get -prevdup] } {

			if { [llength $ret] == 0 } {
				break
			}

			set k [lindex [lindex $ret 0] 0]
			if { [string compare $k $str] != 0 } {
				break
			}
			incr x -1

			set datastr [lindex [lindex $ret 0] 1]
			set d [data_of $datastr]
			error_check_good Test030:put $d $str

			set id [ id_of $datastr ]
			error_check_good Test030:dup# $id $x
		}
		error_check_good Test030:numdups $x 1
		incr count
	}
	close $did

	# Verify on sequential pass of entire file
	puts "\tTest030.b: sequential check"

	# We can't just set lastkey to a null string, since that might
	# be a key now!
	set lastkey "THIS STRING WILL NEVER BE A KEY"

	for {set ret [$dbc get -first]} \
	    {[llength $ret] != 0} \
	    {set ret [$dbc get -next] } {

		# Outer loop should always get a new key

		set k [lindex [lindex $ret 0] 0]
		error_check_bad outer_get_loop:key $k $lastkey

		set datastr [lindex [lindex $ret 0] 1]
		set d [data_of $datastr]
		set id [ id_of $datastr ]

		error_check_good outer_get_loop:data $d $k
		error_check_good outer_get_loop:id $id 1

		set lastkey $k
		# Figure out how may dups we should have
		if { $txnenv == 1 } {
			set ct [$env txn]
			error_check_good txn [is_valid_txn $ct $env] TRUE
			set ctxn "-txn $ct"
		}
		set ret [eval {$cntdb get} $ctxn $pflags {$k}]
		set ndup [lindex [lindex $ret 0] 1]
		if { $txnenv == 1 } {
			error_check_good txn [$ct commit] 0
		}

		set howmany 1
		for { set ret [$dbc get -nextdup] } \
		    { [llength $ret] != 0 } \
		    { set ret [$dbc get -nextdup] } {
			incr howmany

			set k [lindex [lindex $ret 0] 0]
			error_check_good inner_get_loop:key $k $lastkey

			set datastr [lindex [lindex $ret 0] 1]
			set d [data_of $datastr]
			set id [ id_of $datastr ]

			error_check_good inner_get_loop:data $d $k
			error_check_good inner_get_loop:id $id $howmany

		}
		error_check_good ndups_found $howmany $ndup
	}

	# Verify on key lookup
	puts "\tTest030.c: keyed check"
	set cnt_dbc [$cntdb cursor]
	for {set ret [$cnt_dbc get -first]} \
	    {[llength $ret] != 0} \
	    {set ret [$cnt_dbc get -next] } {
		set k [lindex [lindex $ret 0] 0]

		set howmany [lindex [lindex $ret 0] 1]
		error_check_bad cnt_seq:data [string length $howmany] 0

		set i 0
		for {set ret [$dbc get -set $k]} \
		    {[llength $ret] != 0} \
		    {set ret [$dbc get -nextdup] } {
			incr i

			set k [lindex [lindex $ret 0] 0]

			set datastr [lindex [lindex $ret 0] 1]
			set d [data_of $datastr]
			set id [ id_of $datastr ]

			error_check_good inner_get_loop:data $d $k
			error_check_good inner_get_loop:id $id $i
		}
		error_check_good keyed_count $i $howmany

	}
	error_check_good cnt_curs_close [$cnt_dbc close] 0
	error_check_good db_curs_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good cnt_file_close [$cntdb close] 0
	error_check_good db_file_close [$db close] 0
}
