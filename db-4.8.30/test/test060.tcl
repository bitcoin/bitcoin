# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test060
# TEST	Test of the DB_EXCL flag to DB->open().
# TEST	1) Attempt to open and create a nonexistent database; verify success.
# TEST	2) Attempt to reopen it;  verify failure.
proc test060 { method args } {
	global errorCode
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	puts "Test060: $method ($args) Test of the DB_EXCL flag to DB->open"

	# Set the database location and make sure the db doesn't exist yet
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test060.db
		set env NULL
	} else {
		set testfile test060.db
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set testdir [get_home $env]
	}
	cleanup $testdir $env

	# Create the database and check success
	puts "\tTest060.a: open and close non-existent file with DB_EXCL"
	set db [eval {berkdb_open \
	     -create  -excl -mode 0644} $args {$omethod $testfile}]
	error_check_good dbopen:excl [is_valid_db $db] TRUE

	# Close it and check success
	error_check_good db_close [$db close] 0

	# Try to open it again, and make sure the open fails
	puts "\tTest060.b: open it again with DB_EXCL and make sure it fails"
	set errorCode NONE
	error_check_good open:excl:catch [catch { \
	    set db [eval {berkdb_open_noerr \
	     -create  -excl -mode 0644} $args {$omethod $testfile}]
	    } ret ] 1

	error_check_good dbopen:excl [is_substr $errorCode EEXIST] 1
}
