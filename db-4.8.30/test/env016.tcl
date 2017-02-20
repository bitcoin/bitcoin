# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	env016
# TEST	Replication settings and DB_CONFIG
# TEST
# TEST	Create a DB_CONFIG for various replication settings.  Use
# TEST	rep_stat or getter functions to verify they're set correctly.
#
proc env016 { } {
	source ./include.tcl

	puts "Env016: Replication DB_CONFIG settings."

	#
	# Test options that we query via rep_stat.
	# Structure of the list is:
	#	0. Arg used in DB_CONFIG.
	#	1. Value assigned in DB_CONFIG.
	#	2. Message output during test.
	#	3. String to search for in stat output.
	#
	set slist {
		{ "rep_set_priority" "1" "Env016.a0: Priority"
		    "Environment priority" }
	}
	puts "\tEnv016.a: Check settings via rep_stat."
	foreach l $slist {
		set carg [lindex $l 0]
		set val [lindex $l 1]
		set msg [lindex $l 2]
		set str [lindex $l 3]
		env_cleanup $testdir
		replsetup $testdir/MSGQUEUEDIR
		set masterdir $testdir/MASTERDIR
		file mkdir $masterdir
		repladd 1

		# Open a master.
		puts "\t\t$msg"
		#
		# Create DB_CONFIG.
		#
		env016_make_config $masterdir $carg $val
		#
		# Open env.
		#
		set ma_envcmd "berkdb_env_noerr -create -txn nosync \
		    -home $masterdir -errpfx MASTER -rep_master \
		    -rep_transport \[list 1 replsend\]"
		set masterenv [eval $ma_envcmd]
		#
		# Verify value
		#
		set gval [stat_field $masterenv rep_stat $str]
		error_check_good stat_get $gval $val

		error_check_good masterenv_close [$masterenv close] 0
		replclose $testdir/MSGQUEUEDIR
	}

	# Test options that we query via getter functions.
	# Structure of the list is:
	#	0. Arg used in DB_CONFIG.
	#	1. Value assigned in DB_CONFIG.
	#	2. Message output during test.
	#	3. Getter command.
	#	4. Getter results expected if different from #1 value.
	set glist {
		{ "rep_set_clockskew" "102 100" "Env016.b0: Rep clockskew"
		    "rep_get_clockskew" }
		{ "rep_set_config" "db_rep_conf_bulk" "Env016.b1: Rep config"
		    "rep_get_config bulk" "1" }
		{ "rep_set_limit" "0 1048576" "Env016.b2: Rep limit"
		    "rep_get_limit" }
		{ "rep_set_nsites" "6" "Env016.b3: Rep nsites"
		    "rep_get_nsites" }
		{ "rep_set_request" "4000 128000" "Env016.b4: Rep request"
		    "rep_get_request" }
		{ "rep_set_timeout" "db_rep_election_timeout 500000"
		    "Env016.b5: Rep elect timeout" "rep_get_timeout election" 
		    "500000" }
		{ "rep_set_timeout" "db_rep_checkpoint_delay 500000"
		    "Env016.b6: Rep ckp timeout"
		    "rep_get_timeout checkpoint_delay" "500000" }
	}
	puts "\tEnv016.b: Check settings via getter functions."
	foreach l $glist {
		set carg [lindex $l 0]
		set val [lindex $l 1]
		set msg [lindex $l 2]
		set getter [lindex $l 3]
		if { [llength $l] > 4 } {
			set getval [lindex $l 4]
		} else {
			set getval $val
		}
		env_cleanup $testdir
		replsetup $testdir/MSGQUEUEDIR
		set masterdir $testdir/MASTERDIR
		file mkdir $masterdir
		repladd 1

		# Open a master.
		puts "\t\t$msg"
		#
		# Create DB_CONFIG.
		#
		env016_make_config $masterdir $carg $val
		#
		# Open env.
		#
		set ma_envcmd "berkdb_env_noerr -create -txn nosync \
		    -home $masterdir -errpfx MASTER -rep_master \
		    -rep_transport \[list 1 replsend\]"
		set masterenv [eval $ma_envcmd]
		#
		# Verify value
		#
		set gval [eval $masterenv $getter]
		error_check_good stat_get $gval $getval

		error_check_good masterenv_close [$masterenv close] 0
		replclose $testdir/MSGQUEUEDIR
	}
}

proc env016_make_config { dir carg cval } {
	set cid [open $dir/DB_CONFIG w]
	puts $cid "$carg $cval"
	close $cid
}
