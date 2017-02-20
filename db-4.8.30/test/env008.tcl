# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	env008
# TEST	Test environments and subdirectories.
proc env008 { } {
	global errorInfo
	global errorCode

	source ./include.tcl

	env_cleanup $testdir

	set subdir  1/1
	set subdir1 1/2
	file mkdir $testdir/$subdir $testdir/$subdir1
	set testfile $subdir/env.db

	puts "Env008: Test of environments and subdirectories."

	puts "\tEnv008.a: Create env and db."
	set env [berkdb_env -create -mode 0644 -home $testdir -txn]
	error_check_good env [is_valid_env $env] TRUE

	puts "\tEnv008.b: Remove db in subdir."
	env008_db $env $testfile
	error_check_good dbremove:$testfile \
	    [berkdb dbremove -env $env $testfile] 0

	#
	# Rather than remaking the db every time for the renames
	# just move around the new file name to another new file
	# name.
	#
	puts "\tEnv008.c: Rename db in subdir."
	env008_db $env $testfile
	set newfile $subdir/new.db
	error_check_good dbrename:$testfile/.. \
	    [berkdb dbrename -env $env $testfile $newfile] 0
	set testfile $newfile

	puts "\tEnv008.d: Rename db to parent dir."
	set newfile $subdir/../new.db
	error_check_good dbrename:$testfile/.. \
	    [berkdb dbrename -env $env $testfile $newfile] 0
	set testfile $newfile

	puts "\tEnv008.e: Rename db to child dir."
	set newfile $subdir/env.db
	error_check_good dbrename:$testfile/.. \
	    [berkdb dbrename -env $env $testfile $newfile] 0
	set testfile $newfile

	puts "\tEnv008.f: Rename db to another dir."
	set newfile $subdir1/env.db
	error_check_good dbrename:$testfile/.. \
	    [berkdb dbrename -env $env $testfile $newfile] 0

	error_check_good envclose [$env close] 0
	puts "\tEnv008 complete."
}

proc env008_db { env testfile } {
	set db [berkdb_open -env $env -create -btree $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE
	set ret [$db put key data]
	error_check_good dbput $ret 0
	error_check_good dbclose [$db close] 0
}
