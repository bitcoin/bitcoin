# See the file LICENSE for redistribution information.
#
# Copyright (c)-2009 Oracle.  All rights reserved.
#
# TEST repmgr024
# TEST Ensuring exactly one listener process.
# TEST 
# TEST Start a repmgr process with a listener.
# TEST Start a second process, and see that it does not become the listener.
# TEST Shut down the first process (gracefully).  Now a second process should
# TEST become listener.
# TEST Kill the listener process abruptly.  Running failchk should show that
# TEST recovery is necessary.  Run recovery and start a clean listener.

proc repmgr024 {  } {
	source ./include.tcl
	source $test_path/testutils.tcl

	set tnum "024"
	puts "Repmgr$tnum: Ensuring exactly one listener process."
	set site_prog [setup_site_prog]

	env_cleanup $testdir

	set masterdir $testdir/MASTERDIR

	file mkdir $masterdir

	set ports [available_ports 1]
	set master_port [lindex $ports 0]

	make_dbconfig $masterdir {{rep_set_nsites 3}}
	set masterenv [berkdb_env -rep -txn -thread -home $masterdir \
			   -isalive my_isalive -create]
	$masterenv close

	puts "\tRepmgr$tnum.a: Set up the master (on TCP port $master_port)."
	set master [open "| $site_prog" "r+"]
	fconfigure $master -buffering line
	puts $master "home $masterdir"
	puts $master "local $master_port"
	puts $master "output $testdir/m1output"
	puts $master "open_env"
	puts $master "start master"
	error_check_match ok1 [gets $master] "*Successful*" 

	# sync.
	puts $master "echo setup"
	set sentinel [gets $master]
	error_check_good echo_setup $sentinel "setup"
	
	puts "\tRepmgr$tnum.b: Start a second process at master."
	set m2 [open "| $site_prog" "r+"]
	fconfigure $m2 -buffering line
	puts $m2 "home $masterdir"
	puts $m2 "local $master_port"
	puts $m2 "output $testdir/m2output"
	puts $m2 "open_env"
	puts $m2 "start master"
	set ret [gets $m2]
	error_check_match ignored "$ret" "*DB_REP_IGNORE*"

	puts $m2 "echo started"
	set sentinel [gets $m2]
	error_check_good started $sentinel "started"

	close $m2
	close $master

	# Hmm, actually it'd probably be better to send them an "exit" command,
	# and then read until we get an EOF error.  That we we're sure they've
	# had a chance to finish the close operation.  This is a recurring
	# theme, doing stuff synchronously.  There should be a way to wrap this
	# up to make it the default behavior.

	puts "\tRepmgr$tnum.c: Restart 2nd process, to act as listener this time"
	set m2 [open "| $site_prog" "r+"]
	fconfigure $m2 -buffering line
	puts $m2 "home $masterdir"
	puts $m2 "local $master_port"
	puts $m2 "output $testdir/m2output2"
	puts $m2 "open_env"
	puts $m2 "start master"
	set answer [gets $m2]
	error_check_match ok2 "$answer" "*Successful*"

	puts "\tRepmgr$tnum.d: Clean up."
	close $m2

	puts "\tRepmgr$tnum.e: Start main process."
	set master [open "| $site_prog" "r+"]
	fconfigure $master -buffering line
	puts $master "home $masterdir"
	puts $master "local $master_port"
	puts $master "output $testdir/m1output3"
	puts $master "open_env"
	puts $master "start master"
	set answer [gets $master]
	error_check_match ok3 $answer "*Successful*"

	# This seems to require $KILL; tclkill does not work. 
	puts "\tRepmgr$tnum.f: Kill process [pid $master] without clean-up."
	exec $KILL [pid $master]
	catch {close $master}

	# In realistic, correct operation, the application should have called
	# failchk before trying to restart a new process.  But let's just prove
	# to ourselves that it's actually doing something.  This first try
	# should fail.
	# 
	puts "\tRepmgr$tnum.g: Start take-over process without failchk."
	set m2 [open "| $site_prog" "r+"]
	fconfigure $m2 -buffering line
	puts $m2 "home $masterdir"
	puts $m2 "local $master_port"
	puts $m2 "output $testdir/m2output3"
	puts $m2 "open_env"
	puts $m2 "start master"
	set answer [gets $m2]
	error_check_match ignored3 $answer "*DB_REP_IGNORE*"
	close $m2

	set masterenv [berkdb_env -thread -home $masterdir -isalive my_isalive]
	$masterenv failchk

	# This time it should work.
	puts "\tRepmgr$tnum.h: Start take-over process after failchk."
	set m2 [open "| $site_prog" "r+"]
	fconfigure $m2 -buffering line
	puts $m2 "home $masterdir"
	puts $m2 "local $master_port"
	puts $m2 "output $testdir/m2output4"
	puts $m2 "open_env"
	puts $m2 "start master"
	set answer [gets $m2]
	error_check_match ok4 $answer "*Successful*" 

	close $m2
	$masterenv close
}
