# See the file LICENSE for redistribution information.
#
# Copyright (c) 2005-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	plat001
# TEST
# TEST	Test of portability of sequences.
# TEST
# TEST	Create and dump a database containing sequences.  Save the dump.
# TEST	This test is used in conjunction with the upgrade tests, which
# TEST	will compare the saved dump to a locally created dump.

proc plat001 { method {tnum "001"} args } {
	source ./include.tcl
	global fixed_len
	global util_path

	# Fixed_len must be increased from the default to
	# accommodate fixed-record length methods.
	set orig_fixed_len $fixed_len
	set fixed_len 128
	set args [convert_args $method $args]
	set omethod [convert_method $method]

	set eindex [lsearch -exact $args "-env"]
	set txnenv 0
	if { $eindex == -1 } {
		set testfile $testdir/plat$tnum.db
		set testdump $testdir/plat$tnum.dmp
		set env NULL
	} else {
		set testfile plat$tnum.db
		set testdump plat$tnum.dmp
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		set rpcenv [is_rpcenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set testdir [get_home $env]
	}

	cleanup $testdir $env

	# Make the key numeric so we can test record-based methods.
	set key 1

	puts "\tPlat$tnum.a: Create $method db with a sequence."
	set db [eval {berkdb_open -create -mode 0644} $args $omethod $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	set init 1
	set min $init
	set max 1000000000
	set seq [eval {berkdb sequence} \
	    -create -init $init -min $min -max $max $db $key]
	error_check_good is_valid_seq [is_valid_seq $seq] TRUE

	error_check_good seq_close [$seq close] 0
	error_check_good db_close [$db close] 0

	puts "\tPlat$tnum.b: Dump the db."
	set stat [catch {eval {exec $util_path/db_dump} -f $testdump \
	    $testfile} ret]
	error_check_good sequence_dump $stat 0

	puts "\tPlat$tnum.c: Delete the db."
	error_check_good db_delete [fileremove $testfile] ""

	set fixed_len $orig_fixed_len
	return
}
