# See the file LICENSE for redistribution information.
#
# Copyright (c)-2009 Oracle.  All rights reserved.
#
# TEST repmgr027
# TEST Repmgr recognition of peer setting, across processes.
# TEST
# TEST Set up a master and two clients, synchronized with some data.
# TEST Add a new client, configured to use c2c sync with one of the original
# TEST clients.  Check stats to make sure the correct c2c peer was used.

proc repmgr027 { } {
	repmgr027_sub position_chg
	repmgr027_sub chg_site
	repmgr027_sub chg_after_open
	repmgr027_sub set_peer_after_open
}

proc repmgr027_sub { config } {
	source ./include.tcl

	set tnum "027" 
	puts "Repmgr$tnum: Repmgr peer, with \"$config\" configuration."
	set site_prog [setup_site_prog]

	env_cleanup $testdir

	set ports [available_ports 4]
	set mport [lindex $ports 0]
	set portA [lindex $ports 1]
	set portB [lindex $ports 2]

	file mkdir [set masterdir $testdir/MASTER]
	file mkdir $testdir/A
	file mkdir $testdir/B
	file mkdir $testdir/C

	puts "\tRepmgr$tnum.a: Start master, write some data."
	make_dbconfig $masterdir {{rep_set_nsites 4}}
	set cmds {
		"home $masterdir"
		"local $mport"
		"output $testdir/moutput"
		"open_env"
		"start master"
		"open_db test.db"
		"put key1 value1"
	}
	set m [open_site_prog [subst $cmds]]

	puts "\tRepmgr$tnum.b:\
	    Start initial two clients; wait for them to synchronize."
	# Allowing both A and B to start at the same time, and synchronize
	# concurrently would make sense.  But it causes very slow performance on
	# Windows.  Since it's really only client C that's under test here, this
	# detail doesn't matter.
	# 
	make_dbconfig $testdir/A {{rep_set_nsites 4}}
	set a [open_site_prog [list \
			       "home $testdir/A" \
			       "local $portA" \
			       "output $testdir/aoutput" \
			       "remote localhost $mport" \
			       "open_env" \
			       "start client"]]
	set env [berkdb_env -home $testdir/A]
	await_startup_done $env
	$env close

	make_dbconfig $testdir/B {{rep_set_nsites 4}}
	set b [open_site_prog [list  \
			       "home $testdir/B" \
			       "local $portB" \
			       "output $testdir/boutput" \
			       "remote localhost $mport" \
			       "open_env" \
			       "start client"]]
	set env [berkdb_env -home $testdir/B]
	await_startup_done $env
	$env close

	# Client C is the one whose behavior is being tested.  It has two
	# processes.  "c" will be the main replication process, and "c2" the
	# subordinate process.  The initial configuration commands used to set
	# up the two processes vary slightly with each test.  The variable
	# $config contains the name of the proc which will fill out the
	# configuration information appropriately for each test variant.
	#
	puts "\tRepmgr$tnum.c: Start client under test."
	make_dbconfig $testdir/C {{rep_set_nsites 4}}

	set c2 [list \
		    "home $testdir/C" \
		    "local [lindex $ports 3]" \
		    "output $testdir/c2output" \
		    "open_env"]
	set c [list \
		   "home $testdir/C" \
		   "local [lindex $ports 3]" \
		   "output $testdir/coutput" \
		   "open_env"]
	set lists [repmgr027_$config $c2 $c]
	set c2 [lindex $lists 0]
	set c [lindex $lists 1]

	# Ugly hack: in this one case, the order of opening the two client
	# processes has to be reversed.
	#
	if {$config == "chg_after_open"} {
		set c [open_site_prog $c]
		set c2 [open_site_prog $c2]
	} else {
		set c2 [open_site_prog $c2]
		set c [open_site_prog $c]
	}
	puts $c "start client"
	gets $c

	puts "\tRepmgr$tnum.d: Wait for startup-done at test client."
	set env [berkdb_env -home $testdir/C]
	await_startup_done $env 27
	$env close

	puts "\tRepmgr$tnum.e: Check stats to make sure proper peer was used."
	set env [berkdb_env -home $testdir/A]
	set reqs [stat_field $env rep_stat "Client service requests"]
	error_check_good used_client_A [expr {$reqs > 0}] 1
	$env close
	set env [berkdb_env -home $testdir/B]
	set reqs [stat_field $env rep_stat "Client service requests"]
	error_check_good didnt_use_b [expr {$reqs == 0}] 1
	$env close

	puts "\tRepmgr$tnum.f: Clean up."
	close $c2
	close $c
	close $b
	close $a
	close $m
}

# Scenario 1: client A is the peer; C2 sets B, A; C sets A.  For C, this means
# no peer change, but its position in the list changes, requiring some tricky
# shuffling.
#
proc repmgr027_position_chg { c2 c } {
	set remote_config [uplevel 1 {list \
			   "remote localhost $mport" \
			   "remote localhost $portB" \
			   "remote -p localhost $portA"}]
	set i [lsearch -exact $c2 "open_env"]

	# It should be found, in the middle somewhere, or this will break.
	set c2 "[lrange $c2 0 [expr $i - 1]] $remote_config [lrange $c2 $i end]"

	set remote_config [uplevel 1 {list \
			       "remote -p localhost $portA" \
					  "remote localhost $mport"}]
	set i [lsearch -exact $c "open_env"]
	set c "[lrange $c 0 [expr $i - 1]] $remote_config [lrange $c $i end]"

	return [list $c2 $c]
}

# C2 first sets the peer as B, but then C comes along and changes it to A.
#
proc repmgr027_chg_site { c2 c } {
	set remote_config [uplevel 1 {list \
			   "remote localhost $mport" \
			   "remote -p localhost $portB"}]
	set i [lsearch -exact $c2 "open_env"]

	# It should be found, in the middle somewhere, or this will break.
	set c2 "[lrange $c2 0 [expr $i - 1]] $remote_config [lrange $c2 $i end]"

	set remote_config [uplevel 1 {list \
			       "remote -p localhost $portA" \
					  "remote localhost $mport"}]
	set i [lsearch -exact $c "open_env"]
	set c "[lrange $c 0 [expr $i - 1]] $remote_config [lrange $c $i end]"

	return [list $c2 $c]
}

# C first sets B as its peer, and creates the env.  Then C2 comes along and
# changes it to A.  C will have to learn of the change on the fly, rather than
# at env open/join time.  Even though the actual order of process creation will
# be reversed (by the caller), we still conform to the convention of putting C2
# first, and then C, in the ordered list.
# 
proc repmgr027_chg_after_open { c2 c } {
	set remote_config [uplevel 1 {list \
			   "remote localhost $mport" \
			   "remote -p localhost $portA"}]
	set i [lsearch -exact $c2 "open_env"]

	# It should be found, in the middle somewhere, or this will break.
	set c2 "[lrange $c2 0 [expr $i - 1]] $remote_config [lrange $c2 $i end]"

	set remote_config [uplevel 1 {list \
			       "remote -p localhost $portB" \
					  "remote localhost $mport"}]
	set i [lsearch -exact $c "open_env"]
	set c "[lrange $c 0 [expr $i - 1]] $remote_config [lrange $c $i end]"

	return [list $c2 $c]
}

# Nothing especially exotic here, except this exercises a code path where I
# previously discovered a bug.
# 
proc repmgr027_set_peer_after_open { c2 c } {
	set remote_config [uplevel 1 {subst "remote -p localhost $portA"}]
	lappend c $remote_config
	return [list $c2 $c]
}
