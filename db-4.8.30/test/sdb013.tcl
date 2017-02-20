# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	sdb013
# TEST	Tests in-memory subdatabases.
# TEST	Create an in-memory subdb.  Test for persistence after
# TEST	overflowing the cache.  Test for conflicts when we have
# TEST	two in-memory files.

proc sdb013 { method { nentries 10 } args } {
	source ./include.tcl

	set tnum "013"
	set args [convert_args $method $args]
	set omethod [convert_method $method]

	if { [is_queueext $method] == 1 } {
		puts "Subdb$tnum: skipping for method $method"
		return
	}

	puts "Subdb$tnum: $method ($args) in-memory subdb tests"

	# If we are using an env, then skip this test.  It needs its own.
	set eindex [lsearch -exact $args "-env"]
	if { $eindex != -1 } {
		set env NULL
		incr eindex
		set env [lindex $args $eindex]
		puts "Subdb$tnum skipping for env $env"
		return
	}

	# In-memory dbs never go to disk, so we can't do checksumming.
	# If the test module sent in the -chksum arg, get rid of it.
	set chkindex [lsearch -exact $args "-chksum"]
	if { $chkindex != -1 } {
		set args [lreplace $args $chkindex $chkindex]
	}

	# Create the env, with a very small cache that we can easily
	# fill.  If a particularly large page size is specified, make
	# the cache a little larger, but still on the small side.
	env_cleanup $testdir
	set csize {0 65536 1}
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		incr pgindex
		set pagesize [lindex $args $pgindex]
		if { $pagesize > 8192 } {
			set cache [expr 4 * $pagesize]
			set csize "0 $cache 1"
		}
	}

	set env [berkdb_env_noerr -create -cachesize $csize -home $testdir -txn]
	error_check_good dbenv [is_valid_env $env] TRUE

	# Set filename to NULL; this causes the creation of an in-memory
	# subdb.
	set testfile ""
	set subdb subdb0

	puts "\tSubdb$tnum.a: Create in-mem subdb, add data, close."
	set sdb [eval {berkdb_open_noerr -create -mode 0644} \
	    $args -env $env -auto_commit {$omethod $testfile $subdb}]
	error_check_good dbopen [is_valid_db $sdb] TRUE

	set ret [sdb013_populate $sdb $method $nentries]
	error_check_good populate $ret 0
	error_check_good sdb_close [$sdb close] 0

	# Do a bunch of writing to evict all pages from the memory pool.
	puts "\tSubdb$tnum.b: Create another db, overflow the cache."
	set dummyfile foo.db
	set db [eval {berkdb_open_noerr -create -mode 0644} $args -env $env\
	    -auto_commit $omethod $dummyfile]
	error_check_good dummy_open [is_valid_db $db] TRUE

	set entries [expr $nentries * 100]
	set ret [sdb013_populate $db $method $entries]
	error_check_good dummy_close [$db close] 0

	# Make sure we can still open the in-memory subdb.
	puts "\tSubdb$tnum.c: Check we can still open the in-mem subdb."
	set sdb [eval {berkdb_open_noerr} \
	    $args -env $env -auto_commit {$omethod $testfile $subdb}]
	error_check_good sdb_reopen [is_valid_db $sdb] TRUE
	error_check_good sdb_close [$sdb close] 0

	# Exercise the -m (dump in-memory) option on db_dump.
	puts "\tSubdb$tnum.d: Exercise in-memory db_dump."
	set stat \
	    [catch {eval {exec $util_path/db_dump} -h $testdir -m $subdb} res]
	error_check_good dump_successful $stat 0

	puts "\tSubdb$tnum.e: Remove in-mem subdb."
	error_check_good \
	    sdb_remove [berkdb dbremove -env $env $testfile $subdb] 0

	puts "\tSubdb$tnum.f: Check we cannot open the in-mem subdb."
	set ret [catch {eval {berkdb_open_noerr} -env $env $args \
	    -auto_commit {$omethod $testfile $subdb}} db]
	error_check_bad dbopen $ret 0

	foreach end { commit abort } {
		# Create an in-memory database.
		puts "\tSubdb$tnum.g: Create in-mem subdb, add data, close."
		set sdb [eval {berkdb_open_noerr -create -mode 0644} \
		    $args -env $env -auto_commit {$omethod $testfile $subdb}]
		error_check_good dbopen [is_valid_db $sdb] TRUE

		set ret [sdb013_populate $sdb $method $nentries]
		error_check_good populate $ret 0
		error_check_good sdb_close [$sdb close] 0

		# Transactionally remove the database.
		puts "\tSubdb$tnum.h: Transactionally remove in-mem database."
		set txn [$env txn]
		error_check_good db_remove \
		    [berkdb dbremove -env $env -txn $txn $testfile $subdb] 0

		# Write a cacheful of data.
		puts "\tSubdb$tnum.i: Create another db, overflow the cache."
		set db [eval {berkdb_open_noerr -create -mode 0644} $args \
		    -env $env -auto_commit $omethod $dummyfile]
		error_check_good dummy_open [is_valid_db $db] TRUE

		set entries [expr $nentries * 100]
		set ret [sdb013_populate $db $method $entries]
		error_check_good dummy_close [$db close] 0

		# Finish the txn and make sure the database is either
		# gone (if committed) or still there (if aborted).
		error_check_good txn_$end [$txn $end] 0
		if { $end == "abort" } {
			puts "\tSubdb$tnum.j: Check that database still exists."
			set sdb [eval {berkdb_open_noerr} $args \
			    -env $env -auto_commit {$omethod $testfile $subdb}]
			error_check_good sdb_reopen [is_valid_db $sdb] TRUE
			error_check_good sdb_close [$sdb close] 0
		} else {
			puts "\tSubdb$tnum.j: Check that database is gone."
			set ret [catch {eval {berkdb_open_noerr} -env $env \
			    $args -auto_commit {$omethod $testfile $subdb}} res]
			error_check_bad dbopen $ret 0
		}
	}

	error_check_good env_close [$env close] 0
}

proc sdb013_populate { db method nentries } {
	source ./include.tcl

	set did [open $dict]
	set count 0
	while { [gets $did str] != -1 && $count < $nentries } {
		if { [is_record_based $method] == 1 } {
			set key [expr $count + 1]
		} else {
			set key $str
		}

		set r [ catch {$db put $key [chop_data $method $str]} ret ]
		if { $r != 0 } {
			close $did
			return $ret
		}

 		incr count
	}
	close $did
	return 0
}

