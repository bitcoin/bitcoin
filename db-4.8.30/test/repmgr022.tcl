# See the file LICENSE for redistribution information.
#
# Copyright (c)-2009 Oracle.  All rights reserved.
#

# TEST repmgr022
# TEST Basic test of repmgr's multi-process master support.
# TEST
# TEST Set up a simple 2-site group, create data and replicate it.
# TEST Add a second process at the master and have it write some
# TEST updates.  It does not explicitly start repmgr (nor do any
# TEST replication configuration, for that matter).  Its first
# TEST update triggers initiation of connections, and so it doesn't
# TEST get to the client without a log request.  But later updates
# TEST should go directly.

proc repmgr022 {  } {
	source ./include.tcl
	global rep_verbose
	global verbose_type

	set tnum "022"
	puts "Repmgr$tnum: Basic repmgr multi-process master support."
	set site_prog [setup_site_prog]

	env_cleanup $testdir

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR

	file mkdir $masterdir
	file mkdir $clientdir

	set ports [available_ports 2]
	set master_port [lindex $ports 0]
	set client_port [lindex $ports 1]

	puts "\tRepmgr$tnum.a: Set up the master (on TCP port $master_port)."
	set master [open "| $site_prog" "r+"]
	fconfigure $master -buffering line
	puts $master "home $masterdir"
	puts $master "local $master_port"
	make_dbconfig $masterdir {{rep_set_nsites 3}}
	puts $master "output $testdir/m1output"
	puts $master "open_env"
	puts $master "start master"
	error_check_match start_master [gets $master] "*Successful*"
	puts $master "open_db test.db"
	puts $master "put myKey myValue"

	puts "\tRepmgr$tnum.b: Set up the client (on TCP port $client_port)."
	set client [open "| $site_prog" "r+"]
	fconfigure $client -buffering line
	puts $client "home $clientdir"
	puts $client "local $client_port"
	make_dbconfig $clientdir {{rep_set_nsites 3}}
	puts $client "output $testdir/coutput"
	puts $client "open_env"
	puts $client "remote localhost $master_port"
	puts $client "start client"
	error_check_match start_client [gets $client] "*Successful*"

	puts "\tRepmgr$tnum.c: Wait for STARTUPDONE."
	set clientenv [berkdb_env -home $clientdir]
	await_startup_done $clientenv
	  
	puts "\tRepmgr$tnum.d: Start a second process at master."
	set m2 [open "| $site_prog" "r+"]
	fconfigure $m2 -buffering line
	puts $m2 "home $masterdir"
	puts $m2 "output $testdir/m2output"
	puts $m2 "open_env"
	puts $m2 "open_db test.db"
	puts $m2 "put sub1 abc"
	puts $m2 "echo firstputted"
	set sentinel [gets $m2]
	error_check_good m2_firstputted $sentinel "firstputted"
	puts $m2 "put sub2 xyz"
	puts $m2 "put sub3 ijk"
	puts $m2 "put sub4 pqr"
	puts $m2 "echo putted"
	set sentinel [gets $m2]
	error_check_good m2_putted $sentinel "putted"
	puts $master "put another record"
	puts $master "put and again"
	puts $master "echo m1putted"
	set sentinel [gets $master]
	error_check_good m1_putted $sentinel "m1putted"

	puts "\tRepmgr$tnum.e: Check that replicated data is visible at client."
	puts $client "open_db test.db"
	set expected {{myKey myValue} {sub1 abc} {sub2 xyz} {another record}}
	verify_client_data $clientenv test.db $expected

	# make sure there weren't too many rerequests
	puts "\tRepmgr$tnum.f: Check rerequest stats"
	set pfs [stat_field $clientenv rep_stat "Log records requested"]
	error_check_good rerequest_count [expr $pfs <= 1] 1

	puts "\tRepmgr$tnum.g: Clean up."
	$clientenv close
	close $client
	close $master
	close $m2
}
