# See the file LICENSE for redistribution information.
#
# Copyright (c) 2006-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	env015
# TEST	Rename the underlying directory of an env, make sure everything
# TEST	still works.  Test runs with regular named databases and with
# TEST	in-memory named databases.
proc env015 { } {
	source ./include.tcl

	env_cleanup $testdir
	set newdir NEWDIR

	puts "Env015: Test of renaming env directories."

	foreach dbtype { inmem ondisk } {
		puts "\tEnv015.a: Create env."
		set env [berkdb_env -create -mode 0644 -home $testdir]
		error_check_good env [is_valid_env $env] TRUE

		puts "\tEnv015.b: Create $dbtype db."
		if { $dbtype == "inmem" } {
			set testfile { "" file1.db }
		} else {
			set testfile file1.db
		}
		set db [eval {berkdb_open} -create -env $env -btree $testfile]
		error_check_good db_open [is_valid_db $db] TRUE
		for { set i 0 } { $i < 10 } { incr i } {
			error_check_good db_put [$db put $i $i] 0
		}

		# When the database is on disk, we have a file handle open
		# during the attempt to rename the directory.  As far as we
		# can tell, Windows doesn't allow this (that is, Windows
		# doesn't allow directories to be renamed when there is an
		# open handle inside them). For QNX, tclsh can not rename a 
		# directory correctly while there are shared memory files in
		# that directory.
		puts "\tEnv015.b: Rename directory."
		if { $is_windows_test || $is_qnx_test } {
			file mkdir $newdir
			eval file rename -force [glob $testdir/*] $newdir
			fileremove -force $testdir
		} else {
			file rename -force $testdir $newdir
		}

		puts "\tEnv015.c: Database is still available in new directory."
		for { set i 0 } { $i < 10 } { incr i } {
			set ret [$db get $i]
			error_check_good db_get [lindex [lindex $ret 0] 1] $i
		}

		puts "\tEnv015.d: Can't open database in old directory."
		catch {set db2 [eval \
		    {berkdb_open} -env $env -btree $testdir/$testfile]} db2
		error_check_bad open_fails [is_valid_db $db2] TRUE

		puts \
	    "\tEnv015.e: Recreate directory with original name and use it."
		file mkdir $testdir
		set newenv [berkdb_env -create -mode 0644 -home $testdir]
		error_check_good newenv [is_valid_env $env] TRUE

		set newdb [berkdb_open -create -env $newenv -btree foo.db]
		error_check_good newdb_open [is_valid_db $newdb] TRUE

		# There should not be any data in the new db.
		for { set i 0 } { $i < 10 } { incr i } {
			set ret [$newdb get $i]
			error_check_good db_get [llength $ret] 0
		}

		# Clean up.
		error_check_good db_close [$db close] 0
		error_check_good newdb_close [$newdb close] 0
		error_check_good envclose [$env close] 0
		error_check_good newenvclose [$newenv close] 0
		fileremove -f $newdir
	}
}
