# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	sdb014
# TEST	Tests mixing in-memory named and in-memory unnamed dbs.
# TEST	Create a regular in-memory db, add data.
# TEST	Create a named in-memory db.
# TEST  Try to create the same named in-memory db again (should fail).
# TEST	Try to create a different named in-memory db (should succeed).
# TEST
proc sdb014 { method args } {
	source ./include.tcl

	set tnum "014"
	set orig_tdir $testdir
	if { [is_queueext $method] == 1 } {
		puts "Subdb$tnum: skipping for method $method"
		return
	}

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	# In-memory dbs never go to disk, so we can't do checksumming.
	# If the test module sent in the -chksum arg, get rid of it.
	set chkindex [lsearch -exact $args "-chksum"]
	if { $chkindex != -1 } {
		set args [lreplace $args $chkindex $chkindex]
	}

	puts "Subdb$tnum ($method $args):\
	    In-memory named dbs with regular in-mem dbs."

	# If we are given an env, use it.  Otherwise, open one.
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	if { $eindex == -1 } {
		env_cleanup $testdir
		set env [berkdb_env -create -home $testdir]
		error_check_good env_open [is_valid_env $env] TRUE
	} else {
		incr eindex
		set env [lindex $args $eindex]
		set envflags [$env get_open_flags]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set testdir [get_home $env]
	}

	puts "\tSubdb$tnum.a: Create and populate in-memory unnamed database."
	set testfile ""
	set db [eval {berkdb_open -env $env -create -mode 0644} \
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

	# Create named in-memory db.  Try to create a second in-memory db of
	# the same name.  Should fail.
	puts "\tSubdb$tnum.b: Create in-memory named database."
	set subdb "SUBDB"
	set db [eval {berkdb_open -env $env -create -excl -mode 0644} \
	    $args $omethod {$testfile $subdb}]
	error_check_good dbopen [is_valid_db $db] TRUE

	puts "\tSubdb$tnum.c: Try to create second inmem database."
	set ret [catch {eval {berkdb_open_noerr -env $env -create -excl \
	    -mode 0644} $args {$omethod $testfile $subdb}} db1]
	error_check_bad dbopen $ret 0

	# Clean up.  Close the env if this test created it.
	error_check_good db_close [$db close] 0
	if { $eindex == -1 } {
		error_check_good env_close [$env close] 0
	}

	set testdir $orig_tdir
	return
}

