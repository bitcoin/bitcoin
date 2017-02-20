# See the file LICENSE for redistribution information.
#
# Copyright (c)-2009 Oracle.  All rights reserved.
#
# TEST repmgr031
# TEST Test repmgr's internal juggling of peer EID's.
# TEST
# TEST Set up master and 2 clients, A and B.
# TEST Add a third client (C), with two processes.
# TEST The first process will be configured to know about A.
# TEST The second process will know about B, and set that as peer,
# TEST but when it joins the env site B will have to be shuffled
# TEST into a later position in the list, because A is already first.

# This whole test is highly dependent upon the internal implementation structure
# of repmgr's multi-process support.  If that implementation changes, this test
# may become irrelevant, irrational, and inconsequential.  If that happens, it
# makes sense to simply discard this test.

proc repmgr031 { } {
	foreach initial_action {true false} {
		foreach while_active {true false} {
			repmgr031_sub $initial_action $while_active
		}
	}
}

proc repmgr031_sub { {a_too false} {while_active true} } {
	source ./include.tcl

	if {$a_too} {
		set part1 "shuffle with peer reassignment"
	} else {
		set part1 "shuffle"
	}
	if {$while_active} {
		set part2 "while active"
	} else {
		set part2 "while not active"
	}
	set tnum "031"
	puts "Repmgr$tnum: ($part1, then peer change $part2)"

	env_cleanup $testdir
	foreach {mport aport bport cport} [available_ports 4] {}
	file mkdir [set dirm $testdir/M]
	file mkdir [set dira $testdir/A]
	file mkdir [set dirb $testdir/B]
	file mkdir [set dirc $testdir/C]

	set dbc {{repmgr_set_ack_policy DB_REPMGR_ACKS_ALL}}
	make_dbconfig $dirm $dbc
	make_dbconfig $dira $dbc
	make_dbconfig $dirb $dbc
	make_dbconfig $dirc $dbc

	puts "\tRepmgr$tnum.a: Create a master and first two clients."
	set cmds {
		"home $dirm"
		"local $mport"
		"output $testdir/moutput"
		"open_env"
		"start master"
	}
	set m [open_site_prog [subst $cmds]]

	set cmds {
		"home $dira"
		"local $aport"
		"output $testdir/aoutput"
		"remote localhost $mport"
		"open_env"
		"start client"
	}
	set a [open_site_prog [subst $cmds]]

	set cmds {
		"home $dirb"
		"local $bport"
		"output $testdir/boutput"
		"remote localhost $mport"
		"open_env"
		"start client"
	}
	set b [open_site_prog [subst $cmds]]

	set aenv [berkdb_env -home $dira]
	await_startup_done $aenv
	set benv [berkdb_env -home $dirb]
	await_startup_done $benv

	# Now it gets interesting.
	puts "\tRepmgr$tnum.b: Create client C, with two processes."
	if {$a_too} {
		set peer_flag "-p"
	} else {
		set peer_flag ""
	}
	set cmds {
		"home $dirc"
		"local $cport"
		"output $testdir/c1output"
		"remote $peer_flag localhost $aport"
		"remote localhost $mport"
		"open_env"
	}
	set c1 [open_site_prog [subst $cmds]]

	set cmds {
		"home $dirc"
		"local $cport"
		"output $testdir/c2output"
		"remote -p localhost $bport"
		"open_env"
	}
	set c2 [open_site_prog [subst $cmds]]

	puts $c1 "start client"
	gets $c1
	set cenv [berkdb_env -home $dirc]
	await_startup_done $cenv 10

	puts "\tRepmgr$tnum.c: Check resulting statistics."
	# Make sure we used B, not A, as the c2c peer.
	set requests_at_A [repmgr031_get_request_count $aenv]
	set requests_at_B [repmgr031_get_request_count $benv]
	error_check_good no_requests_at_A $requests_at_A 0
	error_check_bad some_requests_at_B $requests_at_B 0

	# Check that site list order is what we expect.
	set sl [$cenv repmgr_site_list]
	error_check_good site_list [lindex $sl 0 2] $aport
	error_check_good site_list [lindex $sl 1 2] $mport
	error_check_good site_list [lindex $sl 2 2] $bport


	# Give client C a reason to send another request: shut it down, and
	# create some new transactions at the master.
	# 
	puts $c2 "exit"
	gets $c2
	close $c2
	puts $c1 "exit"
	gets $c1
	close $c1

	puts $m "open_db test.db"
	puts $m "put k1 v1"
	puts $m "put k2 v2"
	puts $m "echo done"
	gets $m

	# Change peer setting at C.
	# 
	puts "\tRepmgr$tnum.d: Start client C again."
	if { $while_active } {
		set cmds {
			"home $dirc"
			"output $testdir/c1output2"
			"open_env"
			"remote -p localhost $aport"
			"start client"
		}
	} else {
		set cmds {
			"home $dirc"
			"output $testdir/c1output2"
			"remote -p localhost $aport"
			"open_env"
			"start client"
		}
	}
	set c [open_site_prog [subst $cmds]]

	# Wait for restarted client to catch up with master.
	set menv [berkdb_env -home $dirm]
	set seq 0
	set cond {
		incr seq
		puts $m "put newkey$seq newdata$seq"
		puts $m "echo done"
		gets $m
		set log_end [next_expected_lsn $menv]
		set client_log_end [next_expected_lsn $cenv]
		expr [string compare $client_log_end $log_end] == 0
	}
	await_condition {[eval $cond]}

	# Make sure client B has not serviced any more requests, and that
	# instead now client A has serviced some.

	error_check_good no_addl_reqs \
	    [repmgr031_get_request_count $benv] $requests_at_B
	error_check_bad some_requests_at_A [repmgr031_get_request_count $aenv] 0
	
	$cenv close
	$benv close
	$aenv close
	$menv close

	close $c
	close $a
	close $b
	close $m
}

proc repmgr031_get_request_count { env } {
	stat_field $env rep_stat "Client service requests"
}
