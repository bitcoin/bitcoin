# See the file LICENSE for redistribution information.
#
# Copyright (c)-2009 Oracle.  All rights reserved.
#
# TEST repmgr026
# TEST Repmgr site address discovery via NEWSITE
# TEST
# TEST New client, previously unknown to (main) master process, connects to an
# TEST existing client, which broadcasts NEWSITE message.
# TEST This causes master to discover its address, and connect to it.
# TEST Other new clients may also have been added to master's configuration in
# TEST the interim (via a subordinate master process).

proc repmgr026 { } {
	foreach sitelist { {} {2} {3 2} {3 2 4} {3} } {
		repmgr026_sub $sitelist
	}
}

proc repmgr026_sub { extras } {
	source ./include.tcl

	set tnum "026"
	puts "Repmgr$tnum:\
	    Repmgr and NEWSITE, with these extra clients: {$extras}"
	set site_prog [setup_site_prog]

	env_cleanup $testdir

	set ports [available_ports 5]
	set master_port [lindex $ports 0]
	set portB [lindex $ports 1]
	set portC [lindex $ports 2]
	
	set i 0
	foreach site_id "A B C D E" {
		set d "$testdir/$site_id"
		set dirs($i) $d
		file mkdir $d
		incr i
	}
	set masterdir $dirs(0)
	set dirB $dirs(1)
	set dirC $dirs(2)

	puts "\tRepmgr$tnum.a: Start 2 processes at master."
	make_dbconfig $masterdir {{rep_set_nsites 5}
		{rep_set_timeout DB_REP_CONNECTION_RETRY 300000000}}
	set m1 [open "| $site_prog" "r+"]
	fconfigure $m1 -buffering line
	puts $m1 "home $masterdir"
	puts $m1 "local $master_port"
	puts $m1 "output $testdir/m1output"
	puts $m1 "open_env"
	puts $m1 "start master"
	gets $m1

	set m2 [open "| $site_prog" "r+"]
	fconfigure $m2 -buffering line
	puts $m2 "home $masterdir"
	puts $m2 "output $testdir/m2output"
	puts $m2 "open_env"
	puts $m2 "echo done"
	gets $m2

	puts "\tRepmgr$tnum.b: Start client B, connecting to master."
	make_dbconfig $dirB {{rep_set_nsites 5}}
	set b [open "| $site_prog" "r+"]
	fconfigure $b -buffering line
	puts $b "home $dirB"
	puts $b "local $portB"
	puts $b "remote localhost $master_port"
	puts $b "output $testdir/boutput"
	puts $b "open_env"
	puts $b "start client"
	gets $b
	
	set envB [berkdb_env -home $dirB]
	await_startup_done $envB

	# Add some newly arrived client configurations to the master, but do it
	# via the subordinate process A2, so that the main process doesn't
	# notice them until it gets the NEWSITE message.  Note that these
	# clients aren't running yet.
	# 
	puts "\tRepmgr$tnum.c: Add new client addresses at master."
	foreach client_index $extras {
		puts $m2 "remote localhost [lindex $ports $client_index]"
		if {$client_index == 2} {
			# Start client C last, below.
			continue;
		}
		make_dbconfig $dirs($client_index) {{rep_set_nsites 5}}
		set x [open "| $site_prog" "r+"]
		fconfigure $x -buffering line
		puts $x "home $dirs($client_index)"
		puts $x "local [lindex $ports $client_index]"
		puts $x "output $testdir/c${client_index}output"
		puts $x "open_env"
		puts $x "start client"
		gets $x
		set sites($client_index) $x
	}
	puts $m2 "echo done"
	gets $m2

	# Start client C, triggering the NEWSITE mechanism at the master.
	#
	puts "\tRepmgr$tnum.d: Start client C, connecting first only to B."
	make_dbconfig $dirC {{rep_set_nsites 5}}
	set c [open "| $site_prog" "r+"]
	fconfigure $c -buffering line
	puts $c "home $dirC"
	puts $c "local $portC"
	puts $c "remote localhost $portB"
	puts $c "output $testdir/coutput"
	puts $c "open_env"
	puts $c "start client"
	gets $c
	
	# First check for startup-done at site C.
	# 
	set envC [berkdb_env -home $dirC]
	await_startup_done $envC 15
	$envC close

	# Then check for startup-done at any other clients.
	# 
	foreach client_index $extras {
		if {$client_index == 2} {
			continue;
		}
		set envx [berkdb_env -home $dirs($client_index)]
		await_startup_done $envx 20
		$envx close
	}

	# If we've gotten this far, we know that everything must have worked
	# fine, because otherwise the clients wouldn't have been able to
	# complete their start-up.  But let's check the master's site list
	# anyway, to make sure it knows about site C at the list index location
	# we think should be correct.
	#
	# Site C's address appears at the position indicated in the "extras"
	# list, or right after that if it didn't appear in extras.
	# 
	set pos [lsearch -exact $extras 2]
	if {$pos == -1} {
		set pos [llength $extras]
	}
	incr pos;			# make allowance for site B which is
					# always there

	puts "\tRepmgr$tnum.e: Check address lists."
	set masterenv [berkdb_env -home $masterdir]
	error_check_good master_knows_clientC \
	    [lindex [$masterenv repmgr_site_list] $pos 2 ] $portC

	set envC [berkdb_env -home $dirC]
	error_check_good C_knows_master \
	    [lindex [$envC repmgr_site_list] 1 2] $master_port

	puts "\tRepmgr$tnum.f: Clean up."
	$envC close
	$envB close
	$masterenv close

	close $c 

	foreach client_index $extras {
		if {$client_index == 2} {
			continue;
		}
		close $sites($client_index)
	}
	close $b
	close $m2
	close $m1
}
