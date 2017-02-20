# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	dead003
# TEST
# TEST	Same test as dead002, but explicitly specify DB_LOCK_OLDEST and
# TEST	DB_LOCK_YOUNGEST.  Verify the correct lock was aborted/granted.
proc dead003 { {procs "2 4 10"} {tests "ring clump"} {tnum "003"} } {
	source ./include.tcl
	global lock_curid
	global lock_maxid

	set detects { oldest youngest }
	puts "Dead$tnum: Deadlock detector tests: $detects"

	# Create the environment.
	foreach d $detects {
		env_cleanup $testdir
		puts "\tDead$tnum.a: creating environment for $d"
		set env [berkdb_env \
		    -create -mode 0644 -home $testdir -lock -lock_detect $d]
		error_check_good lock_env:open [is_valid_env $env] TRUE

		foreach t $tests {
			foreach n $procs {
				set pidlist ""
				sentinel_init
				set ret [$env lock_id_set \
				     $lock_curid $lock_maxid]
				error_check_good lock_id_set $ret 0

				# Fire off the tests
				puts "\tDead$tnum: $n procs of test $t"
				for { set i 0 } { $i < $n } { incr i } {
					set locker [$env lock_id]
					puts "$tclsh_path\
					    test_path/ddscript.tcl $testdir \
					    $t $locker $i $n >& \
					    $testdir/dead$tnum.log.$i"
					set p [exec $tclsh_path \
					    $test_path/wrap.tcl \
					    ddscript.tcl \
					    $testdir/dead$tnum.log.$i $testdir \
					    $t $locker $i $n &]
					lappend pidlist $p
				}
				watch_procs $pidlist 5

				# Now check output
				set dead 0
				set clean 0
				set other 0
				for { set i 0 } { $i < $n } { incr i } {
					set did [open $testdir/dead$tnum.log.$i]
					while { [gets $did val] != -1 } {
						switch $val {
							DEADLOCK { incr dead }
							1 { incr clean }
							default { incr other }
						}
					}
					close $did
				}
				puts "\tDead$tnum: dead check..."
				dead_check $t $n 0 $dead $clean $other
				#
				# If we get here we know we have the
				# correct number of dead/clean procs, as
				# checked by dead_check above.  Now verify
				# that the right process was the one.
				puts "\tDead$tnum: Verify $d locks were aborted"
				set l ""
				if { $d == "oldest" } {
					set l [expr $n - 1]
				}
				if { $d == "youngest" } {
					set l 0
				}
				set did [open $testdir/dead$tnum.log.$l]
				while { [gets $did val] != -1 } {
					error_check_good check_abort \
					    $val 1
				}
				close $did
			}
		}

		fileremove -f $testdir/dd.out
		# Remove log files
		for { set i 0 } { $i < $n } { incr i } {
			fileremove -f $testdir/dead$tnum.log.$i
		}
		error_check_good lock_env:close [$env close] 0
	}
}
