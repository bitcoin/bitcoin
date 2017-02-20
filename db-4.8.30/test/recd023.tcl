# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	recd023
# TEST	Test recover of reverse split.
#
proc recd023 { method args } {
	source ./include.tcl
	env_cleanup $testdir
	set tnum "023"

	if { [is_btree $method] != 1 && [is_rbtree $method] != 1 } {
		puts "Skipping recd$tnum for method $method"
		return
	}

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	if { [is_partition_callback $args] == 1 } {
		set nodump 1
	} else {
		set nodump 0
	}

	puts "Recd$tnum ($omethod $args): Recovery of reverse split."
	set testfile recd$tnum.db

	puts "\tRecd$tnum.a: Create environment and database."
	set flags "-create -txn -home $testdir"

	set env_cmd "berkdb_env $flags"
	set env [eval $env_cmd]
	error_check_good env [is_valid_env $env] TRUE

	set pagesize 512
	set oflags "$omethod -auto_commit \
	    -pagesize $pagesize -create -mode 0644 $args"
	set db [eval {berkdb_open} -env $env $oflags $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	# Write to database -- enough to fill at least 3 levels.
	puts "\tRecd$tnum.b: Create a 3 level btree database."
	set nentries 1000
	set datastr [repeat x 45]
	for { set i 1 } { $i < $nentries } { incr i } {
		set key a$i
		set ret [$db put $key [chop_data $method $datastr]]
		error_check_good put $ret 0
	}

	# Verify we have enough levels.
	set levels [stat_field $db stat "Levels"]
	error_check_good 3_levels [expr $levels >= 3] 1

	# Save the original database.
	file copy -force $testdir/$testfile $testdir/$testfile.save

	# Delete enough pieces to collapse the tree.
	puts "\tRecd$tnum.c: Do deletes to collapse database."
	for { set count 2 } { $count < 10 } { incr count } {
		error_check_good db_del [$db del a$count] 0
	}
	for { set count 15 } { $count < 100 } { incr count } {
		error_check_good db_del [$db del a$count] 0
	}
	for { set count 150 } { $count < 1000 } { incr count } {
		error_check_good db_del [$db del a$count] 0
	}

	error_check_good db_close [$db close] 0
	error_check_good verify_dir\
	    [verify_dir $testdir "\tRecd$tnum.d: " 0 0 $nodump] 0

	# Overwrite the current database with the saved database.
	file copy -force $testdir/$testfile.save $testdir/$testfile
	error_check_good log_flush [$env log_flush] 0
	error_check_good env_close [$env close] 0

	# Recover the saved database to roll forward and apply the deletes.
	set env [berkdb_env -create -txn -home $testdir -recover]
	error_check_good env_open [is_valid_env $env] TRUE
	error_check_good log_flush [$env log_flush] 0
	error_check_good env_close [$env close] 0

	error_check_good verify_dir\
	     [verify_dir $testdir "\tRecd$tnum.e: " 0 0 $nodump] 0
}
