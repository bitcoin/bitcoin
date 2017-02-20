# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	sdb017
# TEST	Test DB->rename() for in-memory named databases.
proc sdb017 { method args } {
	global errorCode
	source ./include.tcl

	if { [is_queueext $method] == 1 } {
		puts "Subdb017: Skipping for method $method"
		return
	}

	set omethod [convert_method $method]
	set args [convert_args $method $args]

	puts "Subdb017: $method ($args): DB->rename() for in-memory named dbs."

	# Skip test if given an env - this test needs its own.
	set eindex [lsearch -exact $args "-env"]
	if { $eindex != -1 } {
		incr eindex
		set env [lindex $args $eindex]
		puts "Subdb017 skipping for env $env"
		return
	}

	# In-memory dbs never go to disk, so we can't do checksumming.
	# If the test module sent in the -chksum arg, get rid of it.
	set chkindex [lsearch -exact $args "-chksum"]
	if { $chkindex != -1 } {
		set args [lreplace $args $chkindex $chkindex]
	}

	# Make sure we're starting from a clean slate.
	env_cleanup $testdir

        # Set up env.
        set env [berkdb_env_noerr -create -home $testdir -mode 0644]
	error_check_good env_open [is_valid_env $env] TRUE

	set oldsdb OLDDB
	set newsdb NEWDB

	puts "\tSubdb017.a: Create/rename file"
	puts "\t\tSubdb017.a.1: create"
	set testfile ""
	set db [eval {berkdb_open_noerr -create -mode 0644}\
	    $omethod -env $env $args {$testfile $oldsdb}]
	error_check_good dbopen [is_valid_db $db] TRUE

	# The nature of the key and data are unimportant; use numeric key
	# so record-based methods don't need special treatment.
	set key 1
	set data [pad_data $method data]

	error_check_good dbput [eval {$db put} $key $data] 0
	error_check_good dbclose [$db close] 0

	puts "\t\tSubdb017.a.2: rename"
	error_check_good rename_file [eval {berkdb dbrename} -env $env \
	    {$testfile $oldsdb $newsdb}] 0

	puts "\t\tSubdb017.a.3: check"
	# Open again with create to make sure we've really completely
	# disassociated the subdb from the old name.
	set odb [eval {berkdb_open_noerr -create -mode 0644}\
	    $omethod -env $env $args {$testfile $oldsdb}]
	error_check_good odb_open [is_valid_db $odb] TRUE
	set odbt [$odb get $key]
	error_check_good odb_close [$odb close] 0

	set ndb [eval {berkdb_open_noerr -mode 0644}\
	    $omethod -env $env $args {$testfile $newsdb}]
	error_check_good ndb_open [is_valid_db $ndb] TRUE
	set ndbt [$ndb get $key]
	error_check_good ndb_close [$ndb close] 0

	# The DBT from the "old" database should be empty, not the "new" one.
	error_check_good odbt_empty [llength $odbt] 0
	error_check_bad ndbt_empty [llength $ndbt] 0
	error_check_good ndbt [lindex [lindex $ndbt 0] 1] $data

	# Now there's both an old and a new.  Rename the "new" to the "old"
	# and make sure that fails.
	puts "\tSubdb017.b: Make sure rename fails instead of overwriting"
	set errorCode NONE
	set ret [catch {eval {berkdb dbrename} -env $env \
	    {$testfile $oldsdb $newsdb}} res]
	error_check_bad rename_overwrite $ret 0
	error_check_good rename_overwrite_ret [is_substr $errorCode EEXIST] 1

	error_check_good env_close [$env close] 0
}

