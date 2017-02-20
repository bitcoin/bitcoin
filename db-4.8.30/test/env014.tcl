# See the file LICENSE for redistribution information.
#
# Copyright (c) 2005-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	env014
# TEST
# TEST	Make sure that attempts to open an environment with
# TEST	incompatible flags (e.g. replication without transactions)
# TEST	fail with the appropriate messages.
# TEST
# TEST	A new thread of control joining an env automatically
# TEST	initializes the same subsystems as the original env.
# TEST	Make sure that the attempt to change subsystems when
# TEST	joining an env fails with the appropriate messages.

proc env014 { } {
	source ./include.tcl

	set tnum "014"
	puts "Env$tnum: Environment subsystem initialization and env joins."
	env_cleanup $testdir

	# Open an env with -recover but not -create; should fail.
	puts "\tEnv$tnum.a: Open env with -recover but not -create."
	catch {set env [berkdb_env_noerr -recover -txn -home $testdir]} ret
	error_check_good recover_wo_create \
	    [is_substr $ret "requires the create flag"] 1

	# Open an env with -recover but not -txn; should fail.
	puts "\tEnv$tnum.b: Open env with -recover but not -txn."
	catch {set env [berkdb_env_noerr -create -recover -home $testdir]} ret
	error_check_good recover_wo_txn \
	    [is_substr $ret "requires transaction support"] 1

	# Open an env with -replication but not -lock; should fail.
	puts "\tEnv$tnum.c: Open env with -rep but not -lock."
	catch {set env\
	    [berkdb_env_noerr -create -rep_master -home $testdir]} ret
	error_check_good rep_wo_lock \
	    [is_substr $ret "requires locking support"] 1

	# Open an env with -replication but not -txn; should fail.
	puts "\tEnv$tnum.d: Open env with -rep but not -txn."
	catch {set env\
	    [berkdb_env_noerr -create -rep_master -lock -home $testdir]} ret
	error_check_good rep_wo_txn \
	    [is_substr $ret "requires transaction support"] 1

	# Skip remainder of test for HP-UX; HP-UX does not allow
	# opening a second handle on an environment.
	if { $is_hp_test == 1 } {
		puts "Skipping remainder of env$tnum for HP-UX."
		return
	}

	# Join -txn env with -cdb; should fail.
	puts "\tEnv$tnum.e: Join -txn env with -cdb."
	set env [berkdb_env_noerr -create -home $testdir -txn]
	error_check_good env_open [is_valid_env $env] TRUE

	catch {set env2 [berkdb_env_noerr -home $testdir -cdb]} ret
	error_check_good txn+cdb [is_substr $ret "incompatible"] 1
	error_check_good env_close [$env close] 0
	error_check_good env_remove [berkdb envremove -force -home $testdir] 0

	# Join -cdb env with -txn; should fail.
	puts "\tEnv$tnum.f: Join -cdb env with -txn."
	set env [berkdb_env_noerr -create -home $testdir -cdb]
	error_check_good env_open [is_valid_env $env] TRUE

	catch {set env2 [berkdb_env_noerr -home $testdir -txn]} ret
	error_check_good cdb+txn [is_substr $ret "incompatible"] 1
	error_check_good env_close [$env close] 0
	error_check_good env_remove [berkdb envremove -force -home $testdir] 0

	# Open an env with -txn.  Join the env, and start a txn.
	puts "\tEnv$tnum.g: Join -txn env, and start a txn."
	set env [berkdb_env_noerr -create -home $testdir -txn]
	error_check_good env_open [is_valid_env $env] TRUE
	set env2 [berkdb_env_noerr -home $testdir]
	error_check_good env2_open [is_valid_env $env2] TRUE

	set txn [$env2 txn]
	error_check_good env2_txn [is_valid_txn $txn $env2] TRUE
	error_check_good txn_commit [$txn commit] 0

	error_check_good env2_close [$env2 close] 0
	error_check_good env_close [$env close] 0
	error_check_good env_remove [berkdb envremove -force -home $testdir] 0

	# Join -txn env with -lock; should succeed and use txns.
	puts "\tEnv$tnum.h: Join -txn env with -lock, and start a txn."
	set env [berkdb_env_noerr -create -home $testdir -txn]
	error_check_good env_open [is_valid_env $env] TRUE
	set env2 [berkdb_env_noerr -home $testdir -lock]
	error_check_good env2_open [is_valid_env $env2] TRUE

	set txn [$env2 txn]
	error_check_good env2_txn [is_valid_txn $txn $env2] TRUE
	error_check_good txn_commit [$txn commit] 0

	error_check_good env2_close [$env2 close] 0
	error_check_good env_close [$env close] 0
	error_check_good env_remove [berkdb envremove -force -home $testdir] 0

  	# Join plain vanilla env with -txn; should fail.
	puts "\tEnv$tnum.i: Join plain vanilla env with -txn."
	set env [berkdb_env_noerr -create -home $testdir]
	error_check_good env_open [is_valid_env $env] TRUE
	catch {set env2 [berkdb_env_noerr -home $testdir -txn]} ret
	error_check_good ds+txn [is_substr $ret "incompatible"] 1

	error_check_good env_close [$env close] 0
	error_check_good env_remove [berkdb envremove -force -home $testdir] 0
}
