# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	sec001
# TEST	Test of security interface
proc sec001 { } {
	global errorInfo
	global errorCode
	global has_crypto
	global is_hp_test

	source ./include.tcl
	# Skip test if release does not support encryption.
	if { $has_crypto == 0 } {
		puts "Skipping test sec001 for non-crypto release."
		return
	}

	set testfile1 env1.db
	set testfile2 $testdir/env2.db
	set subdb1 sub1
	set subdb2 sub2

	puts "Sec001: Test of basic encryption interface."
	env_cleanup $testdir

	set passwd1 "passwd1"
	set passwd1_bad "passwd1_bad"
	set passwd2 "passwd2"
	set key "key"
	set data "data"

	#
	# This first group tests bad create scenarios and also
	# tests attempting to use encryption after creating a
	# non-encrypted env/db to begin with.
	#
	set nopass ""
	puts "\tSec001.a.1: Create db with encryption."
	set db [berkdb_open -create -encryptaes $passwd1 -btree $testfile2]
	error_check_good db [is_valid_db $db] TRUE
	error_check_good dbput [$db put $key $data] 0
	error_check_good dbclose [$db close] 0

	puts "\tSec001.a.2: Open db without encryption."
	set stat [catch {berkdb_open_noerr $testfile2} ret]
	error_check_good db:nocrypto $stat 1
	error_check_good db:fail [is_substr $ret "no encryption key"] 1

	set ret [berkdb dbremove -encryptaes $passwd1 $testfile2]

	puts "\tSec001.b.1: Create db without encryption or checksum."
	set db [berkdb_open -create -btree $testfile2]
	error_check_good db [is_valid_db $db] TRUE
	error_check_good dbput [$db put $key $data] 0
	error_check_good dbclose [$db close] 0

	puts "\tSec001.b.2: Open db with encryption."
	set stat [catch {berkdb_open_noerr -encryptaes $passwd1 $testfile2} ret]
	error_check_good db:nocrypto $stat 1
	error_check_good db:fail [is_substr $ret "supplied encryption key"] 1

	set ret [berkdb dbremove $testfile2]

	puts "\tSec001.c.1: Create db with checksum."
	set db [berkdb_open -create -chksum -btree $testfile2]
	error_check_good db [is_valid_db $db] TRUE
	error_check_good dbput [$db put $key $data] 0
	error_check_good dbclose [$db close] 0

	puts "\tSec001.c.2: Open db with encryption."
	set stat [catch {berkdb_open_noerr -encryptaes $passwd1 $testfile2} ret]
	error_check_good db:nocrypto $stat 1
	error_check_good db:fail [is_substr $ret "supplied encryption key"] 1

	set ret [berkdb dbremove $testfile2]

	puts "\tSec001.d.1: Create subdb with encryption."
	set db [berkdb_open -create -encryptaes $passwd1 -btree \
	    $testfile2 $subdb1]
	error_check_good subdb [is_valid_db $db] TRUE
	error_check_good dbput [$db put $key $data] 0
	error_check_good dbclose [$db close] 0

	puts "\tSec001.d.2: Create 2nd subdb without encryption."
	set stat [catch {berkdb_open_noerr -create -btree \
	    $testfile2 $subdb2} ret]
	error_check_good subdb:nocrypto $stat 1
	error_check_good subdb:fail [is_substr $ret "no encryption key"] 1

	set ret [berkdb dbremove -encryptaes $passwd1 $testfile2]

	puts "\tSec001.e.1: Create subdb without encryption or checksum."
	set db [berkdb_open -create -btree $testfile2 $subdb1]
	error_check_good db [is_valid_db $db] TRUE
	error_check_good dbput [$db put $key $data] 0
	error_check_good dbclose [$db close] 0

	puts "\tSec001.e.2: Create 2nd subdb with encryption."
	set stat [catch {berkdb_open_noerr -create -btree -encryptaes $passwd1 \
	    $testfile2 $subdb2} ret]
	error_check_good subdb:nocrypto $stat 1
	error_check_good subdb:fail [is_substr $ret "supplied encryption key"] 1

	env_cleanup $testdir

	puts "\tSec001.f.1: Open env with encryption, empty passwd."
	set stat [catch {berkdb_env_noerr -create -home $testdir \
	    -encryptaes $nopass} ret]
	error_check_good env:nopass $stat 1
	error_check_good env:fail [is_substr $ret "Empty password"] 1

	puts "\tSec001.f.2: Create without encryption algorithm (DB_ENCRYPT_ANY)."
	set stat [catch {berkdb_env_noerr -create -home $testdir \
	    -encryptany $passwd1} ret]
	error_check_good env:any $stat 1
	error_check_good env:fail [is_substr $ret "algorithm not supplied"] 1

	puts "\tSec001.f.3: Create without encryption."
	set env [berkdb_env -create -home $testdir]
	error_check_good env [is_valid_env $env] TRUE

	# Skip this piece of the test on HP-UX, where we can't
	# join the env.
	if { $is_hp_test != 1 } {
		puts "\tSec001.f.4: Open again with encryption."
		set stat [catch {berkdb_env_noerr -home $testdir \
		    -encryptaes $passwd1} ret]
		error_check_good env:unencrypted $stat 1
		error_check_good env:fail [is_substr $ret \
		    "Joining non-encrypted environment"] 1
	}

	error_check_good envclose [$env close] 0

	env_cleanup $testdir

	#
	# This second group tests creating and opening a secure env.
	# We test that others can join successfully, and that other's with
	# bad/no passwords cannot.  Also test that we cannot use the
	# db->set_encrypt method when we've already got a secure dbenv.
	#
	puts "\tSec001.g.1: Open with encryption."
	set env [berkdb_env_noerr -create -home $testdir -encryptaes $passwd1]
	error_check_good env [is_valid_env $env] TRUE

	# We can't open an env twice in HP-UX, so skip the rest.
	if { $is_hp_test == 1 } {
		puts "Skipping remainder of test for HP-UX."
		error_check_good env_close [$env close] 0
		return
	}

	puts "\tSec001.g.2: Open again with encryption - same passwd."
	set env1 [berkdb_env -home $testdir -encryptaes $passwd1]
	error_check_good env [is_valid_env $env1] TRUE
	error_check_good envclose [$env1 close] 0

	puts "\tSec001.g.3: Open again with any encryption (DB_ENCRYPT_ANY)."
	set env1 [berkdb_env -home $testdir -encryptany $passwd1]
	error_check_good env [is_valid_env $env1] TRUE
	error_check_good envclose [$env1 close] 0

	puts "\tSec001.g.4: Open with encryption - different length passwd."
	set stat [catch {berkdb_env_noerr -home $testdir \
	    -encryptaes $passwd1_bad} ret]
	error_check_good env:$passwd1_bad $stat 1
	error_check_good env:fail [is_substr $ret "Invalid password"] 1

	puts "\tSec001.g.5: Open with encryption - different passwd."
	set stat [catch {berkdb_env_noerr -home $testdir \
	    -encryptaes $passwd2} ret]
	error_check_good env:$passwd2 $stat 1
	error_check_good env:fail [is_substr $ret "Invalid password"] 1

	puts "\tSec001.g.6: Open env without encryption."
	set stat [catch {berkdb_env_noerr -home $testdir} ret]
	error_check_good env:$passwd2 $stat 1
	error_check_good env:fail [is_substr $ret "Encrypted environment"] 1

	puts "\tSec001.g.7: Open database with encryption in env"
	set stat [catch {berkdb_open_noerr -env $env -btree -create \
	    -encryptaes $passwd2 $testfile1} ret]
	error_check_good db:$passwd2 $stat 1
	error_check_good env:fail [is_substr $ret "method not permitted"] 1

	puts "\tSec001.g.8: Close creating env"
	error_check_good envclose [$env close] 0

	#
	# This third group tests opening the env after the original env
	# handle is closed.  Just to make sure we can reopen it in
	# the right fashion even if no handles are currently open.
	#
	puts "\tSec001.h.1: Reopen without encryption."
	set stat [catch {berkdb_env_noerr -home $testdir} ret]
	error_check_good env:noencrypt $stat 1
	error_check_good env:fail [is_substr $ret "Encrypted environment"] 1

	puts "\tSec001.h.2: Reopen with bad passwd."
	set stat [catch {berkdb_env_noerr -home $testdir -encryptaes \
	    $passwd1_bad} ret]
	error_check_good env:$passwd1_bad $stat 1
	error_check_good env:fail [is_substr $ret "Invalid password"] 1

	puts "\tSec001.h.3: Reopen with encryption."
	set env [berkdb_env -create -home $testdir -encryptaes $passwd1]
	error_check_good env [is_valid_env $env] TRUE

	puts "\tSec001.h.4: 2nd Reopen with encryption."
	set env1 [berkdb_env -home $testdir -encryptaes $passwd1]
	error_check_good env [is_valid_env $env1] TRUE

	error_check_good envclose [$env1 close] 0
	error_check_good envclose [$env close] 0

	puts "\tSec001 complete."
}
