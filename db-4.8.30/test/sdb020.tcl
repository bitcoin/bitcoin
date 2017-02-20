# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	sdb020
# TEST	Tests in-memory subdatabases.
# TEST	Create an in-memory subdb with one page size.  Close, and
# TEST	open with a different page size: should fail.

proc sdb020 { method { nentries 10 } args } {
	source ./include.tcl
	global errorCode

	set tnum "020"
	set args [convert_args $method $args]
	set omethod [convert_method $method]

	if { [is_queueext $method] == 1 } {
		puts "Subdb$tnum: skipping for method $method"
		return
	}

	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Subdb$tnum: skipping for specific page sizes."
		return
	}

	# If we are using an env, then skip this test.  It needs its own.
	set eindex [lsearch -exact $args "-env"]
	if { $eindex != -1 } {
		set env NULL
		incr eindex
		set env [lindex $args $eindex]
		puts "Subdb020 skipping for env $env"
		return
	}

	# In-memory dbs never go to disk, so we can't do checksumming.
	# If the test module sent in the -chksum arg, get rid of it.
	set chkindex [lsearch -exact $args "-chksum"]
	if { $chkindex != -1 } {
		set args [lreplace $args $chkindex $chkindex]
	}

	puts "Subdb$tnum: $method ($args) \
	    in-memory named db tests with different pagesizes"

	# Create the env.
	env_cleanup $testdir
	set env [berkdb_env_noerr -create -home $testdir -txn]
	error_check_good dbenv [is_valid_env $env] TRUE

	# Set filename to NULL; this causes the creation of an in-memory
	# subdb.
	set testfile ""
	set name NAME

	puts "\tSubdb$tnum.a: Create in-mem named db with default page size."
	set db [eval {berkdb_open_noerr -create -mode 0644} \
	    $args -env $env {$omethod $testfile $name}]
	error_check_good dbopen [is_valid_db $db] TRUE

	# Figure out the default page size so we can try to open
	# later with a different value.
	set psize [stat_field $db stat "Page size"]
	if { $psize == 512 } {
		set psize2 2048
	} else {
		set psize2 512
	}

	error_check_good db_close [$db close] 0

	# Try to open again with a different page size (should fail).
	puts "\tSubdb$tnum.b: Try to reopen with different page size."
	set errorCode NONE
	catch {set db [eval {berkdb_open_noerr} $args -env $env \
	    -pagesize $psize2 {$omethod $testfile $name}]} res
	error_check_good expect_error [is_substr $errorCode EINVAL] 1

	# Try to open again with the correct pagesize (should succeed).
	puts "\tSubdb$tnum.c: Reopen with original page size."
	set db [eval {berkdb_open_noerr} $args -env $env \
	    -pagesize $psize {$omethod $testfile $name}]
	# Close DB
	error_check_good db_close [$db close] 0

	puts "\tSubdb$tnum.d: Create in-mem named db with specific page size."
	set psize 8192
	set db [eval {berkdb_open_noerr -create -mode 0644} \
	    $args -env $env -pagesize $psize {$omethod $testfile $name}]
	error_check_good dbopen [is_valid_db $db] TRUE
	error_check_good db_close [$db close] 0

	# Try to open again with a different page size (should fail).
	set psize2 [expr $psize / 2]
	puts "\tSubdb$tnum.e: Try to reopen with different page size."
	set errorCode NONE
	catch {set db [eval {berkdb_open_noerr} $args -env $env \
	    -pagesize $psize2 {$omethod $testfile $name}]} res
	error_check_good expect_error [is_substr $errorCode EINVAL] 1

	# Try to open again with the correct pagesize (should succeed).
	puts "\tSubdb$tnum.f: Reopen with original page size."
	set db [eval {berkdb_open} $args -env $env \
	    -pagesize $psize {$omethod $testfile $name}]

	# Try to open a different database with a different page size
	# (should succeed).
	puts "\tSubdb$tnum.g: Open different db with different page size."
	set newname NEWNAME
	set db2 [eval {berkdb_open} -create $args -env $env \
	    -pagesize $psize2 {$omethod $testfile $newname}]

	# Clean up.
	error_check_good db_close [$db close] 0
	error_check_good db2_close [$db2 close] 0
	error_check_good env_close [$env close] 0
}


