# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test089
# TEST	Concurrent Data Store test (CDB)
# TEST
# TEST	Enhanced CDB testing to test off-page dups, cursor dups and
# TEST	cursor operations like c_del then c_get.
proc test089 { method {nentries 1000} args } {
	global datastr
	global encrypt
	source ./include.tcl

	#
	# If we are using an env, then skip this test.  It needs its own.
	set eindex [lsearch -exact $args "-env"]
	if { $eindex != -1 } {
		incr eindex
		set env [lindex $args $eindex]
		puts "Test089 skipping for env $env"
		return
	}
	set encargs ""
	set args [convert_args $method $args]
	set oargs [split_encargs $args encargs]
	set omethod [convert_method $method]
	set pageargs ""
	split_pageargs $args pageargs

	puts "Test089: ($oargs) $method CDB Test cursor/dup operations"

	# Process arguments
	# Create the database and open the dictionary
	set testfile test089.db
	set testfile1 test089a.db

	env_cleanup $testdir

	set env [eval \
	     {berkdb_env -create -cdb} $pageargs $encargs -home $testdir]
	error_check_good dbenv [is_valid_env $env] TRUE

	set db [eval {berkdb_open -env $env -create \
	    -mode 0644 $omethod} $oargs {$testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE

	set db1 [eval {berkdb_open -env $env -create \
	    -mode 0644 $omethod} $oargs {$testfile1}]
	error_check_good dbopen [is_valid_db $db1] TRUE

	set pflags ""
	set gflags ""
	set txn ""
	set count 0

	# Here is the loop where we put each key/data pair
	puts "\tTest089.a: Put loop"
	set did [open $dict]
	while { [gets $did str] != -1 && $count < $nentries } {
		if { [is_record_based $method] == 1 } {
			set key [expr $count + 1]
		} else {
			set key $str
		}
		set ret [eval {$db put} \
		    $txn $pflags {$key [chop_data $method $datastr]}]
		error_check_good put:$db $ret 0
		set ret [eval {$db1 put} \
		    $txn $pflags {$key [chop_data $method $datastr]}]
		error_check_good put:$db1 $ret 0
		incr count
	}
	close $did
	error_check_good close:$db [$db close] 0
	error_check_good close:$db1 [$db1 close] 0

	# Database is created, now set up environment

	# Remove old mpools and Open/create the lock and mpool regions
	error_check_good env:close:$env [$env close] 0
	set ret [eval {berkdb envremove} $encargs -home $testdir]
	error_check_good env_remove $ret 0

	set env [eval \
	     {berkdb_env_noerr -create -cdb} $pageargs $encargs -home $testdir]
	error_check_good dbenv [is_valid_widget $env env] TRUE

	puts "\tTest089.b: CDB cursor dups"

	set db1 [eval {berkdb_open_noerr -env $env -create \
	    -mode 0644 $omethod} $oargs {$testfile1}]
	error_check_good dbopen [is_valid_db $db1] TRUE

	# Create a read-only cursor and make sure we can't write with it.
	set dbcr [$db1 cursor]
	error_check_good dbcursor [is_valid_cursor $dbcr $db1] TRUE
	set ret [$dbcr get -first]
	catch { [$dbcr put -current data] } ret
	error_check_good is_read_only \
	    [is_substr $ret "Write attempted on read-only cursor"] 1
	error_check_good dbcr_close [$dbcr close] 0

	# Create a write cursor and duplicate it.
	set dbcw [$db1 cursor -update]
	error_check_good dbcursor [is_valid_cursor $dbcw $db1] TRUE
	set dup_dbcw [$dbcw dup]
	error_check_good dup_write_cursor [is_valid_cursor $dup_dbcw $db1] TRUE

	# Position both cursors at get -first.  They should find the same data.
	set get_first [$dbcw get -first]
	set get_first_dup [$dup_dbcw get -first]
	error_check_good dup_read $get_first $get_first_dup

	# Test that the write cursors can both write and that they
	# read each other's writes correctly.  First write reversed
	# datastr with original cursor and read with dup cursor.
	error_check_good put_current_orig \
	    [$dbcw put -current [chop_data $method [reverse $datastr]]] 0
	set reversed [$dup_dbcw get -current]
	error_check_good check_with_dup [lindex [lindex $reversed 0] 1] \
	    [pad_data $method [reverse $datastr]]

	# Write forward datastr with dup cursor and read with original.
	error_check_good put_current_dup \
	    [$dup_dbcw put -current [chop_data $method $datastr]] 0
	set forward [$dbcw get -current]
	error_check_good check_with_orig $forward $get_first

	error_check_good dbcw_close [$dbcw close] 0
	error_check_good dup_dbcw_close [$dup_dbcw close] 0

	# This tests the failure found in #1923
	puts "\tTest089.c: Test delete then get"

	set dbc [$db1 cursor -update]
	error_check_good dbcursor [is_valid_cursor $dbc $db1] TRUE

	for {set kd [$dbc get -first] } { [llength $kd] != 0 } \
	    {set kd [$dbc get -next] } {
		error_check_good dbcdel [$dbc del] 0
	}
	error_check_good dbc_close [$dbc close] 0
	error_check_good db_close [$db1 close] 0
	error_check_good env_close [$env close] 0

	if { [is_btree $method] != 1 } {
		puts "Skipping rest of test089 for $method method."
		return
	}
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Skipping rest of test089 for specific pagesizes"
		return
	}

	append oargs " -dup "
	# Skip unsorted duplicates for btree with compression.
	if { [is_compressed $args] == 0 } {
		test089_dup $testdir $encargs $oargs $omethod $nentries
	}

	append oargs " -dupsort "
	test089_dup $testdir $encargs $oargs $omethod $nentries
}

proc test089_dup { testdir encargs oargs method nentries } {
	env_cleanup $testdir
	set pageargs ""
	split_pageargs $oargs pageargs
	set env [eval \
	     {berkdb_env -create -cdb} $encargs $pageargs -home $testdir]
	error_check_good dbenv [is_valid_env $env] TRUE

	#
	# Set pagesize small to generate lots of off-page dups
	#
	set page 512
	set nkeys 5
	set data "data"
	set key "test089_key"
	set testfile test089.db
	puts "\tTest089.d: CDB ($oargs) off-page dups"
	set oflags "-env $env -create -mode 0644 $oargs $method"
	set db [eval {berkdb_open} -pagesize $page $oflags $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	puts "\tTest089.e: Fill page with $nkeys keys, with $nentries dups"
	for { set k 0 } { $k < $nkeys } { incr k } {
		for { set i 0 } { $i < $nentries } { incr i } {
			set ret [$db put $key$k $i$data$k]
			error_check_good dbput $ret 0
		}
	}

	# Verify we have off-page duplicates
	set stat [$db stat]
	error_check_bad stat:offpage [is_substr $stat "{{Internal pages} 0}"] 1

	# This tests the failure reported in #6950.  Skip for -dupsort.
	puts "\tTest089.f: Clear locks for duped off-page dup cursors."
	if { [is_substr $oargs dupsort] != 1 } {
		# Create a read cursor, put it on an off-page dup.
		set dbcr [$db cursor]
		error_check_good dbcr [is_valid_cursor $dbcr $db] TRUE
		set offpage [$dbcr get -get_both test089_key4 900data4]
		error_check_bad offpage [llength $offpage] 0

		# Create a write cursor, put it on an off-page dup.
		set dbcw [$db cursor -update]
		error_check_good dbcw [is_valid_cursor $dbcw $db] TRUE
		set offpage [$dbcw get -get_both test089_key3 900data3]
		error_check_bad offpage [llength $offpage] 0

		# Add a new item using the write cursor, then close the cursor.
		error_check_good add_dup [$dbcw put -after $data] 0
		error_check_good close_dbcw [$dbcw close] 0

		# Get next dup with read cursor, then close the cursor.
		set nextdup [$dbcr get -nextdup]
		error_check_good close_dbcr [$dbcr close] 0
	}

	puts "\tTest089.g: CDB duplicate write cursors with off-page dups"
	# Create a write cursor and duplicate it.
	set dbcw [$db cursor -update]
	error_check_good dbcursor [is_valid_cursor $dbcw $db] TRUE
	set dup_dbcw [$dbcw dup]
	error_check_good dup_write_cursor [is_valid_cursor $dup_dbcw $db] TRUE

	# Position both cursors at get -first.  They should find the same data.
	set get_first [$dbcw get -first]
	set get_first_dup [$dup_dbcw get -first]
	error_check_good dup_read $get_first $get_first_dup

	# Test with -after and -before.  Skip for -dupsort.
	if { [is_substr $oargs dupsort] != 1 } {
		# Original and duplicate cursors both point to first item.
		# Do a put -before of new string with original cursor,
		# and a put -after of new string with duplicate cursor.
		set newdata "newdata"
		error_check_good put_before [$dbcw put -before $newdata] 0
		error_check_good put_after [$dup_dbcw put -after $newdata] 0

		# Now walk forward with original cursor ...
		set first [$dbcw get -first]
		error_check_good check_first [lindex [lindex $first 0] 1] $newdata
		set next1 [$dbcw get -next]
		error_check_good check_next1 $next1 $get_first
		set next2 [$dbcw get -next]
		error_check_good check_next2 [lindex [lindex $next2 0] 1] $newdata

		# ... and backward with duplicate cursor.
		set current [$dup_dbcw get -current]
		error_check_good check_current [lindex [lindex $current 0] 1] $newdata
		set prev1 [$dup_dbcw get -prev]
		error_check_good check_prev1 $prev1 $get_first
		set prev2 [$dup_dbcw get -prev]
		error_check_good check_prev2 [lindex [lindex $prev2 0] 1] $newdata
	}

	puts "\tTest089.h: test delete then get of off-page dups"
	for {set kd [$dbcw get -first] } { [llength $kd] != 0 } \
	    {set kd [$dbcw get -next] } {
		error_check_good dbcdel [$dbcw del] 0
	}

	error_check_good dbcw_close [$dbcw close] 0
	error_check_good dup_dbcw_close [$dup_dbcw close] 0

	error_check_good db_close [$db close] 0
	error_check_good env_close [$env close] 0
}
