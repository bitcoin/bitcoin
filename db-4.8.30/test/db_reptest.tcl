# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999,2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	db_reptest
# TEST	Wrapper to configure and run the db_reptest program.

#
# TODO:
# late client start.
# Number of message proc threads.
#

global last_nsites
set last_nsites 0

#
# There are 3 user-level procs that the user may invoke.
# 1. db_reptest - Runs randomized configurations in a loop.
# 2. basic_db_reptest - Runs a simple set configuration once,
#	as a smoke test.
# 3. restore_db_reptest 'dir' - Runs the configuration given in 'dir'
#	in a loop.  The purpose is either to reproduce a problem
#	that some configuration encountered, or test a fix.
#

#
# db_reptest - Run a randomized configuration.  Run the test
# 'count' times in a loop, or if no count it given, it is
# an infinite loop.
#
proc db_reptest { {count -1} } {
	global rand_init

	berkdb srand $rand_init
	set cmd "db_reptest_int random"
	db_reptest_loop $cmd $count
}

#
# Run a basic reptest.  The types are:
# Basic 0 - Two sites, start with site 1 as master, 5 worker threads, btree,
#	run 100 seconds, onesite remote knowledge.
# Basic 1 - Three sites, all sites start as client, 5 worker threads, btree
#	run 150 seconds, full remote knowledge.
#
proc basic_db_reptest { { basic 0 } } {
	global util_path

	if { [file exists $util_path/db_reptest] == 0 } {
		puts "Skipping db_reptest.  Is it built?"
		return
	}
	if { $basic == 0 } {
		db_reptest_int basic0
	}
	if { $basic == 1 } {
		db_reptest_int basic1
	}
}

#
# Restore a configuration from the given directory and
# run that configuration in a loop 'count' times.
#
proc restore_db_reptest { restoredir { count -1 } } {
	set cmd "db_reptest_int restore $restoredir/SAVE_RUN"
	db_reptest_loop $cmd $count
}

#
# Wrapper to run the command in a loop, 'count' times.
#
proc db_reptest_loop { cmd count } {
	global util_path

	if { [file exists $util_path/db_reptest] == 0 } {
		puts "Skipping db_reptest.  Is it built?"
		return
	}
	set iteration 1
	while { 1 } {
		puts -nonewline "ITERATION $iteration: "
		puts [clock format [clock seconds] -format "%H:%M %D"]

		#
		eval $cmd

		puts -nonewline "COMPLETED $iteration: "
		puts [clock format [clock seconds] -format "%H:%M %D"]
		incr iteration
		if { $count > 0 && $iteration > $count } {
			break
		}
	}
}

#
# Internal version of db_reptest that all user-level procs
# eventually call.  It will configure a single run of
# db_reptest based on the configuration type specified
# in 'cfgtype'.  This proc will:
# Configure a run of db_reptest
# Run db_reptest
# Verify the sites after db_reptest completes.
#
proc db_reptest_int { cfgtype { restoredir NULL } } {
	source ./include.tcl
	global rporttype

	env_cleanup $testdir

	set savedir TESTDIR/SAVE_RUN
	reptest_cleanup $savedir

	#
	# Get all the default or random values needed for the test
	# and its args first.
	#
	set runtime 0
	set kill 0
	#
	# Get number of sites first because pretty much everything else
	# after here depends on how many sites there are.
	#
	set num_sites [get_nsites $cfgtype $restoredir]
	set use_lease [get_lease $cfgtype $restoredir]
	#
	# Only use kill if we have > 2 sites.
	# Returns the site number of the site to kill, or 0
	# if this will not be a kill test.
	#
	if { $num_sites > 2 } {
		set kill [get_kill $cfgtype $restoredir $num_sites]
	}
	if { $cfgtype != "restore" } {
		if { $use_lease } {
			set use_master 0
		} else {
			set use_master [get_usemaster $cfgtype]
		}
		set master_site [get_mastersite $cfgtype $use_master $num_sites]
		set workers [get_workers $cfgtype $use_lease]
		set dbtype [get_dbtype $cfgtype]
		set runtime [get_runtime $cfgtype]
		set use_peers [get_peers $cfgtype]
		puts -nonewline "Running: $num_sites sites, $runtime seconds "
		if { $kill } {
			puts -nonewline "kill site $kill "
		}
		if { $use_lease } {
			puts "with leases"
		} elseif { $use_master } {
			puts "master site $master_site"
		} else {
			puts "no master"
		}
	}
	set baseport 6100
	set rporttype NULL
	#
	# This loop sets up the args to the invocation of db_reptest
	# for each site.
	#
	for { set i 1 } {$i <= $num_sites } { incr i } {
		set envdirs($i) TESTDIR/ENV$i
		reptest_cleanup $envdirs($i)
		#
		# If we are restoring the args, just read them from the
		# saved location for this sites.  Otherwise build up
		# the args for each piece we need.
		#
		if { $cfgtype == "restore" } {
			set cid [open $restoredir/DB_REPTEST_ARGS.$i r]
			set prog_args($i) [read $cid]
			close $cid
			if { $runtime == 0 } {
				set runtime [parse_runtime $prog_args($i)]
				puts "Runtime: $runtime"
			}
		} else {
			set prog_args($i) \
			    "-v -c $workers -t $dbtype -T $runtime "
			set prog_args($i) \
			    [concat $prog_args($i) "-h $envdirs($i)"]
			#
			# Add in if this site should kill itself.
			#
			if { $kill == $i } {
				set prog_args($i) [concat $prog_args($i) "-k"]
			}
			#
			# Add in if this site starts as a master or client.
			#
			if { $i == $master_site } {
				set state($i) MASTER
				set prog_args($i) [concat $prog_args($i) "-M"]
			} else {
				set state($i) CLIENT
				#
				# If we have a master, then we just want to
				# start as a client.  Otherwise start with
				# elections.
				#
				if { $use_master } {
					set prog_args($i) \
					    [concat $prog_args($i) "-C"]
				} else {
					set prog_args($i) \
					    [concat $prog_args($i) "-E"]
				}
			}
			#
			# Add in host:port configuration, both this site's
			# local address and any remote addresses it knows.
			#
			set lport($i) [expr $baseport + $i]
			set prog_args($i) \
			    [concat $prog_args($i) "-l localhost:$lport($i)"]
			set rport($i) [get_rport $baseport $i \
			    $num_sites $cfgtype]
			if { $use_peers } {
				set remote_arg "-R"
			} else {
				set remote_arg "-r"
			}
			foreach p $rport($i) {
				set prog_args($i) \
				    [concat $prog_args($i) $remote_arg \
				    "localhost:$p"]
			}
		}
		save_db_reptest $savedir ARGS $i $prog_args($i)
	}

	# Now make the DB_CONFIG file for each site.
	reptest_make_config $savedir $num_sites envdirs state \
	    $use_lease $cfgtype $restoredir

	# Run the test
	run_db_reptest $savedir $num_sites $runtime
	puts "Test run complete.  Verify."

	# Verify the test run.
	verify_db_reptest $num_sites envdirs $kill

}

#
# Make a DB_CONFIG file for all sites in the group
#
proc reptest_make_config { savedir nsites edirs st lease cfgtype restoredir } {
	upvar $edirs envdirs
	upvar $st state

	#
	# Generate global config values that should be the same
	# across all sites, such as number of sites and log size, etc.
	#
	set default_cfglist {
	{ "rep_set_nsites" $nsites }
	{ "rep_set_request" "150000 2400000" }
	{ "rep_set_timeout" "db_rep_checkpoint_delay 0" }
	{ "rep_set_timeout" "db_rep_connection_retry 2000000" }
	{ "rep_set_timeout" "db_rep_heartbeat_monitor 1000000" }
	{ "rep_set_timeout" "db_rep_heartbeat_send 500000" }
	{ "set_cachesize"  "0 536870912 1" }
	{ "set_lg_max" "131072" }
	{ "set_lk_detect" "db_lock_default" }
	{ "set_verbose" "db_verb_recovery" }
	{ "set_verbose" "db_verb_replication" }
	}

	set acks { db_repmgr_acks_all db_repmgr_acks_all_peers \
	    db_repmgr_acks_none db_repmgr_acks_one db_repmgr_acks_one_peer \
	    db_repmgr_acks_quorum }

	#
	# Ack policy must be the same on all sites.
	#
	if { $cfgtype == "random" } {
		if { $lease } {
			set ackpolicy db_repmgr_acks_quorum
		} else {
			set done 0
			while { $done == 0 } {
				set acksz [expr [llength $acks] - 1]
				set myack [berkdb random_int 0 $acksz]
				set ackpolicy [lindex $acks $myack]
				#
				# Only allow the "none" policy with 2 sites
				# otherwise it can overwhelm the system and
				# it is a rarely used option.
				#
				if { $ackpolicy == "db_repmgr_acks_none" && \
				    $nsites > 2 } {
					continue
				}
				set done 1
			}
		}
	} else {
		set ackpolicy db_repmgr_acks_one
	}
	for { set i 1 } { $i <= $nsites } { incr i } {
		#
		# If we're restoring we just need to copy it.
		#
		if { $cfgtype == "restore" } {
			file copy $restoredir/DB_CONFIG.$i \
			    $envdirs($i)/DB_CONFIG
			file copy $restoredir/DB_CONFIG.$i \
			    $savedir/DB_CONFIG.$i
			continue
		}
		#
		# Otherwise set up per-site config information
		#
		set cfglist $default_cfglist

		#
		# Add lease configuration if needed.  We're running all
		# locally, so there is no clock skew.
		#
		if { $lease } {
			#
			# We need to have an ack timeout > lease timeout.
			# Otherwise txns can get committed without waiting
			# long enough for leases to get granted.
			#
			lappend cfglist { "rep_set_config" "db_rep_conf_lease" }
			lappend cfglist { "rep_set_timeout" \
			    "db_rep_lease_timeout 10000000" }
			lappend cfglist \
			    { "rep_set_timeout" "db_rep_ack_timeout 20000000" }
		} else {
			lappend cfglist \
			    { "rep_set_timeout" "db_rep_ack_timeout 5000000" }
		}

		#
		# Priority
		#
		if { $state($i) == "MASTER" } {
			lappend cfglist { "rep_set_priority" 100 }
		} else {
			if { $cfgtype == "random" } {
				set pri [berkdb random_int 10 25]
			} else {
				set pri 20
			}
			lappend cfglist { "rep_set_priority" $pri }
		}
		#
		# Others: limit size, bulk, 2site strict,
		#
		if { $cfgtype == "random" } {
			set limit_sz [berkdb random_int 15000 1000000]
			set bulk [berkdb random_int 0 1]
			if { $bulk } {
				lappend cfglist \
				    { "rep_set_config" "db_rep_conf_bulk" }
			}
			if { $nsites == 2 } {
				set strict [berkdb random_int 0 1]
				if { $strict } {
					lappend cfglist { "rep_set_config" \
					    "db_repmgr_conf_2site_strict" }
				}
			}
		} else {
			set limit_sz 100000
		}
		lappend cfglist { "rep_set_limit" "0 $limit_sz" }
		lappend cfglist { "repmgr_set_ack_policy" $ackpolicy }
		set cid [open $envdirs($i)/DB_CONFIG a]
		foreach c $cfglist {
			set carg [subst [lindex $c 0]]
			set cval [subst [lindex $c 1]]
			puts $cid "$carg $cval"
		}
		close $cid
		set cid [open $envdirs($i)/DB_CONFIG r]
		set cfg [read $cid]
		close $cid
	
		save_db_reptest $savedir CONFIG $i $cfg
	}

}

proc reptest_cleanup { dir } {
	#
	# For now, just completely remove it all.  We might want
	# to use env_cleanup at some point in the future.
	#
	fileremove -f $dir
	file mkdir $dir
}


proc save_db_reptest { savedir op site savelist } {
	#
	# Save a copy of the configuration and args used to run this
	# instance of the test.
	#
	if { $op == "CONFIG" } {
		set outfile $savedir/DB_CONFIG.$site
	} else {
		set outfile $savedir/DB_REPTEST_ARGS.$site
	}
	set cid [open $outfile a]
	puts -nonewline $cid $savelist
	close $cid
}

proc run_db_reptest { savedir numsites runtime } {
	source ./include.tcl
	global killed_procs

	set pids {}
	for {set i 1} {$i <= $numsites} {incr i} {
		lappend pids [exec $tclsh_path $test_path/wrap_reptest.tcl \
		    $savedir/DB_REPTEST_ARGS.$i $savedir/site$i.log &]
		tclsleep 1
	}
	watch_procs $pids 15 [expr $runtime * 3]
	set killed [llength $killed_procs]
	if { $killed > 0 } {
		error "Processes $killed_procs never finished"
	}
}

proc verify_db_reptest { num_sites edirs kill } {
	upvar $edirs envdirs

	set startenv 1
	set cmpeid 2
	if { $kill == 1 } {
		set startenv 2
		set cmpeid 3
	}
	set envbase [berkdb_env_noerr -home $envdirs($startenv)]
	for { set i $cmpeid } { $i <= $num_sites } { incr i } {
		if { $i == $kill } {
			continue
		}
		set cmpenv [berkdb_env_noerr -home $envdirs($i)]
		puts "Compare $envdirs($startenv) with $envdirs($i)"
		#
		# Compare 2 envs.  We assume the name of the database that
		# db_reptest creates and know it is 'am1.db'.
		# We want as other args:
		# 0 - compare_shared_portion
		# 1 - match databases
		# 0 - don't compare logs (for now)
		rep_verify $envdirs($startenv) $envbase $envdirs($i) $cmpenv \
		    0 1 0 am1.db
		$cmpenv close
	}
	$envbase close
}

proc get_nsites { cfgtype restoredir } {
	global last_nsites

	#
	# The number of sites must be the same for all.  Read the
	# first site's saved DB_CONFIG file if we're restoring since
	# we only know we have at least 1 site.
	#
	if { $cfgtype == "restore" } {
		set cid [open $restoredir/DB_CONFIG.1 r]
		while { [gets $cid cfglist] } {
			puts "Read in: $cfglist"
			set cfg [lindex $cfglist 0]
			if { $cfg == "rep_set_nsites" } {
				set num_sites [lindex $cfglist 1]
				break;
			}
		}
		close $cid
		return $num_sites
	}
	if { $cfgtype == "random" } {
		#
		# Sometimes 'random' doesn't seem to do a good job.  I have
		# seen on all iterations after the first one, nsites is
		# always 2, 100% of the time.  Add this bit to make sure
		# this nsites values is different from the last iteration.
		#
		set n [berkdb random_int 2 5]
		while { $n == $last_nsites } {
			set n [berkdb random_int 2 5]
puts "Getting random nsites between 2 and 5.  Got $n, last_nsites $last_nsites"
		}
		set last_nsites $n
		return $n
#		return [berkdb random_int 2 5]
	}
	if { $cfgtype == "basic0" } {
		return 2
	}
	if { $cfgtype == "basic1" } {
		return 3
	}
	return -1
}

#
# Run with master leases?  25%/75% (use a master lease 25% of the time).
#
proc get_lease { cfgtype restoredir } {
	#
	# The number of sites must be the same for all.  Read the
	# first site's saved DB_CONFIG file if we're restoring since
	# we only know we have at least 1 site.
	#
	if { $cfgtype == "restore" } {
		set use_lease 0
		set cid [open $restoredir/DB_CONFIG.1 r]
		while { [gets $cid cfglist] } {
#			puts "Read in: $cfglist"
			if { [llength $cfglist] == 0 } {
				break;
			}
			set cfg [lindex $cfglist 0]
			if { $cfg == "rep_set_config" } {
				set lease [lindex $cfglist 1]
				if { $lease == "db_rep_conf_lease" } {
					set use_lease 1
					break;
				}
			}
		}
		close $cid
		return $use_lease
	}
	if { $cfgtype == "random" } {
		set leases { 1 0 0 0 }
		set len [expr [llength $leases] - 1]
		set i [berkdb random_int 0 $len]
		return [lindex $leases $i]
	}
	if { $cfgtype == "basic0" } {
		return 0
	}
	if { $cfgtype == "basic1" } {
		return 0
	}
}

#
# Do a kill test about half the time.  We randomly choose a
# site number to kill, it could be a master or a client.
# Return 0 if we don't kill any site.
#
proc get_kill { cfgtype restoredir num_sites } {
	if { $cfgtype == "restore" } {
		set ksite 0
		for { set i 1 } { $i <= $num_sites } { incr i } {
			set cid [open $restoredir/DB_REPTEST_ARGS.$i r]
			#
			# !!!
			# We currently assume the args file is 1 line.
			# We assume only 1 site can get killed.  So, if we
			# find one, we break the loop and don't look further.
			#
			gets $cid arglist
			close $cid
#			puts "Read in: $arglist"
			set dokill [lsearch $arglist "-k"]
			if { $dokill != -1 } {
				set ksite $i
				break
			}
		}
		return $ksite
	}
	if { $cfgtype == "random" } {
		set k { 0 0 0 1 1 1 0 1 1 0 }
		set len [expr [llength $k] - 1]
		set i [berkdb random_int 0 $len]
		if { [lindex $k $i] == 1 } {
			set ksite [berkdb random_int 1 $num_sites]
		} else {
			set ksite 0
		}
		return $ksite
	}
	if { $cfgtype == "basic0" || $cfgtype == "basic1" } {
		return 0
	} else {
		error "Get_kill: Invalid config type $cfgtype"
	}
}

#
# Use peers or only the master for requests? 25%/75% (use a peer 25%
# of the time and master 75%)
#
proc get_peers { cfgtype } {
	if { $cfgtype == "random" } {
		set peer { 0 0 0 1 }
		set len [expr [llength $peer] - 1]
		set i [berkdb random_int 0 $len]
		return [lindex $peer $i]
	}
	if { $cfgtype == "basic0" || $cfgtype == "basic1" } {
		return 0
	}
}

#
# Start with a master or all clients?  25%/75% (use a master 25%
# of the time and have all clients 75%)
#
proc get_usemaster { cfgtype } {
	if { $cfgtype == "random" } {
		set mst { 1 0 0 0 }
		set len [expr [llength $mst] - 1]
		set i [berkdb random_int 0 $len]
		return [lindex $mst $i]
	}
	if { $cfgtype == "basic0" } {
		return 1
	}
	if { $cfgtype == "basic1" } {
		return 0
	}
}

#
# If we use a master, which site?  This proc will return
# the site number of the mastersite, or it will return
# 0 if no site should start as master.  Sites are numbered
# starting at 1.
#
proc get_mastersite { cfgtype usemaster nsites } {
	if { $usemaster == 0 } {
		return 0
	}
	if { $cfgtype == "random" } {
		return [berkdb random_int 1 $nsites]
	}
	if { $cfgtype == "basic0" } {
		return 1
	}
	if { $cfgtype == "basic1" } {
		return 0
	}
}

#
# This is the number of worker threads performing the workload.
# This is not the number of message processing threads.
#
# Scale back the number of worker threads if leases are in use.
# The timing with leases can be fairly sensitive and since all sites
# run on the local machine, too many workers on every site can
# overwhelm the system, causing lost messages and delays that make
# the tests fail.  Rather than try to tweak timeouts, just reduce
# the workloads a bit.
#
proc get_workers { cfgtype lease } {
	if { $cfgtype == "random" } {
		if { $lease } {
			return [berkdb random_int 2 4]
		} else {
			return [berkdb random_int 2 8]
		}
	}
	if { $cfgtype == "basic0" || $cfgtype == "basic1" } {
		return 5
	}
}

proc get_dbtype { cfgtype } {
	if { $cfgtype == "random" } {
		#
		# 50% btree, 25% queue 12.5% hash 12.5% recno
		# We favor queue only because there is special handling
		# for queue in internal init.
		#
#		set methods {btree btree btree btree queue queue hash recno}
		set methods {btree btree btree btree hash recno}
		set len [expr [llength $methods] - 1]
		set i [berkdb random_int 0 $len]
		return [lindex $methods $i]
	}
	if { $cfgtype == "basic0" || $cfgtype == "basic1" } {
		return btree
	}
}

proc get_runtime { cfgtype } {
	if { $cfgtype == "random" } {
		return [berkdb random_int 100 500]
	}
	if { $cfgtype == "basic0" } {
		return 100
	}
	if { $cfgtype == "basic1" } {
		return 150
	}
}

proc get_rport { baseport i num_sites cfgtype} {
	global rporttype

	if { $cfgtype == "random" && $rporttype == "NULL" } {
		#
		# The circular comm choices seem problematic.
		# Remove them for now.
		#
#		set types {backcirc forwcirc full onesite}
		set types {full onesite}
		set len [expr [llength $types] - 1]
		set rindex [berkdb random_int 0 $len]
		set rporttype [lindex $types $rindex]
	}
	if { $cfgtype == "basic0" } {
		set rporttype onesite
	}
	if { $cfgtype == "basic1" } {
		set rporttype full
	}
	#
	# This produces a circular knowledge ring.  Either forward
	# or backward.  In the forwcirc, ENV1 knows (via -r) about
	# ENV2, ENV2 knows about ENV3, ..., ENVX knows about ENV1.
	#
	if { $rporttype == "forwcirc" } {
		if { $i != $num_sites } {
			return [list [expr $baseport + $i + 1]]
		} else {
			return [list [expr $baseport + 1]]
		}
	}
	if { $rporttype == "backcirc" } {
		if { $i != 1 } {
			return [list [expr $baseport + $i - 1]]
		} else {
			return [list [expr $baseport + $num_sites]]
		}
	}
	#
	# This produces a configuration where site 1 does not know
	# about any other site and every other site knows about site 1.
	#
	if { $rporttype == "onesite" } {
		if { $i == 1 } {
			return {}
		} else {
			return [list [expr $baseport + 1]]
		}
	}
	#
	# This produces a fully connected configuration
	#
	if { $rporttype == "full" } {
		set rlist {}
		for { set site 1 } { $site <= $num_sites } { incr site } {
			if { $site != $i } {
				lappend rlist [expr $baseport + $site]
			}
		}
		return $rlist
	}
}

proc parse_runtime { progargs } {
	set i [lsearch $progargs "-T"]
	set val [lindex $progargs [expr $i + 1]]
	return $val
}
