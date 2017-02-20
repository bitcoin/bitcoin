# See the file LICENSE for redistribution information.
#
# Copyright (c)-2009 Oracle.  All rights reserved.
#
# TEST repmgr028
# TEST Simple smoke test for repmgr elections with multi-process envs.

proc repmgr028 { } {
	source ./include.tcl

	set tnum "028"
	puts "Repmgr$tnum:\
	    Smoke test for repmgr elections with multi-process envs."

	env_cleanup $testdir

	set ports [available_ports 3]
	
	set dbconfig {
		{rep_set_nsites 3} 
		{rep_set_timeout DB_REP_ELECTION_RETRY 2000000}
		{rep_set_timeout DB_REP_ELECTION_TIMEOUT 1000000}
	}
	foreach d {A B C} { 
		file mkdir $testdir/$d 
		make_dbconfig $testdir/$d $dbconfig
	}

	puts "\tRepmgr$tnum.a: Start 3 sites (with 2 processes each)."
	set cmds {
		{home $testdir/A}
		{local [lindex $ports 0]}
		{open_env}
		{start election}
	}
	set cmds [subst $cmds]
	set a1 [open_site_prog [linsert $cmds 1 "output $testdir/a1output"]]
	set a2 [open_site_prog [linsert $cmds 1 "output $testdir/a2output"]]

	set cmds {
		{home $testdir/B}
		{local [lindex $ports 1]}
		{remote localhost [lindex $ports 0]}
		{open_env}
		{start election}
	}
	set cmds [subst $cmds]
	set b1 [open_site_prog [linsert $cmds 1 "output $testdir/b1output"]]
	set b2 [open_site_prog [linsert $cmds 1 "output $testdir/b2output"]]

	set cmds {
		{home $testdir/C}
		{local [lindex $ports 2]}
		{remote localhost [lindex $ports 0]}
		{open_env}
		{start election}
	}
	set cmds [subst $cmds]
	set c1 [open_site_prog [linsert $cmds 1 "output $testdir/c1output"]]
	set c2 [open_site_prog [linsert $cmds 1 "output $testdir/c2output"]]
			
	puts "\tRepmgr$tnum.b: Wait for an election to choose initial master."
	set a [berkdb_env -home $testdir/A]
	set b [berkdb_env -home $testdir/B]
	set c [berkdb_env -home $testdir/C]
	set sites "$a $b $c"
	set menv [repmgr028_await_election $sites]
	set i [lsearch -exact $sites $menv]
	error_check_bad notfound $i -1
	set site_names "abc"
	set m [string range $site_names $i $i]
	puts "\tRepmgr$tnum.c: (site $m is master)."
	
	puts "\tRepmgr$tnum.d: Wait for other two sites to sync up."
	set clients [lreplace $sites $i $i]
	set site_names [string replace $site_names $i $i]
	await_startup_done [lindex $clients 0]
	await_startup_done [lindex $clients 1]

	set m1 [subst $${m}1]
	set m2 [subst $${m}2]

	puts $m2 "open_db test.db"
	puts $m2 "put key1 value1"
	puts $m2 "echo done"
	gets $m2

	puts "\tRepmgr$tnum.e:\
	    Shut down master, wait for survivors to elect new master."
	$menv close
	close $m1
	close $m2

	set menv [repmgr028_await_election $clients]
	set i [lsearch -exact $clients $menv]
	error_check_bad notfound2 $i -1
	set m [string range $site_names $i $i]
	puts "\tRepmgr$tnum.f: (site $m is new master)."

	puts "\tRepmgr$tnum.g: Wait for remaining client to sync to new master."
	set client [lreplace $clients $i $i]
	error_check_good master_changes \
	    [stat_field $client rep_stat "Master changes"] 2
	await_startup_done $client

	puts "\tRepmgr$tnum.h: Clean up."
	$client close
	$menv close

	set c [string range $site_names 0 0]
	close [subst $${c}1]
	close [subst $${c}2]
	set c [string range $site_names 1 1]
	close [subst $${c}1]
	close [subst $${c}2]
}

proc repmgr028_await_election { env_list } {
	set cond {
		foreach e $env_list {
			if {[stat_field $e rep_stat "Role"] == "master"} {
				set answer $e
				break
			}
		}
		expr {[info exists answer]}
	}
	await_condition {[eval $cond]} 20
	return $answer
}
