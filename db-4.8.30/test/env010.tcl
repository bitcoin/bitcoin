# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	env010
# TEST	Run recovery in an empty directory, and then make sure we can still
# TEST	create a database in that directory.
proc env010 { } {
	source ./include.tcl

	puts "Env010: Test of recovery in an empty directory."

	# Create a new directory used only for this test

	if { [file exists $testdir/EMPTYDIR] != 1 } {
		file mkdir $testdir/EMPTYDIR
	} else {
		puts "\nDirectory already exists."
	}

	# Do the test twice, for regular recovery and catastrophic
	# Open environment and recover, but don't create a database

	foreach rmethod {recover recover_fatal} {

		puts "\tEnv010: Creating env for $rmethod test."
		env_cleanup $testdir/EMPTYDIR
		set e [berkdb_env \
		    -create -home $testdir/EMPTYDIR -txn -$rmethod]
		error_check_good dbenv [is_valid_env $e] TRUE

		# Open and close a database
		# The method doesn't matter, so picked btree arbitrarily

		set db [eval {berkdb_open -env $e \
			-btree -create -mode 0644} ]
		error_check_good dbopen [is_valid_db $db] TRUE
		error_check_good db_close [$db close] 0

		# Close environment

		error_check_good envclose [$e close] 0
		error_check_good berkdb:envremove \
			[berkdb envremove -home $testdir/EMPTYDIR] 0
	}
	puts "\tEnv010 complete."
}
