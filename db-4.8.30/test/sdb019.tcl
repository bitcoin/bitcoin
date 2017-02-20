# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	sdb019
# TEST	Tests in-memory subdatabases.
# TEST	Create an in-memory subdb.  Test for persistence after
# TEST	overflowing the cache.  Test for conflicts when we have
# TEST	two in-memory files.

proc sdb019 { method { nentries 100 } args } {
	source ./include.tcl

	set tnum "019"
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
		puts "Subdb019 skipping for env $env"
		return
	}

	# In-memory dbs never go to disk, so we can't do checksumming.
	# If the test module sent in the -chksum arg, get rid of it.
	set chkindex [lsearch -exact $args "-chksum"]
	if { $chkindex != -1 } {
		set args [lreplace $args $chkindex $chkindex]
	}

	# The standard cachesize isn't big enough for 64k pages.
	set csize "0 262144 1"
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		incr pgindex
		set pagesize [lindex $args $pgindex]
		if { $pagesize > 16384 } {
			set cache [expr 8 * $pagesize]
			set csize "0 $cache 1"
		}
	}

	# Create the env.
	env_cleanup $testdir
	set env [eval berkdb_env -create {-cachesize $csize} \
	    -home $testdir -txn]
	error_check_good dbenv [is_valid_env $env] TRUE

	# Set filename to NULL; this allows us to create an in-memory
	# named database.
	set testfile ""

	# Create two in-memory subdb and test for conflicts.  Try all the
	# combinations of named (NULL/NAME) and purely temporary
	# (NULL/NULL) databases.
	#
	foreach s1 { S1 "" } {
		foreach s2 { S2 "" } {
			puts "\tSubdb$tnum.a:\
			    2 in-memory subdbs (NULL/$s1, NULL/$s2)."
			set sdb1 [eval {berkdb_open -create -mode 0644} \
			    $args -env $env {$omethod $testfile $s1}]
			error_check_good sdb1_open [is_valid_db $sdb1] TRUE
			set sdb2 [eval {berkdb_open -create -mode 0644} \
			    $args -env $env {$omethod $testfile $s2}]
			error_check_good sdb1_open [is_valid_db $sdb2] TRUE

			# Subdatabases are open, now put something in.
			set string1 STRING1
			set string2 STRING2
			for { set i 1 } { $i <= $nentries } { incr i } {
				set key $i
				error_check_good sdb1_put [$sdb1 put $key \
				    [chop_data $method $string1.$key]] 0
				error_check_good sdb2_put [$sdb2 put $key \
				    [chop_data $method $string2.$key]] 0
			}

			# If the subs are both NULL/NULL, we have two handles
			# on the same db.  Skip testing the contents.
			if { $s1 != "" || $s2 != "" } {
				# This can't work when both subs are NULL/NULL.
				# Check contents.
				for { set i 1 } { $i <= $nentries } { incr i } {
					set key $i
					set ret1 [lindex \
					    [lindex [$sdb1 get $key] 0] 1]
					error_check_good sdb1_get $ret1 \
					    [pad_data $method $string1.$key]
					set ret2 [lindex \
					    [lindex [$sdb2 get $key] 0] 1]
					error_check_good sdb2_get $ret2 \
					    [pad_data $method $string2.$key]
				}

				error_check_good sdb1_close [$sdb1 close] 0
				error_check_good sdb2_close [$sdb2 close] 0

				# Reopen, make sure we get the right data.
				set sdb1 [eval {berkdb_open -mode 0644} \
				    $args -env $env {$omethod $testfile $s1}]
				error_check_good \
				    sdb1_open [is_valid_db $sdb1] TRUE
				set sdb2 [eval {berkdb_open -mode 0644} \
				    $args -env $env {$omethod $testfile $s2}]
				error_check_good \
				    sdb1_open [is_valid_db $sdb2] TRUE

				for { set i 1 } { $i <= $nentries } { incr i } {
					set key $i
					set ret1 [lindex \
					    [lindex [$sdb1 get $key] 0] 1]
					error_check_good sdb1_get $ret1 \
					    [pad_data $method $string1.$key]
					set ret2 [lindex \
					    [lindex [$sdb2 get $key] 0] 1]
					error_check_good sdb2_get $ret2 \
					    [pad_data $method $string2.$key]
				}
			}
			error_check_good sdb1_close [$sdb1 close] 0
			error_check_good sdb2_close [$sdb2 close] 0
		}
	}
	error_check_good env_close [$env close] 0
}

