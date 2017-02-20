# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	memp003
# TEST	Test reader-only/writer process combinations; we use the access methods
# TEST	for testing.
proc memp003 { } {
	source ./include.tcl
	global rand_init
	error_check_good set_random_seed [berkdb srand $rand_init] 0
	#
	# Multiple processes not supported by private memory so don't
	# run memp003_body with -private.
	#
	memp003_body ""
	if { $is_qnx_test } {
		puts "Skipping remainder of memp003 for\
		    environments in system memory on QNX"
		return
	}
	memp003_body "-system_mem -shm_key 1"
}

proc memp003_body { flags } {
	global alphabet
	source ./include.tcl

	puts "Memp003: {$flags} Reader/Writer tests"

	if { [mem_chk $flags] == 1 } {
		return
	}

	env_cleanup $testdir
	set psize 1024
	set nentries 500
	set testfile mpool.db
	set t1 $testdir/t1

	# Create an environment that the two processes can share, with
	# 20 pages per cache.
	set c [list 0 [expr $psize * 20 * 3] 3]
	set dbenv [eval {berkdb_env \
	    -create -lock -home $testdir -cachesize $c} $flags]
	error_check_good dbenv [is_valid_env $dbenv] TRUE

	# First open and create the file.
	set db [berkdb_open -env $dbenv -create \
	    -mode 0644 -pagesize $psize -btree $testfile]
	error_check_good dbopen/RW [is_valid_db $db] TRUE

	set did [open $dict]
	set txn ""
	set count 0

	puts "\tMemp003.a: create database"
	set keys ""
	# Here is the loop where we put and get each key/data pair
	while { [gets $did str] != -1 && $count < $nentries } {
		lappend keys $str

		set ret [eval {$db put} $txn {$str $str}]
		error_check_good put $ret 0

		set ret [eval {$db get} $txn {$str}]
		error_check_good get $ret [list [list $str $str]]

		incr count
	}
	close $did
	error_check_good close [$db close] 0

	# Now open the file for read-only
	set db [berkdb_open -env $dbenv -rdonly $testfile]
	error_check_good dbopen/RO [is_substr $db db] 1

	puts "\tMemp003.b: verify a few keys"
	# Read and verify a couple of keys; saving them to check later
	set testset ""
	for { set i 0 } { $i < 10 } { incr i } {
		set ndx [berkdb random_int 0 [expr $nentries - 1]]
		set key [lindex $keys $ndx]
		if { [lsearch $testset $key] != -1 } {
			incr i -1
			continue;
		}

		# The remote process stuff is unhappy with
		# zero-length keys;  make sure we don't pick one.
		if { [llength $key] == 0 } {
			incr i -1
			continue
		}

		lappend testset $key

		set ret [eval {$db get} $txn {$key}]
		error_check_good get/RO $ret [list [list $key $key]]
	}

	puts "\tMemp003.c: retrieve and modify keys in remote process"
	# Now open remote process where we will open the file RW
	set f1 [open |$tclsh_path r+]
	puts $f1 "source $test_path/test.tcl"
	puts $f1 "flush stdout"
	flush $f1

	set c [concat "{" [list 0 [expr $psize * 20 * 3] 3] "}" ]
	set remote_env [send_cmd $f1 \
	    "berkdb_env -create -lock -home $testdir -cachesize $c $flags"]
	error_check_good remote_dbenv [is_valid_env $remote_env] TRUE

	set remote_db [send_cmd $f1 "berkdb_open -env $remote_env $testfile"]
	error_check_good remote_dbopen [is_valid_db $remote_db] TRUE

	foreach k $testset {
		# Get the key
		set ret [send_cmd $f1 "$remote_db get $k"]
		error_check_good remote_get $ret [list [list $k $k]]

		# Now replace the key
		set ret [send_cmd $f1 "$remote_db put $k $k$k"]
		error_check_good remote_put $ret 0
	}

	puts "\tMemp003.d: verify changes in local process"
	foreach k $testset {
		set ret [eval {$db get} $txn {$key}]
		error_check_good get_verify/RO $ret [list [list $key $key$key]]
	}

	puts "\tMemp003.e: Fill up the cache with dirty buffers"
	foreach k $testset {
		# Now rewrite the keys with BIG data
		set data [replicate $alphabet 32]
		set ret [send_cmd $f1 "$remote_db put $k $data"]
		error_check_good remote_put $ret 0
	}

	puts "\tMemp003.f: Get more pages for the read-only file"
	dump_file $db $txn $t1 nop

	puts "\tMemp003.g: Sync from the read-only file"
	error_check_good db_sync [$db sync] 0
	error_check_good db_close [$db close] 0

	set ret [send_cmd $f1 "$remote_db close"]
	error_check_good remote_get $ret 0

	# Close the environment both remotely and locally.
	set ret [send_cmd $f1 "$remote_env close"]
	error_check_good remote:env_close $ret 0
	close $f1

	reset_env $dbenv
}
