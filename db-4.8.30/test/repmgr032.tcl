# See the file LICENSE for redistribution information.
#
# Copyright (c)-2009 Oracle.  All rights reserved.
#
# TEST	repmgr032
# TEST  Multi-process repmgr start-up policies.
#

proc repmgr032 { } {
	source ./include.tcl

	set tnum "032"
	puts "Repmgr$tnum: repmgr multi-process start policies."

	env_cleanup $testdir

	file mkdir [set dira $testdir/A]
	file mkdir [set dirb $testdir/B]
	file mkdir [set dirc $testdir/C]
	
	set conf {
		{rep_set_nsites 3}
		{rep_set_timeout DB_REP_ELECTION_RETRY 3000000}
	}
	make_dbconfig $dira $conf
	make_dbconfig $dirb $conf
	make_dbconfig $dirc $conf
	foreach {aport bport cport} [available_ports 3] {}

	puts "\tRepmgr$tnum.a: Create a master and client."
	set cmds {
		"home $dira"
		"local $aport"
		"output $testdir/a1output1"
		"open_env"
		"start master"
	}
	set a [open_site_prog [subst $cmds]]

	set cmds {
		"home $dirb"
		"local $bport"
		"output $testdir/boutput1"
		"remote localhost $aport"
		"open_env"
		"start client"
	}
	set b [open_site_prog [subst $cmds]]

	puts "\tRepmgr$tnum.b: Wait for start-up done at client."
	set benv [berkdb_env -home $dirb]
	await_startup_done $benv

	puts $a "open_db test.db"
	puts $a "put key1 val1"
	puts $a "echo done"
	gets $a

	#
	# 1. shutting down client and restarting as client should do nothing
	# 
	puts "\tRepmgr$tnum.c: Shut down client, restart as client."
	set elections0 [stat_field $benv rep_stat "Elections held"]
	set aenv [berkdb_env -home $dira]
	set requests0 [stat_field $aenv rep_stat "Client service requests"]
	puts $b "exit"
	gets $b
	error_check_good eof1 [eof $b] 1
	close $b
	set cmds {
		"home $dirb"
		"local $bport"
		"output $testdir/boutput1"
		"remote localhost $aport"
		"open_env"
		"start election"
	}
	set b [open_site_prog [subst $cmds]]
	error_check_good already_startedup \
	    [stat_field $benv rep_stat "Startup complete"] 1
	puts "\tRepmgr$tnum.d: Pause 20 seconds to check for start-up activity"
	tclsleep 20
	error_check_good no_more_requests \
	    [stat_field $aenv rep_stat "Client service requests"] $requests0
	error_check_good no_more_requests \
	    [stat_field $benv rep_stat "Elections held"] $elections0

	#
	# 2. Start policy should be ignored if there's already a listener
	#    running in a separate process.
	#

	# start a second process at master.  Even though it specifies "election"
	# as its start policy, the fact that a listener is already running
	# should force it to continue as master (IMHO).
	
	puts "\tRepmgr$tnum.e: Second master process accepts existing role"
	set cmds {
		"home $dira"
		"local $aport"
		"output $testdir/a2output1"
		"open_env"
		"start election"
	}
	set a2 [open_site_prog [subst $cmds]]

	# Make sure we still seem to be master, by checking stats, and by trying
	# to write a new transaction.
	# 
	error_check_good still_master \
	    [stat_field $aenv rep_stat "Role"] "master"

	puts $a2 "open_db test.db"
	puts $a2 "put key2 val2"
	puts $a2 "echo done"
	gets $a2

	#
	# 3. Specifying MASTER start policy results in rep_start(MASTER), no
	#    matter what happened previously.
	#
	puts "\tRepmgr$tnum.f: Restart master as master."

	puts $a "exit"
	gets $a
	error_check_good eof2 [eof $a] 1
	close $a
	puts $a2 "exit"
	gets $a2
	error_check_good eof3 [eof $a2] 1
	close $a2

	set initial_gen [stat_field $aenv rep_stat "Generation number"]
	set cmds {
		"home $dira"
		"local $aport"
		"output $testdir/a2output2"
		"remote localhost $bport"
		"open_env"
		"start master"
	}
	set a [open_site_prog [subst $cmds]]

	# Since we were already master, the gen number shouldn't change.
	error_check_good same_gen \
	    [stat_field $aenv rep_stat "Generation number"] $initial_gen

	puts $a "exit"
	gets $a
	error_check_good eof4 [eof $a] 1
	close $a
	puts $b "exit"
	gets $b
	error_check_good eof5 [eof $b] 1
	close $b

	puts "\tRepmgr$tnum.g: Restart client as master."
	# Note that site A is not running at this point.
	set cmds {
		"home $dirb"
		"local $bport"
		"output $testdir/boutput3"
		"open_env"
		"start master"
	}
	set b [open_site_prog [subst $cmds]]
	set gen [stat_field $benv rep_stat "Generation number"]
	error_check_good bumped_gen [expr $gen > $initial_gen] 1


	#
	# 4. Specifying CLIENT when we were MASTER causes a change
	#
	puts $b "exit"
	gets $b
	error_check_good eof6 [eof $b] 1
	close $b
	$benv close
	exec $util_path/db_recover -h $dirb

	puts "\tRepmgr$tnum.h: Restart master as client"
	set initial_value [stat_field $aenv rep_stat "Elections held"]
	set cmds {
		"home $dira"
		"local $aport"
		"output $testdir/aoutput4"
		"open_env"
		"start election"
	}
	set a [open_site_prog [subst $cmds]]
	puts "\tRepmgr$tnum.i: Pause for 10 seconds to wait for elections."
	tclsleep 10
	set elections [stat_field $aenv rep_stat "Elections held"]
	error_check_good bumped_gen [expr $elections > $initial_value] 1

	$aenv close
	close $a
}
