# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	sdb001	Tests mixing db and subdb operations
# TEST	Tests mixing db and subdb operations
# TEST	Create a db, add data, try to create a subdb.
# TEST	Test naming db and subdb with a leading - for correct parsing
# TEST	Existence check -- test use of -excl with subdbs
# TEST
# TEST	Test non-subdb and subdb operations
# TEST	Test naming (filenames begin with -)
# TEST	Test existence (cannot create subdb of same name with -excl)
proc sdb001 { method args } {
	source ./include.tcl
	global errorInfo

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	if { [is_queue $method] == 1 } {
		puts "Subdb001: skipping for method $method"
		return
	}
	puts "Subdb001: $method ($args) subdb and non-subdb tests"

	set testfile $testdir/subdb001.db
	set eindex [lsearch -exact $args "-env"]
	if { $eindex != -1 } {
		set env NULL
		incr eindex
		set env [lindex $args $eindex]
		puts "Subdb001 skipping for env $env"
		return
	}
	# Create the database and open the dictionary
	set subdb subdb0
	cleanup $testdir NULL
	puts "\tSubdb001.a: Non-subdb database and subdb operations"
	#
	# Create a db with no subdbs.  Add some data.  Close.  Try to
	# open/add with a subdb.  Should fail.
	#
	puts "\tSubdb001.a.0: Create db, add data, close, try subdb"
	set db [eval {berkdb_open -create -mode 0644} \
	    $args {$omethod $testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE

	set did [open $dict]

	set pflags ""
	set gflags ""
	set count 0

	if { [is_record_based $method] == 1 } {
		append gflags " -recno"
	}
	while { [gets $did str] != -1 && $count < 5 } {
		if { [is_record_based $method] == 1 } {
			global kvals

			set key [expr $count + 1]
			set kvals($key) $str
		} else {
			set key $str
		}
		set ret [eval \
		    {$db put} $pflags {$key [chop_data $method $str]}]
		error_check_good put $ret 0

		set ret [eval {$db get} $gflags {$key}]
		error_check_good \
		    get $ret [list [list $key [pad_data $method $str]]]
		incr count
	}
	close $did
	error_check_good db_close [$db close] 0
	set ret [catch {eval {berkdb_open_noerr -create -mode 0644} $args \
	    {$omethod $testfile $subdb}} db]
	error_check_bad dbopen $ret 0
	#
	# Create a db with no subdbs.  Add no data.  Close.  Try to
	# open/add with a subdb.  Should fail.
	#
	set testfile $testdir/subdb001a.db
	puts "\tSubdb001.a.1: Create db, close, try subdb"
	#
	# !!!
	# Using -truncate is illegal when opening for subdbs, but we
	# can use it here because we are not using subdbs for this
	# create.
	#
	set db [eval {berkdb_open -create -truncate -mode 0644} $args \
	    {$omethod $testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE
	error_check_good db_close [$db close] 0

	set ret [catch {eval {berkdb_open_noerr -create -mode 0644} $args \
	    {$omethod $testfile $subdb}} db]
	error_check_bad dbopen $ret 0

	if { [is_queue $method] == 1 || [is_partitioned $args]} {
		puts "Subdb001: skipping remainder of test for method $method $args"
		return
	}

	#
	# Test naming, db and subdb names beginning with -.
	#
	puts "\tSubdb001.b: Naming"
	set cwd [pwd]
	cd $testdir
	set testfile1 -subdb001.db
	set subdb -subdb
	puts "\tSubdb001.b.0: Create db and subdb with -name, no --"
	set ret [catch {eval {berkdb_open -create -mode 0644} $args \
	    {$omethod $testfile1 $subdb}} db]
	error_check_bad dbopen $ret 0
	puts "\tSubdb001.b.1: Create db and subdb with -name, with --"
	set db [eval {berkdb_open -create -mode 0644} $args \
	    {$omethod -- $testfile1 $subdb}]
	error_check_good dbopen [is_valid_db $db] TRUE
	error_check_good db_close [$db close] 0

	cd $cwd

	#
	# Create 1 db with 1 subdb.  Try to create another subdb of
	# the same name.  Should fail.
	#

	puts "\tSubdb001.c: Existence check"
	set testfile $testdir/subdb001d.db
	set subdb subdb
	set ret [catch {eval {berkdb_open -create -excl -mode 0644} $args \
	    {$omethod $testfile $subdb}} db]
	error_check_good dbopen [is_valid_db $db] TRUE
	set ret [catch {eval {berkdb_open_noerr -create -excl -mode 0644} \
	    $args {$omethod $testfile $subdb}} db1]
	error_check_bad dbopen $ret 0
	error_check_good db_close [$db close] 0

	return
}
