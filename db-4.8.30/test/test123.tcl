# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test123
# TEST	Concurrent Data Store cdsgroup smoke test.
# TEST
# TEST	Open a CDS env with -cdb_alldb.
# TEST	Start a "txn" with -cdsgroup.
# TEST	Create two databases in the env, do a cursor put 
# TEST	in both within the same txn.  This should succeed.  

proc test123 { method args } {
	source ./include.tcl

	# If we are using an env, then skip this test.  It needs its own.
	set eindex [lsearch -exact $args "-env"]
	if { $eindex != -1 } {
		incr eindex
		set env [lindex $args $eindex]
		puts "Skipping test123 for env $env"
		return
	}

	if { [is_queue $method] == 1 } {
		puts "Skipping test123 for method $method"
		return
	}
	if { [is_partitioned $args] == 1 } {
		puts "Test123 skipping for partitioned $method"
		return
	}
	set args [convert_args $method $args]
	set encargs ""
	set args [split_encargs $args encargs]
	set omethod [convert_method $method]
	set pageargs ""
	split_pageargs $args pageargs
	set dbname test123.db
	set tnum "123"

	puts "Test$tnum: CDB with cdsgroup ($method)"
	env_cleanup $testdir

	# Open environment and start cdsgroup "transaction".
	puts "\tTest$tnum.a: Open env."
	set env [eval {berkdb_env -create} \
	     $pageargs $encargs -cdb -cdb_alldb -home $testdir]
	error_check_good dbenv [is_valid_env $env] TRUE
	set txn [$env cdsgroup]

	# Env is created, now set up 2 databases
	puts "\tTest$tnum.b: Open first database."
	set db1 [eval {berkdb_open}\
	    -create -env $env $args $omethod -txn $txn $dbname "A"] 
	puts "\tTest$tnum.b1: Open cursor."
	set curs1 [eval {$db1 cursor} -update -txn $txn]
	puts "\tTest$tnum.b2: Initialize cursor and do a put."
	error_check_good curs1_put [eval {$curs1 put} -keyfirst 1 DATA1] 0

	puts "\tTest$tnum.c: Open second database."
	set db2 [eval {berkdb_open}\
	    -create -env $env $args $omethod -txn $txn $dbname "B"]
	puts "\tTest$tnum.c1: Open cursor."
	set curs2 [eval {$db2 cursor} -update -txn $txn]
	puts "\tTest$tnum.b2: Initialize cursor and do a put."
	error_check_good curs2_put [eval {$curs2 put} -keyfirst 2 DATA2] 0

	# Clean up. 
	$curs2 close
	$curs1 close
	$txn commit 
	$db2 close
	$db1 close
	$env close

}

