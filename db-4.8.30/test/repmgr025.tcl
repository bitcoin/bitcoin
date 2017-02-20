# See the file LICENSE for redistribution information.
#
# Copyright (c)-2009 Oracle.  All rights reserved.
#

# TEST repmgr025
# TEST Repmgr site address discovery via handshakes.
# TEST
# TEST Master with 2 processes, does not know client's address.
# TEST Client processes start in either order, connect to master.
# TEST Master learns of client's address via handshake message.

proc repmgr025 { } {
	repmgr025_sub yes
	repmgr025_sub no
}

# We can either have the master's subordinate process discover the client
# address "on the fly" (i.e., in the course of doing a transaction), or more
# delicately by not starting it until we know that the address is in the
# shared region ready to be discovered.  In the delicate case, we expect no perm
# failures, but otherwise there's almost always one.
#
proc repmgr025_sub { on_the_fly } {
	source ./include.tcl

	set tnum "025"
	if { $on_the_fly } { 
		set msg "on the fly"
	} else { 
		set msg "regular"
	}
	puts "Repmgr$tnum: Site address discovery via handshakes ($msg)"
	set site_prog [setup_site_prog]

	env_cleanup $testdir

	foreach {master_port client_port} [available_ports 2] {
		puts "\tRepmgr$tnum.a:\
		    Using ports $master_port, $client_port"
	}

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	file mkdir $masterdir
	file mkdir $clientdir
	
	puts "\tRepmgr$tnum.b: Start 2 processes at master."
	make_dbconfig $masterdir {{rep_set_nsites 3}}
	set m1 [open "| $site_prog" "r+"]
	fconfigure $m1 -buffering line
	puts $m1 "home $masterdir"
	puts $m1 "local $master_port"
	puts $m1 "output $testdir/m1output"
	puts $m1 "open_env"
	puts $m1 "start master"
	gets $m1

	if {$on_the_fly} {
		set m2 [open "| $site_prog 2>erroutp" "r+"]
		fconfigure $m2 -buffering line
		puts $m2 "home $masterdir"
		puts $m2 "local $master_port"
		puts $m2 "output $testdir/m2output"
		puts $m2 "open_env"
		puts $m2 "start master"
		gets $m2
	}

	puts "\tRepmgr$tnum.c: Start 1st client process, but not connected."
	make_dbconfig $clientdir {{rep_set_nsites 3}}
	set c1 [open "| $site_prog 2> c1stderr" "r+"]
	fconfigure $c1 -buffering line
	puts $c1 "home $clientdir"
	puts $c1 "local $client_port"
	puts $c1 "output $testdir/c1output"
	puts $c1 "open_env"
	puts $c1 "start client"
	gets $c1

	# Obviously, at this point the two sites aren't connected, since neither
	# one has been told about the other.

	puts "\tRepmgr$tnum.d: Start 2nd client process, connecting to master."
	set c2 [open "| $site_prog" "r+"]
	fconfigure $c2 -buffering line
	puts $c2 "home $clientdir"
	puts $c2 "local $client_port"
	puts $c2 "remote localhost $master_port"
	puts $c2 "output $testdir/c2output"
	puts $c2 "open_env"
	puts $c2 "start client"
	gets $c2
	
	# At this point, c2 will connect to the master.  (The truly observant
	# reader will notice that there's essentially no point to this
	# connection, at least as long as this site remains a client.  But if it
	# ever becomes master, this connection will be ready to go, for sending
	# log records from live updates originating at that process.)
	#
	# The master should extract the site address from the handshake,
	# recognize that this is a subordinate connection, and therefore
	# initiate an outgoing connection to the client.  It also of course
	# stashes the site's address in the shared region, so that any
	# subordinate processes (m2) can find it.
	# 
	# Check site list, to make sure A discovers B's network address.  Then
	# wait for startup-done. 
	#
	set cond {
		set masterenv [berkdb_env -home $masterdir]
		set msl [$masterenv repmgr_site_list]
		$masterenv close
		expr {[llength $msl] == 1 && [lindex $msl 0 2] == $client_port}
	}
	await_condition {[eval $cond]} 20

	set clientenv [berkdb_env -home $clientdir]
	await_startup_done $clientenv

	if {!$on_the_fly} {
		set m2 [open "| $site_prog 2>erroutp" "r+"]
		fconfigure $m2 -buffering line
		puts $m2 "home $masterdir"
		puts $m2 "local $master_port"
		puts $m2 "output $testdir/m2output"
		puts $m2 "open_env"
		puts $m2 "start master"
		gets $m2
		tclsleep 2
	}

	puts $m2 "open_db test.db"
	puts $m2 "put k1 v1"
	puts $m2 "echo done"
	gets $m2

	# make sure there weren't too many perm failures
	puts "\tRepmgr$tnum.e: Check stats"
	set get_pfs { expr [stat_field \
			    $clientenv repmgr_stat "Acknowledgement failures"] }
	set pfs [eval $get_pfs]

	if {$on_the_fly} {
		set max 1
	} else {
		set max 0
	}
	if {$pfs > $max} {
		error "FAIL: too many perm failures"
	}

	puts $m1 "open_db test.db"
	puts $m1 "put k2 v2"
	puts $m1 "echo done"
	gets $m1
	
	puts "\tRepmgr$tnum.f: Check that replicated data is visible at client."
	set expected {{k1 v1} {k2 v2}}
	verify_client_data $clientenv test.db $expected

	# make sure there were no additional perm failures
	puts "\tRepmgr$tnum.g: Check stats again"
	set pfs2 [eval $get_pfs]
	error_check_good subsequent $pfs2 $pfs

	puts "\tRepmgr$tnum.h: Clean up."
	$clientenv close
	close $c1
	close $c2
	close $m1
	close $m2
}
