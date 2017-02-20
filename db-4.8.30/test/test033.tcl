# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test033
# TEST	DB_GET_BOTH without comparison function
# TEST
# TEST	Use the first 10,000 entries from the dictionary.  Insert each with
# TEST	self as key and data; add duplicate records for each.  After all are
# TEST	entered, retrieve all and verify output using DB_GET_BOTH (on DB and
# TEST	DBC handles) and DB_GET_BOTH_RANGE (on a DBC handle) on existent and
# TEST	nonexistent keys.
# TEST
# TEST	XXX
# TEST	This does not work for rbtree.
proc test033 { method {nentries 10000} {ndups 5} {tnum "033"} args } {
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]
	if { [is_rbtree $method] == 1 } {
		puts "Test$tnum skipping for method $method"
		return
	}

	# Btree with compression does not support unsorted duplicates.
	if { [is_compressed $args] == 1 } {
		puts "Test$tnum skipping for btree with compression."
		return
	}

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
			if { $nentries == 10000 } {
				set nentries 100
			}
			reduce_dups nentries ndups
		}
		set testdir [get_home $env]
	}

	puts "Test$tnum: $method ($args) $nentries small $ndups dup key/data pairs"
	set t1 $testdir/t1
	set t2 $testdir/t2
	set t3 $testdir/t3
	cleanup $testdir $env

	# Duplicate data entries are not allowed in record based methods.
	if { [is_record_based $method] == 1 } {
		set db [eval {berkdb_open -create -mode 0644 \
			$omethod} $args {$testfile}]
	} else {
		set db [eval {berkdb_open -create -mode 0644 \
			$omethod -dup} $args {$testfile}]
	}
	error_check_good dbopen [is_valid_db $db] TRUE

	set pflags ""
	set gflags ""
	set txn ""

	# Allocate a cursor for DB_GET_BOTH_RANGE.
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good cursor_open [is_valid_cursor $dbc $db] TRUE

	puts "\tTest$tnum.a: Put/get loop."
	# Here is the loop where we put and get each key/data pair
	set count 0
	set did [open $dict]
	while { [gets $did str] != -1 && $count < $nentries } {
		if { [is_record_based $method] == 1 } {
			set key [expr $count + 1]
			set ret [eval {$db put} $txn $pflags \
			    {$key [chop_data $method $str]}]
			error_check_good put $ret 0
		} else {
			for { set i 1 } { $i <= $ndups } { incr i } {
				set datastr $i:$str
				set ret [eval {$db put} \
				    $txn $pflags {$str [chop_data $method $datastr]}]
				error_check_good db_put $ret 0
			}
		}

		# Now retrieve all the keys matching this key and dup
		# for non-record based AMs.
		if { [is_record_based $method] == 1 } {
			test033_recno.check $db $dbc $method $str $txn $key
		} else {
			test033_check $db $dbc $method $str $txn $ndups
		}
		incr count
	}

	close $did

	puts "\tTest$tnum.b: Verifying DB_GET_BOTH after creation."
	set count 0
	set did [open $dict]
	while { [gets $did str] != -1 && $count < $nentries } {
		# Now retrieve all the keys matching this key
		# for non-record based AMs.
		if { [is_record_based $method] == 1 } {
			set key [expr $count + 1]
			test033_recno.check $db $dbc $method $str $txn $key
		} else {
			test033_check $db $dbc $method $str $txn $ndups
		}
		incr count
	}
	close $did

	error_check_good dbc_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0
}

# No testing of dups is done on record-based methods.
proc test033_recno.check {db dbc method str txn key} {
	set ret [eval {$db get} $txn {-recno $key}]
	error_check_good "db_get:$method" \
	    [lindex [lindex $ret 0] 1] [pad_data $method $str]
	set ret [$dbc get -get_both $key [pad_data $method $str]]
	error_check_good "db_get_both:$method" \
	    [lindex [lindex $ret 0] 1] [pad_data $method $str]
}

# Testing of non-record-based methods includes duplicates
# and get_both_range.
proc test033_check {db dbc method str txn ndups} {
	for {set i 1} {$i <= $ndups } { incr i } {
		set datastr $i:$str

		set ret [eval {$db get} $txn {-get_both $str $datastr}]
		error_check_good "db_get_both:dup#" \
		    [lindex [lindex $ret 0] 1] $datastr

		set ret [$dbc get -get_both $str $datastr]
		error_check_good "dbc_get_both:dup#" \
		    [lindex [lindex $ret 0] 1] $datastr

		set ret [$dbc get -get_both_range $str $datastr]
		error_check_good "dbc_get_both_range:dup#" \
		    [lindex [lindex $ret 0] 1] $datastr
	}

	# Now retrieve non-existent dup (i is ndups + 1)
	set datastr $i:$str
	set ret [eval {$db get} $txn {-get_both $str $datastr}]
	error_check_good db_get_both:dupfailure [llength $ret] 0
	set ret [$dbc get -get_both $str $datastr]
	error_check_good dbc_get_both:dupfailure [llength $ret] 0
	set ret [$dbc get -get_both_range $str $datastr]
	error_check_good dbc_get_both_range [llength $ret] 0
}
