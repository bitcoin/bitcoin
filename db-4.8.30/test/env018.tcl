# See the file LICENSE for redistribution information.
#
# Copyright (c)-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	env018
# TEST	Test getters when joining an env.  When a second handle is 
# TEST	opened on an existing env, get_open_flags needs to return 
# TEST	the correct flags to the second handle so it knows what sort
# TEST 	of environment it's just joined.
# TEST
# TEST	For several different flags to env_open, open an env.  Open
# TEST	a second handle on the same env, get_open_flags and verify 
# TEST	the flag is returned.
proc env018 { } {
	source ./include.tcl
	set tnum "018"

	puts "Env$tnum: Test of join_env and getters."

	# Skip for HP-UX where a second handle on an env is not allowed.
	if { $is_hp_test == 1 } {
		puts "Skipping env$tnum for HP-UX."
		return
	}

	# Set up flags to use in opening envs. 
	set flags { -cdb -lock -log -txn } 

	foreach flag $flags {
		env_cleanup $testdir

		puts "\t\tEnv$tnum.a: Open env with $flag."
		set e1 [eval {berkdb_env} -create -home $testdir $flag]
		error_check_good e1_open [is_valid_env $e1] TRUE
	
		puts "\t\tEnv$tnum.b: Join the env."
		set e2 [eval {berkdb_env} -home $testdir]
		error_check_good e2_open [is_valid_env $e2] TRUE

		# Get open flags for both envs.
		set e1_flags_returned [$e1 get_open_flags]
		set e2_flags_returned [$e2 get_open_flags]

		# Test that the flag given to the original env is
		# returned by a call to the second env. 
		puts "\t\tEnv$tnum.c: Check that flag is returned."
		error_check_good flag_is_returned \
		    [is_substr $e2_flags_returned $flag] 1

		# Clean up. 
		error_check_good e1_close [$e1 close] 0
		error_check_good e2_close [$e2 close] 0
	}
}

