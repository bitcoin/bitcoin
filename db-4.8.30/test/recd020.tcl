# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	recd020
# TEST	Test creation of intermediate directories -- an
# TEST	undocumented, UNIX-only feature.
#
proc recd020 { method args } {
	source ./include.tcl
	global tcl_platform

	set args [convert_args $method $args]
	set omethod [convert_method $method]
	set tnum "020"
	set nentries 10

	if { $tcl_platform(platform) != "unix" } {
		puts "Skipping recd$tnum for non-UNIX platform."
		return
	}

	puts "Recd$tnum ($method):\
	    Test creation of intermediate directories in recovery."

	# Create the original intermediate directory.
	env_cleanup $testdir
	set intdir INTDIR
	file mkdir $testdir/$intdir

	set testfile recd$tnum.db
	set flags "-create -txn -home $testdir"

	puts "\tRecd$tnum.a: Create environment and populate database."
	set env_cmd "berkdb_env $flags"
	set env [eval $env_cmd]
	error_check_good env [is_valid_env $env] TRUE

	set db [eval berkdb_open \
	    -create $omethod $args -env $env -auto_commit $intdir/$testfile]
	error_check_good db_open [is_valid_db $db] TRUE

	set txn [$env txn]
	set data "data"
	for { set i 1 } { $i <= $nentries } { incr i } {
		error_check_good db_put [eval \
		    {$db put} -txn $txn $i [chop_data $method $data.$i]] 0
	}
	error_check_good txn_commit [$txn commit] 0
	error_check_good db_close [$db close] 0
	error_check_good log_flush [$env log_flush] 0
	error_check_good env_close [$env close] 0

	puts "\tRecd$tnum.b: Remove intermediate directory."
	error_check_good directory_there [file exists $testdir/$intdir] 1
	file delete -force $testdir/$intdir
	error_check_good directory_gone [file exists $testdir/$intdir] 0

	puts "\tRecd020.c: Run recovery, recreating intermediate directory."
	set env [eval $env_cmd -set_intermediate_dir_mode "rwxr-x--x" -recover]
	error_check_good env [is_valid_env $env] TRUE

	puts "\tRecd020.d: Reopen test file to verify success."
	set db [eval {berkdb_open} -env $env $args $intdir/$testfile]
	error_check_good db_open [is_valid_db $db] TRUE
	for { set i 1 } { $i <= $nentries } { incr i } {
		set ret [$db get $i]
		set k [lindex [lindex $ret 0] 0]
		set d [lindex [lindex $ret 0] 1]
		error_check_good key $k $i
		error_check_good data $d [pad_data $method $data.$i]
	}

	# Clean up.
	error_check_good db_close [$db close] 0
	error_check_good log_flush [$env log_flush] 0
	error_check_good env_close [$env close] 0

}
