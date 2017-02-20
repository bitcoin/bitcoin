# See the file LICENSE for redistribution information.
#
# Copyright (c) 2006-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep065
# TEST	Tests replication running with different versions.
# TEST	This capability is introduced with 4.5.
# TEST
# TEST	Start a replication group of 1 master and N sites, all
# TEST	running some historical version greater than or equal to 4.4.
# TEST	Take down a client and bring it up again running current.
# TEST	Run some upgrades, make sure everything works.
# TEST
# TEST	Each site runs the tcllib of its own version, but uses
# TEST	the current tcl code (e.g. test.tcl).
proc rep065 { method { nsites 3 } args } {
	source ./include.tcl
	global repfiles_in_memory
	global noenv_messaging
	set noenv_messaging 1

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}
	#
	# Skip all methods but btree - we don't use the method, as we
	# run over all of them with varying versions.
	#
	if { $checking_valid_methods } {
		set test_methods { btree }
		return $test_methods
	}

	if { [is_btree $method] == 0 } {
		puts "Rep065: Skipping for method $method."
		return
	}

	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	# Make the list of {method version} pairs to test.
	#
	set mvlist [method_version]
	set mvlen [llength $mvlist]
	puts "Rep065: Testing the following $mvlen method/version pairs:"
	puts "Rep065: $mvlist"
	puts "Rep065: $msg2"
	set count 1
	set total [llength $mvlist]
	set slist [setup_sites $nsites]
	foreach i $mvlist {
		puts "Rep065: Test iteration $count of $total: $i"
		rep065_sub $count $i $nsites $slist
		incr count
	}
	set noenv_messaging 0
}

proc rep065_sub { iter mv nsites slist } {
	source ./include.tcl
	global machids
	global util_path
	set machids {}
	set method [lindex $mv 0]
	set vers [lindex $mv 1]

	puts "\tRep065.$iter.a: Set up."
	# Whatever directory we started this process from is referred
	# to as the controlling directory.  It will contain the message
	# queue and start all the child processes.
	set controldir [pwd]
	env_cleanup $controldir/$testdir
	replsetup_noenv $controldir/$testdir/MSGQUEUEDIR

	# Set up the historical build directory.  The master will start
	# running with historical code.
	#
	# This test presumes we are running in the current build
	# directory and that the expected historical builds are
	# set up in a similar fashion.  If they are not, quit gracefully.

	set pwd [pwd]
	set homedir [file dirname [file dirname $pwd]]
	set reputils_path $pwd/../test
	set histdir $homedir/$vers/build_unix
	if { [file exists $histdir] == 0 } {
		puts -nonewline "Skipping iteration $iter: cannot find"
		puts " historical version $vers."
		return
	}
	if { [file exists $histdir/db_verify] == 0 } {
		puts -nonewline "Skipping iteration $iter: historical version"
		puts " $vers is missing some executables.  Is it built?"
		return
	}

	set histtestdir $histdir/TESTDIR

	env_cleanup $histtestdir
	set markerdir $controldir/$testdir/MARKER
	file delete -force $markerdir

	# Create site directories.  They start running in the historical
	# directory, too.  They will be upgraded to the current version
	# first.
	set allids { }
	for { set i 0 } { $i < $nsites } { incr i } {
		set siteid($i) [expr $i + 1]
		set sid $siteid($i)
		lappend allids $sid
		set histdirs($sid) $histtestdir/SITE.$i
		set upgdir($sid) $controldir/$testdir/SITE.$i
		file mkdir $histdirs($sid)
		file mkdir $upgdir($sid)
	}

	# Open master env running 4.4.
	#
	# We know that slist has all sites starting in the histdir.
	# So if we encounter an upgrade value, we upgrade that client
	# from the hist dir.
	#
	set count 1
	foreach sitevers $slist {
		puts "\tRep065.b.$iter.$count: Run with sitelist $sitevers."
		#
		# Delete the marker directory each iteration so that
		# we don't find old data in there.
		#
		file delete -force $markerdir
		file mkdir $markerdir
		#
		# Get the chosen master index from the list of sites.
		#
		set mindex [get_master $nsites $sitevers]
		set meid [expr $mindex + 1]

		#
		# Kick off the test processes.  We need 1 test process
		# per site and 1 message process per site.
		#
		set pids {}
		for { set i 0 } { $i < $nsites } { incr i } {
			set upg [lindex $sitevers $i]
			set sid $siteid($i)
			#
			# If we are running "old" set up an array
			# saying if this site has run old/new yet.
			# The reason is that we want to "upgrade"
			# only the first time we go from old to new,
			# not every iteration through this loop.
			#
			if { $upg == 0 } {
				puts -nonewline "\t\tRep065.b: Test: Old site $i"
				set sitedir($i) $histdirs($sid)
				set already_upgraded($i) 0
			} else {
				puts -nonewline "\t\tRep065.b: Test: Upgraded site $i"
				set sitedir($i) $upgdir($sid)
				if { $already_upgraded($i) == 0 } {
					upg_repdir $histdirs($sid) $sitedir($i)
				}
				set already_upgraded($i) 1
			}
			if { $sid == $meid } {
				set state MASTER
				set runtest [list REPTEST $method 15 10]
				puts " (MASTER)"
			} else {
				set state CLIENT
				set runtest {REPTEST_GET}
				puts " (CLIENT)"
			}
			lappend pids [exec $tclsh_path $test_path/wrap.tcl \
			    rep065script.tcl \
			    $controldir/$testdir/$count.S$i.log \
		      	    SKIP \
			    START $state \
			    $runtest \
			    $sid $allids $controldir \
			    $sitedir($i) $reputils_path &]
			lappend pids [exec $tclsh_path $test_path/wrap.tcl \
			    rep065script.tcl \
			    $controldir/$testdir/$count.S$i.msg \
		    	    SKIP \
			    PROCMSGS $state \
		    	    NULL \
			    $sid $allids $controldir \
			    $sitedir($i) $reputils_path &]
		}

		watch_procs $pids 20
		#
		# At this point, clean up any message files.  The message
		# system leads to a significant number of duplicate
		# requests.  If the master site handled them after the
		# client message processes exited, then there can be
		# a large number of "dead" message files waiting for
		# non-existent clients.  Just clean up everyone.
		#
		for { set i 0 } { $i < $nsites } { incr i } {
			replclear_noenv $siteid($i)
		}

		#
		# Kick off the verification processes.  These just walk
		# their own logs and databases, so we don't need to have
		# a message process.  We need separate processes because
		# old sites need to use old utilities.
		#
		set pids {}
		puts "\tRep065.c.$iter.$count: Verify all sites."
		for { set i 0 } { $i < $nsites } { incr i } {
			if { $siteid($i) == $meid } {
				set state MASTER
			} else {
				set state CLIENT
			}
			lappend pids [exec $tclsh_path $test_path/wrap.tcl \
			    rep065script.tcl \
			    $controldir/$testdir/$count.S$i.ver \
		      	    SKIP \
			    VERIFY $state \
		    	    {LOG DB} \
			    $siteid($i) $allids $controldir \
			    $sitedir($i) $reputils_path &]
		}

		watch_procs $pids 10
		#
		# Now that each site created its verification files,
		# we can now verify everyone.
		#
		for { set i 0 } { $i < $nsites } { incr i } {
			if { $i == $mindex } {
				continue
			}
			puts \
	"\t\tRep065.c: Verify: Compare databases master and client $i"
			error_check_good db_cmp \
			    [filecmp $sitedir($mindex)/VERIFY/dbdump \
			    $sitedir($i)/VERIFY/dbdump] 0
			set upg [lindex $sitevers $i]
			# !!!
			# Although db_printlog works and can read old logs,
			# there have been some changes to the output text that
			# makes comparing difficult.  One possible solution
			# is to run db_printlog here, from the current directory
			# instead of from the historical directory.
			#
			if { $upg == 0 } {
				puts \
	"\t\tRep065.c: Verify: Compare logs master and client $i"
				error_check_good log_cmp \
				    [filecmp $sitedir($mindex)/VERIFY/prlog \
				    $sitedir($i)/VERIFY/prlog] 0
			} else {
				puts \
	"\t\tRep065.c: Verify: Compare LSNs master and client $i"
				error_check_good log_cmp \
				    [filecmp $sitedir($mindex)/VERIFY/loglsn \
				    $sitedir($i)/VERIFY/loglsn] 0
			}
		}

		#
		# At this point we have a master and sites all up to date
		# with each other.  Now, one at a time, upgrade the sites
		# to the current version and start everyone up again.
		incr count
	}
}

proc setup_sites { nsites } {
	#
	# Set up a list that goes from 0 to $nsites running
	# upgraded.  A 0 represents running old version and 1
	# represents running upgraded.  So, for 3 sites it will look like:
	# { 0 0 0 } { 1 0 0 } { 1 1 0 } { 1 1 1 }
	#
	set sitelist {}
	for { set i 0 } { $i <= $nsites } { incr i } {
		set l ""
		for { set j 1 } { $j <= $nsites } { incr j } {
			if { $i < $j } {
				lappend l 0
			} else {
				lappend l 1
			}
		}
		lappend sitelist $l
	}
	return $sitelist
}

proc upg_repdir { histdir upgdir } {
	global util_path

	#
	# Upgrade a site to the current version.  This entails:
	# 1.  Removing any old files from the upgrade directory.
	# 2.  Copy all old version files to upgrade directory.
	# 3.  Remove any __db files from upgrade directory except __db.rep*gen.
	# 4.  Force checkpoint in new version.
	file delete -force $upgdir

	# Recovery was run before as part of upgradescript.
	# Archive dir by copying it to upgrade dir.
	file copy -force $histdir $upgdir
	set dbfiles [glob -nocomplain $upgdir/__db*]
	foreach d $dbfiles {
		if { $d == "$upgdir/__db.rep.gen" ||
		    $d == "$upgdir/__db.rep.egen" } {
			continue
		}
		file delete -force $d
	}
	# Force current version checkpoint
	set stat [catch {eval exec $util_path/db_checkpoint -1 -h $upgdir} r]
	if { $stat != 0 } {
		puts "CHECKPOINT: $upgdir: $r"
	}
	error_check_good stat_ckp $stat 0
}

proc get_master { nsites verslist } {
	error_check_good vlist_chk [llength $verslist] $nsites
	#
	# When we can, simply run an election to get a new master.
	# We then verify we got an old client.
	#
	# For now, randomly pick among the old sites, or if no old
	# sites just randomly pick anyone.
	#
	set old_count 0
	# Pick 1 out of N old sites or 1 out of nsites if all upgraded.
	foreach i $verslist {
		if { $i == 0 } {
			incr old_count
		}
	}
	if { $old_count == 0 } {
		set old_count $nsites
	}
	set master [berkdb random_int 0 [expr $old_count - 1]]
	#
	# Since the Nth old site may not be at the Nth place in the
	# list unless we used the entire list, we need to loop to find
	# the right index to return.
	if { $old_count == $nsites } {
		return $master
	}
	set ocount 0
	set index 0
	foreach i $verslist {
		if { $i == 1 } {
			incr index
			continue
		}
		if { $ocount == $master } {
			return $index
		}
		incr ocount
		incr index
	}
	#
	# If we get here there is a problem in the code.
	#
	error "FAIL: get_master problem"
}

proc method_version { } {
	global valid_methods

	set meth $valid_methods
	set startmv { {btree db-4.4.20} {hash db-4.5.20} }

	# Remove btree and hash from the method list, we're manually
	# assigning those versions due to log/recovery record changes
	# at that version.
	set midx [lsearch -exact $meth hash]
	set meth [lreplace $meth $midx $midx]
	set midx [lsearch -exact $meth btree]
	set meth [lreplace $meth $midx $midx]

	set vers {db-4.4.20 db-4.5.20 db-4.6.21 db-4.7.25}
	set dbvlen [llength $vers]
	#
	# NOTE: The values in "vers_list" are indices into $vers above.
	# Since we're explicitly testing 4.4.20 and 4.5.20 above,
	# weight later versions more.
	# When you add a new version to $vers, you must
	# add some new items to $vers_list to choose that index.
	# Also need to add an entry for 'vtest' below.
	#
	set vers_list { 0 0 1 1 2 2 2 3 3 3 }
	set vers_len [expr [llength $vers_list] - 1]

	# Walk through the list of remaining methods and randomly
	# assign a version to each one.
	while { 1 } {
		set mv $startmv
		# We want to make sure we test each version.
		# 4.4.20
		set vtest(0) 1
		# 4.5.20
		set vtest(1) 1
		# 4.6.21
		set vtest(2) 0
		# 4.7.25
		set vtest(3) 0
		foreach m $meth {
			# Index into distribution list.
			set vidx [berkdb random_int 0 $vers_len]
			# Index into version list.
			set vindex [lindex $vers_list $vidx]
			set vtest($vindex) 1
			set v [lindex $vers $vindex]
			lappend mv [list $m $v]
		}
		#
		# Assume success.  If we find any $vtest entry of 0,
		# then we fail and try again.
		#
		set all_vers 1
		for { set i 0 } { $i < $dbvlen } { incr i } {
			if { $vtest($i) == 0 } {
				set all_vers 0
			}
		}
		if { $all_vers == 1 } {
			break
		}
#		puts "Did not get all versions with $mv."
	}

	return $mv
}
