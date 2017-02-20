# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	dead001
# TEST	Use two different configurations to test deadlock detection among a
# TEST	variable number of processes.  One configuration has the processes
# TEST	deadlocked in a ring.  The other has the processes all deadlocked on
# TEST	a single resource.
proc dead001 { { procs "2 4 10" } {tests "ring clump" } \
    {timeout 0} {tnum "001"} } {
	source ./include.tcl
	global lock_curid
	global lock_maxid

	puts "Dead$tnum: Deadlock detector tests"

	env_cleanup $testdir

	# Create the environment.
	puts "\tDead$tnum.a: creating environment"
	set env [berkdb_env -create \
	     -mode 0644 -lock -lock_timeout $timeout -home $testdir]
	error_check_good lock_env:open [is_valid_env $env] TRUE

	foreach t $tests {
		foreach n $procs {
			if {$timeout == 0 } {
				set dpid [exec $util_path/db_deadlock -v -t 0.100000 \
				    -h $testdir >& $testdir/dd.out &]
			} else {
				set dpid [exec $util_path/db_deadlock -v -t 0.100000 \
				    -ae -h $testdir >& $testdir/dd.out &]
			}

			sentinel_init
			set pidlist ""
			set ret [$env lock_id_set $lock_curid $lock_maxid]
			error_check_good lock_id_set $ret 0

			# Fire off the tests
			puts "\tDead$tnum: $n procs of test $t"
			for { set i 0 } { $i < $n } { incr i } {
				set locker [$env lock_id]
				puts "$tclsh_path $test_path/wrap.tcl \
				    ddscript.tcl $testdir/dead$tnum.log.$i \
				    $testdir $t $locker $i $n"
				set p [exec $tclsh_path $test_path/wrap.tcl \
					ddscript.tcl $testdir/dead$tnum.log.$i \
					$testdir $t $locker $i $n &]
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
			tclkill $dpid
			puts "\tDead$tnum: dead check..."
			dead_check $t $n $timeout $dead $clean $other
		}
	}

	# Windows needs files closed before deleting files, so pause a little
	tclsleep 3
	fileremove -f $testdir/dd.out
	# Remove log files
	for { set i 0 } { $i < $n } { incr i } {
		fileremove -f $testdir/dead$tnum.log.$i
	}
	error_check_good lock_env:close [$env close] 0
}
