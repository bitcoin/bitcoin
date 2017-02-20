# See the file LICENSE for redistribution information.
#
# Copyright (c) 2000-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	sdb009
# TEST	Test DB->rename() method for subdbs
proc sdb009 { method args } {
	global errorCode
	source ./include.tcl

	set omethod [convert_method $method]
	set args [convert_args $method $args]

	puts "Subdb009: $method ($args): Test of DB->rename()"

	if { [is_queue $method] == 1 } {
		puts "\tSubdb009: Skipping for method $method."
		return
	}

	set txnenv 0
	set envargs ""
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/subdb009.db
		set env NULL
	} else {
		set testfile subdb009.db
		incr eindex
		set env [lindex $args $eindex]
		set envargs " -env $env "
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
			append envargs " -auto_commit "
		}
		set testdir [get_home $env]
	}
	set oldsdb OLDDB
	set newsdb NEWDB

	# Make sure we're starting from a clean slate.
	cleanup $testdir $env
	error_check_bad "$testfile exists" [file exists $testfile] 1

	puts "\tSubdb009.a: Create/rename file"
	puts "\t\tSubdb009.a.1: create"
	set db [eval {berkdb_open -create -mode 0644}\
	    $omethod $args {$testfile $oldsdb}]
	error_check_good dbopen [is_valid_db $db] TRUE

	# The nature of the key and data are unimportant; use numeric key
	# so record-based methods don't need special treatment.
	set txn ""
	set key 1
	set data [pad_data $method data]

	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	error_check_good dbput [eval {$db put} $txn {$key $data}] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good dbclose [$db close] 0

	puts "\t\tSubdb009.a.2: rename"
	error_check_good rename_file [eval {berkdb dbrename} $envargs \
	    {$testfile $oldsdb $newsdb}] 0

	puts "\t\tSubdb009.a.3: check"
	# Open again with create to make sure we've really completely
	# disassociated the subdb from the old name.
	set odb [eval {berkdb_open -create -mode 0644}\
	    $omethod $args $testfile $oldsdb]
	error_check_good odb_open [is_valid_db $odb] TRUE
	set odbt [$odb get $key]
	error_check_good odb_close [$odb close] 0

	set ndb [eval {berkdb_open -create -mode 0644}\
	    $omethod $args $testfile $newsdb]
	error_check_good ndb_open [is_valid_db $ndb] TRUE
	set ndbt [$ndb get $key]
	error_check_good ndb_close [$ndb close] 0

	# The DBT from the "old" database should be empty, not the "new" one.
	error_check_good odbt_empty [llength $odbt] 0
	error_check_bad ndbt_empty [llength $ndbt] 0
	error_check_good ndbt [lindex [lindex $ndbt 0] 1] $data

	# Now there's both an old and a new.  Rename the "new" to the "old"
	# and make sure that fails.
	puts "\tSubdb009.b: Make sure rename fails instead of overwriting"
	set ret [catch {eval {berkdb dbrename} $envargs $testfile \
	    $oldsdb $newsdb} res]
	error_check_bad rename_overwrite $ret 0
	error_check_good rename_overwrite_ret [is_substr $errorCode EEXIST] 1

	puts "\tSubdb009 succeeded."
}
