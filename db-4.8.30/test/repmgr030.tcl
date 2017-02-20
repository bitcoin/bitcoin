# See the file LICENSE for redistribution information.
#
# Copyright (c)-2009 Oracle.  All rights reserved.
#
# TEST repmgr030
# TEST Subordinate connections and processes should not trigger elections.

proc repmgr030 { } {
	source ./include.tcl

	set tnum "030"
	puts "Repmgr$tnum: Subordinate\
	    connections and processes should not trigger elections."

	env_cleanup $testdir

	foreach {mport cport} [available_ports 2] {}
	file mkdir [set mdir $testdir/MASTER]
	file mkdir [set cdir $testdir/CLIENT]

	make_dbconfig $mdir [set dbconfig {{rep_set_nsites 3}}]
	make_dbconfig $cdir $dbconfig

	puts "\tRepmgr$tnum.a: Set up a pair of sites, two processes each."
	set cmds {
		"home $mdir"
		"local $mport"
		"output $testdir/m1output"
		"open_env"
		"start master"
	}
	set m1 [open_site_prog [subst $cmds]]

	set cmds {
		"home $mdir"
		"local $mport"
		"output $testdir/m2output"
		"open_env"
		"start master"
	}
	set m2 [open_site_prog [subst $cmds]]

	# Force subordinate client process to be the one to inform master of its
	# address, to be sure there's a connection.  This shouldn't be
	# necessary, but it's hard to verify this in a test.
	# 
	set cmds {
		"home $cdir"
		"local $cport"
		"output $testdir/c1output"
		"open_env"
		"start client"
	}
	set c1 [open_site_prog [subst $cmds]]

	set cmds {
		"home $cdir"
		"local $cport"
		"output $testdir/c2output"
		"remote localhost $mport"
		"open_env"
		"start client"
	}
	set c2 [open_site_prog [subst $cmds]]

	set cenv [berkdb_env -home $cdir]
	await_startup_done $cenv

	puts "\tRepmgr$tnum.b: Stop master's subordinate process (pause)."
	close $m2

	# Pause to let client notice the connection loss.
	tclsleep 3

	# The client main process is still running, but it shouldn't care about
	# a connection loss to the master's subordinate process.

	puts "\tRepmgr$tnum.c:\
	    Stop client's main process, then master's main process (pause)."
	close $c1
	tclsleep 2
	close $m1
	tclsleep 3

	# If the client main process were still running, it would have reacted
	# to the loss of the master by calling for an election.  However, with
	# only the client subordinate process still running, he cannot call for
	# an election.  So, we should see no elections ever having been
	# started.
	# 
	set election_count [stat_field $cenv rep_stat "Elections held"]
	puts "\tRepmgr$tnum.d: Check election count ($election_count)."
	error_check_good no_elections $election_count 0

	$cenv close
	close $c2
}
