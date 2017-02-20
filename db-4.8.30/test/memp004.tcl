# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#

# TEST	memp004
# TEST	Test that small read-only databases are mapped into memory.
proc memp004 { } {
	global is_qnx_test
	source ./include.tcl

	puts "Memp004: Test of memory-mapped read-only databases"

	if { $is_qnx_test } {
		puts "Memp004: skipping for QNX"
		return
	}

	env_cleanup $testdir
	set testfile memp004.db

	# Create an environment.
	puts "memp004.a: Create an environment and database"
	set dbenv [eval {berkdb_env -create -home $testdir -private}]
	error_check_good dbenv [is_valid_env $dbenv] TRUE
	set db [berkdb_open -env $dbenv -create -mode 0644 -btree $testfile]
	error_check_good dbopen/$testfile/RW [is_valid_db $db] TRUE

	# Put each key/data pair.
	set did [open $dict]
	set keys ""
	set count 0
	while { [gets $did str] != -1 && $count < 1000 } {
		lappend keys $str

		set ret [eval {$db put} {$str $str}]
		error_check_good put $ret 0

		incr count
	}
	close $did
	error_check_good close [$db close] 0

	# Discard the environment.
	error_check_good close [$dbenv close] 0

	puts "memp004.b: Re-create the environment and open database read-only"
	set dbenv [eval {berkdb_env -create -home $testdir}]
	error_check_good dbenv [is_valid_env $dbenv] TRUE
	set db [berkdb_open -env $dbenv -rdonly $testfile]
	error_check_good dbopen/$testfile/RO [is_substr $db db] 1

	# Read a couple of keys.
	set c [eval {$db cursor}]
	for { set i 0 } { $i < 500 } { incr i } {
		set ret [$c get -next]
	}

	puts "memp004.c: Check mpool statistics"
	set tmp [memp004_stat $dbenv "Pages mapped into address space"]
	error_check_good "mmap check: $tmp >= 500" [expr $tmp >= 500] 1

	error_check_good db_close [$db close] 0
	reset_env $dbenv
}

# memp004_stat --
#	Return the current mpool statistics.
proc memp004_stat { env s } {
	set stat [$env mpool_stat]
	foreach statpair $stat {
		set statmsg [lindex $statpair 0]
		set statval [lindex $statpair 1]
		if {[is_substr $statmsg $s] != 0} {
			return $statval
		}
	}
	puts "FAIL: memp004: stat string $s not found"
	return 0
}
