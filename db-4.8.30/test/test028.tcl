# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test028
# TEST	Cursor delete test
# TEST	Test put operations after deleting through a cursor.
proc test028 { method args } {
	global dupnum
	global dupstr
	global alphabet
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	puts "Test028: $method put after cursor delete test"

	if { [is_rbtree $method] == 1 } {
		puts "Test028 skipping for method $method"
		return
	}
	if { [is_record_based $method] == 1 } {
		set key 10
	} else { 
		set key "put_after_cursor_del"
		if { [is_compressed $args] == 0 } {
			append args " -dup"
		}
	}

	# Create the database and open the dictionary
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test028.db
		set env NULL
	} else {
		set testfile test028.db
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set testdir [get_home $env]
	}
	set t1 $testdir/t1
	cleanup $testdir $env
	set db [eval {berkdb_open \
	     -create -mode 0644} $args {$omethod $testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE

	set ndups 20
	set txn ""
	set pflags ""
	set gflags ""

	if { [is_record_based $method] == 1 } {
		set gflags " -recno"
	}

	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good db_cursor [is_substr $dbc $db] 1

	foreach i { offpage onpage } {
		foreach b { bigitem smallitem } {
			if { $i == "onpage" } {
				if { $b == "bigitem" } {
					set dupstr [repeat $alphabet 100]
				} else {
					set dupstr DUP
				}
			} else {
				if { $b == "bigitem" } {
					set dupstr [repeat $alphabet 100]
				} else {
					set dupstr [repeat $alphabet 50]
				}
			}

			if { $b == "bigitem" } {
				set dupstr [repeat $dupstr 10]
			}
			puts "\tTest028: $i/$b"

			puts "\tTest028.a: Insert key with single data item"
			set ret [eval {$db put} \
			    $txn $pflags {$key [chop_data $method $dupstr]}]
			error_check_good db_put $ret 0

			# Now let's get the item and make sure its OK.
			puts "\tTest028.b: Check initial entry"
			set ret [eval {$db get} $txn $gflags {$key}]
			error_check_good db_get \
			    $ret [list [list $key [pad_data $method $dupstr]]]

			# Now try a put with NOOVERWRITE SET (should be error)
			puts "\tTest028.c: No_overwrite test"
			set ret [eval {$db put} $txn \
			    {-nooverwrite $key [chop_data $method $dupstr]}]
			error_check_good \
			    db_put [is_substr $ret "DB_KEYEXIST"] 1

			# Now delete the item with a cursor
			puts "\tTest028.d: Delete test"
			set ret [$dbc get -set $key]
			error_check_bad dbc_get:SET [llength $ret] 0

			set ret [$dbc del]
			error_check_good dbc_del $ret 0

			puts "\tTest028.e: Reput the item"
			set ret [eval {$db put} $txn \
			    {-nooverwrite $key [chop_data $method $dupstr]}]
			error_check_good db_put $ret 0

			puts "\tTest028.f: Retrieve the item"
			set ret [eval {$db get} $txn $gflags {$key}]
			error_check_good db_get $ret \
			    [list [list $key [pad_data $method $dupstr]]]

			# Delete the key to set up for next test
			set ret [eval {$db del} $txn {$key}]
			error_check_good db_del $ret 0

			# Now repeat the above set of tests with
			# duplicates (if not RECNO).
			if { [is_record_based $method] == 1 ||\
			    [is_compressed $args] == 1 } {
				continue;
			}

			puts "\tTest028.g: Insert key with duplicates"
			for { set count 0 } { $count < $ndups } { incr count } {
				set ret [eval {$db put} $txn \
				    {$key [chop_data $method $count$dupstr]}]
				error_check_good db_put $ret 0
			}

			puts "\tTest028.h: Check dups"
			set dupnum 0
			dump_file $db $txn $t1 test028.check

			# Try no_overwrite
			puts "\tTest028.i: No_overwrite test"
			set ret [eval {$db put} \
			    $txn {-nooverwrite $key $dupstr}]
			error_check_good \
			    db_put [is_substr $ret "DB_KEYEXIST"] 1

			# Now delete all the elements with a cursor
			puts "\tTest028.j: Cursor Deletes"
			set count 0
			for { set ret [$dbc get -set $key] } {
			    [string length $ret] != 0 } {
			    set ret [$dbc get -next] } {
				set k [lindex [lindex $ret 0] 0]
				set d [lindex [lindex $ret 0] 1]
				error_check_good db_seq(key) $k $key
				error_check_good db_seq(data) $d $count$dupstr
				set ret [$dbc del]
				error_check_good dbc_del $ret 0
				incr count
				if { $count == [expr $ndups - 1] } {
					puts "\tTest028.k:\
						Duplicate No_Overwrite test"
					set ret [eval {$db put} $txn \
					    {-nooverwrite $key $dupstr}]
					error_check_good db_put [is_substr \
					    $ret "DB_KEYEXIST"] 1
				}
			}

			# Make sure all the items are gone
			puts "\tTest028.l: Get after delete"
			set ret [$dbc get -set $key]
			error_check_good get_after_del [string length $ret] 0

			puts "\tTest028.m: Reput the item"
			set ret [eval {$db put} \
			    $txn {-nooverwrite $key 0$dupstr}]
			error_check_good db_put $ret 0
			for { set count 1 } { $count < $ndups } { incr count } {
				set ret [eval {$db put} $txn \
				    {$key $count$dupstr}]
				error_check_good db_put $ret 0
			}

			puts "\tTest028.n: Retrieve the item"
			set dupnum 0
			dump_file $db $txn $t1 test028.check

			# Clean out in prep for next test
			set ret [eval {$db del} $txn {$key}]
			error_check_good db_del $ret 0
		}
	}
	error_check_good dbc_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0

}

# Check function for test028; keys and data are identical
proc test028.check { key data } {
	global dupnum
	global dupstr
	error_check_good "Bad key" $key put_after_cursor_del
	error_check_good "data mismatch for $key" $data $dupnum$dupstr
	incr dupnum
}
