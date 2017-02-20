# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test044
# TEST	Small system integration tests
# TEST		Test proper functioning of the checkpoint daemon,
# TEST		recovery, transactions, etc.
# TEST
# TEST	System integration DB test: verify that locking, recovery, checkpoint,
# TEST	and all the other utilities basically work.
# TEST
# TEST	The test consists of $nprocs processes operating on $nfiles files.  A
# TEST	transaction consists of adding the same key/data pair to some random
# TEST	number of these files.  We generate a bimodal distribution in key size
# TEST	with 70% of the keys being small (1-10 characters) and the remaining
# TEST	30% of the keys being large (uniform distribution about mean $key_avg).
# TEST	If we generate a key, we first check to make sure that the key is not
# TEST	already in the dataset.  If it is, we do a lookup.
#
# XXX
# This test uses grow-only files currently!
proc test044 { method {nprocs 5} {nfiles 10} {cont 0} args } {
	source ./include.tcl
	global encrypt
	global rand_init

	set args [convert_args $method $args]
	set omethod [convert_method $method]
	set pageargs ""
	split_pageargs $args pageargs


	berkdb srand $rand_init

	# If we are using an env, then skip this test.  It needs its own.
	set eindex [lsearch -exact $args "-env"]
	if { $eindex != -1 } {
		incr eindex
		set env [lindex $args $eindex]
		puts "Test044 skipping for env $env"
		return
	}
	if { $encrypt != 0 } {
		puts "Test044 skipping for security"
		return
	}

	puts "Test044: system integration test db $method $nprocs processes \
	    on $nfiles files"

	# Parse options
	set otherargs ""
	set key_avg 10
	set data_avg 20
	set do_exit 0
	for { set i 0 } { $i < [llength $args] } {incr i} {
		switch -regexp -- [lindex $args $i] {
			-key_avg { incr i; set key_avg [lindex $args $i] }
			-data_avg { incr i; set data_avg [lindex $args $i] }
			-testdir { incr i; set testdir [lindex $args $i] }
			-x.* { set do_exit 1 }
			default {
				lappend otherargs [lindex $args $i]
			}
		}
	}

	if { $cont == 0 } {
		# Create the database and open the dictionary
		env_cleanup $testdir

		# Create an environment.  Bump up the log region because
		# we will create lots of files.  This is especially
		# needed when we test partitioned databases.
		set cid [open $testdir/DB_CONFIG w]
		puts $cid "set_lg_regionmax 200000"
		close $cid

		puts "\tTest044.a: creating environment and $nfiles files"
		set dbenv \
		     [eval {berkdb_env -create -txn} $pageargs -home $testdir]
		error_check_good env_open [is_valid_env $dbenv] TRUE

		# Create a bunch of files
		set m $method

		for { set i 0 } { $i < $nfiles } { incr i } {
			if { $method == "all" } {
				switch [berkdb random_int 1 2] {
				1 { set m -btree }
				2 { set m -hash }
				}
			} else {
				set m $omethod
			}

			set db [eval {berkdb_open -env $dbenv -create \
			    -mode 0644 $m} $otherargs {test044.$i.db}]
			error_check_good dbopen [is_valid_db $db] TRUE
			error_check_good db_close [$db close] 0
		}
	}

	# Close the environment
	$dbenv close

	if { $do_exit == 1 } {
		return
	}

	# Database is created, now fork off the kids.
	puts "\tTest044.b: forking off $nprocs processes and utilities"
	set cycle 1
	set ncycles 3
	while { $cycle <= $ncycles } {
		set dbenv \
		     [eval {berkdb_env -create -txn} $pageargs -home $testdir]
		error_check_good env_open [is_valid_env $dbenv] TRUE

		# Fire off deadlock detector and checkpointer
		puts "Beginning cycle $cycle"
		set ddpid [exec $util_path/db_deadlock -h $testdir -t 5 &]
		set cppid [exec $util_path/db_checkpoint -h $testdir -p 2 &]
		puts "Deadlock detector: $ddpid Checkpoint daemon $cppid"

		set pidlist {}
		for { set i 0 } {$i < $nprocs} {incr i} {
			set p [exec $tclsh_path \
			    $test_path/sysscript.tcl $testdir \
			    $nfiles $key_avg $data_avg $omethod $args\
			    >& $testdir/test044.$i.log &]
			lappend pidlist $p
		}
		set sleep [berkdb random_int 300 600]
		puts \
"[timestamp] $nprocs processes running $pidlist for $sleep seconds"
		tclsleep $sleep

		# Now simulate a crash
		puts "[timestamp] Crashing"

		#
		# The environment must remain open until this point to get
		# proper sharing (using the paging file) on Win/9X.  [#2342]
		#
		error_check_good env_close [$dbenv close] 0

		tclkill $ddpid
		tclkill $cppid

		foreach p $pidlist {
			tclkill $p
		}

		# Check for test failure
		set errstrings [eval findfail [glob $testdir/test044.*.log]]
		foreach str $errstrings {
			puts "FAIL: error message in log file: $str"
		}

		# Now run recovery
		eval test044_verify $testdir $nfiles $otherargs
		incr cycle
	}
}

proc test044_usage { } {
	puts -nonewline "test044 method nentries [-d directory] [-i iterations]"
	puts " [-p procs] -x"
}

proc test044_verify { dir nfiles args} {
	source ./include.tcl

	# Save everything away in case something breaks
#	for { set f 0 } { $f < $nfiles } {incr f} {
#		file copy -force $dir/test044.$f.db $dir/test044.$f.save1
#	}
#	foreach f [glob $dir/log.*] {
#		if { [is_substr $f save] == 0 } {
#			file copy -force $f $f.save1
#		}
#	}

	# Run recovery and then read through all the database files to make
	# sure that they all look good.

	puts "\tTest044.verify: Running recovery and verifying file contents"
	set stat [catch {exec $util_path/db_recover -h $dir} result]
	if { $stat == 1 } {
		error "FAIL: Recovery error: $result."
	}

	# Save everything away in case something breaks
#	for { set f 0 } { $f < $nfiles } {incr f} {
#		file copy -force $dir/test044.$f.db $dir/test044.$f.save2
#	}
#	foreach f [glob $dir/log.*] {
#		if { [is_substr $f save] == 0 } {
#			file copy -force $f $f.save2
#		}
#	}

	for { set f 0 } { $f < $nfiles } { incr f } {
		set db($f) [eval {berkdb_open} $args {$dir/test044.$f.db}]
		error_check_good $f:dbopen [is_valid_db $db($f)] TRUE

		set cursors($f) [$db($f) cursor]
		error_check_bad $f:cursor_open $cursors($f) NULL
		error_check_good \
		    $f:cursor_open [is_substr $cursors($f) $db($f)] 1
	}

	for { set f 0 } { $f < $nfiles } { incr f } {
		for {set d [$cursors($f) get -first] } \
		    { [string length $d] != 0 } \
		    { set d [$cursors($f) get -next] } {

			set k [lindex [lindex $d 0] 0]
			set d [lindex [lindex $d 0] 1]

			set flist [zero_list $nfiles]
			set r $d
			while { [set ndx [string first : $r]] != -1 } {
				set fnum [string range $r 0 [expr $ndx - 1]]
				if { [lindex $flist $fnum] == 0 } {
					set fl "-set"
				} else {
					set fl "-next"
				}

				if { $fl != "-set" || $fnum != $f } {
					if { [string compare $fl "-set"] == 0} {
						set full [$cursors($fnum) \
						    get -set $k]
					} else  {
						set full [$cursors($fnum) \
						    get -next]
					}
					set key [lindex [lindex $full 0] 0]
					set rec [lindex [lindex $full 0] 1]
					error_check_good \
					    $f:dbget_$fnum:key $key $k
					error_check_good \
					    $f:dbget_$fnum:data $rec $d
				}

				set flist [lreplace $flist $fnum $fnum 1]
				incr ndx
				set r [string range $r $ndx end]
			}
		}
	}

	for { set f 0 } { $f < $nfiles } { incr f } {
		error_check_good $cursors($f) [$cursors($f) close] 0
		error_check_good db_close:$f [$db($f) close] 0
	}
}
