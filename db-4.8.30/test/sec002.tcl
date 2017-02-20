# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	sec002
# TEST	Test of security interface and catching errors in the
# TEST  face of attackers overwriting parts of existing files.
proc sec002 { } {
	global errorInfo
	global errorCode
	global has_crypto

	source ./include.tcl

	# Skip test if release does not support encryption.
	if { $has_crypto == 0 } {
		puts "Skipping test sec002 for non-crypto release."
		return
	}

	set testfile1 $testdir/sec002-1.db
	set testfile2 $testdir/sec002-2.db
	set testfile3 $testdir/sec002-3.db
	set testfile4 $testdir/sec002-4.db

	puts "Sec002: Test of basic encryption interface."
	env_cleanup $testdir

	set passwd1 "passwd1"
	set passwd2 "passwd2"
	set key "key"
	set data "data"
	set pagesize 1024

	#
	# Set up 4 databases, two encrypted, but with different passwords
	# and one unencrypt, but with checksumming turned on and one
	# unencrypted and no checksumming.  Place the exact same data
	# in each one.
	#
	puts "\tSec002.a: Setup databases"
	set db_cmd "-create -pagesize $pagesize -btree "
	set db [eval {berkdb_open} -encryptaes $passwd1 $db_cmd $testfile1]
	error_check_good db [is_valid_db $db] TRUE
	error_check_good dbput [$db put $key $data] 0
	error_check_good dbclose [$db close] 0

	set db [eval {berkdb_open} -encryptaes $passwd2 $db_cmd $testfile2]
	error_check_good db [is_valid_db $db] TRUE
	error_check_good dbput [$db put $key $data] 0
	error_check_good dbclose [$db close] 0

	set db [eval {berkdb_open} -chksum $db_cmd $testfile3]
	error_check_good db [is_valid_db $db] TRUE
	error_check_good dbput [$db put $key $data] 0
	error_check_good dbclose [$db close] 0

	set db [eval {berkdb_open} $db_cmd $testfile4]
	error_check_good db [is_valid_db $db] TRUE
	error_check_good dbput [$db put $key $data] 0
	error_check_good dbclose [$db close] 0

	#
	# If we reopen the normal file with the -chksum flag, there
	# should be no error and checksumming should be ignored.
	# If we reopen a checksummed file without the -chksum flag,
	# checksumming should still be in effect.  [#6959]
	#
	puts "\tSec002.b: Inheritance of chksum properties"
	puts "\t\tSec002.b1: Reopen ordinary file with -chksum flag"
	set db [eval {berkdb_open} -chksum $testfile4]
	error_check_good open_with_chksum [is_valid_db $db] TRUE
	set retdata [$db get $key]
	error_check_good testfile4_get [lindex [lindex $retdata 0] 1] $data
	error_check_good dbclose [$db close] 0

	puts "\t\tSec002.b2: Reopen checksummed file without -chksum flag"
	set db [eval {berkdb_open} $testfile3]
	error_check_good open_wo_chksum [is_valid_db $db] TRUE
	set retdata [$db get $key]
	error_check_good testfile3_get [lindex [lindex $retdata 0] 1] $data
	error_check_good dbclose [$db close] 0

	#
	# First just touch some bits in the file.  We know that in btree
	# meta pages, bytes 92-459 are unused.  Scribble on them in both
	# an encrypted, and both unencrypted files.  We should get
	# a checksum error for the encrypted, and checksummed files.
	# We should get no error for the normal file.
	#
	set fidlist {}
	set fid [open $testfile1 r+]
	lappend fidlist $fid
	set fid [open $testfile3 r+]
	lappend fidlist $fid
	set fid [open $testfile4 r+]
	lappend fidlist $fid

	puts "\tSec002.c: Overwrite unused space in meta-page"
	foreach f $fidlist {
		fconfigure $f -translation binary
		seek $f 100 start
		set byte [read $f 1]
		binary scan $byte c val
		set newval [expr ~$val]
		set newbyte [binary format c $newval]
		seek $f 100 start
		puts -nonewline $f $newbyte
		close $f
	}
	puts "\tSec002.d: Reopen modified databases"
	set stat [catch {berkdb_open_noerr -encryptaes $passwd1 $testfile1} ret]
	error_check_good db:$testfile1 $stat 1
	error_check_good db:$testfile1:fail \
	    [is_substr $ret "metadata page checksum error"] 1

	set stat [catch {berkdb_open_noerr -chksum $testfile3} ret]
	error_check_good db:$testfile3 $stat 1
	error_check_good db:$testfile3:fail \
	    [is_substr $ret "metadata page checksum error"] 1

	set stat [catch {berkdb_open_noerr $testfile4} db]
	error_check_good db:$testfile4 $stat 0
	error_check_good dbclose [$db close] 0

	# Skip the remainder of the test for Windows platforms.
	# Forcing the error which causes DB_RUNRECOVERY to be
	# returned ends up leaving open files that cannot be removed.
	if { $is_windows_test == 1 } {
		cleanup $testdir NULL 1
		puts "Skipping remainder of test for Windows"
		return
	}

	puts "\tSec002.e: Replace root page in encrypted w/ encrypted"
	set fid1 [open $testfile1 r+]
	fconfigure $fid1 -translation binary
	set fid2 [open $testfile2 r+]
	fconfigure $fid2 -translation binary
	seek $fid1 $pagesize start
	seek $fid2 $pagesize start
	fcopy $fid1 $fid2 -size $pagesize
	close $fid1
	close $fid2

	set db [berkdb_open_noerr -encryptaes $passwd2 $testfile2]
	error_check_good db [is_valid_db $db] TRUE
	set stat [catch {$db get $key} ret]
	error_check_good dbget $stat 1
	error_check_good db:$testfile2:fail1 \
	    [is_substr $ret "checksum error"] 1
	set stat [catch {$db close} ret]
	error_check_good dbclose $stat 1
	error_check_good db:$testfile2:fail2 [is_substr $ret "DB_RUNRECOVERY"] 1

	puts "\tSec002.f: Replace root page in encrypted w/ unencrypted"
	set fid2 [open $testfile2 r+]
	fconfigure $fid2 -translation binary
	set fid4 [open $testfile4 r+]
	fconfigure $fid4 -translation binary
	seek $fid2 $pagesize start
	seek $fid4 $pagesize start
	fcopy $fid4 $fid2 -size $pagesize
	close $fid4
	close $fid2

	set db [berkdb_open_noerr -encryptaes $passwd2 $testfile2]
	error_check_good db [is_valid_db $db] TRUE
	set stat [catch {$db get $key} ret]
	error_check_good dbget $stat 1
	error_check_good db:$testfile2:fail \
	    [is_substr $ret "checksum error"] 1
	set stat [catch {$db close} ret]
	error_check_good dbclose $stat 1
	error_check_good db:$testfile2:fail [is_substr $ret "DB_RUNRECOVERY"] 1

	cleanup $testdir NULL 1
}
