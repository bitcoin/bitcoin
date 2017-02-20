# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	dead002
# TEST	Same test as dead001, but use "detect on every collision" instead
# TEST	of separate deadlock detector.
proc dead002 { { procs "2 4 10" } {tests "ring clump" } \
    {timeout 0} {tnum 002} } {
	source ./include.tcl

	puts "Dead$tnum: Deadlock detector tests (detect on every collision)"

	env_cleanup $testdir

	# Create the environment.
	puts "\tDead$tnum.a: creating environment"
	set lmode "default"
	if { $timeout != 0 } {
		set lmode "expire"
	}
	set env [berkdb_env \
	    -create -mode 0644 -home $testdir \
	    -lock -lock_timeout $timeout -lock_detect $lmode]
	error_check_good lock_env:open [is_valid_env $env] TRUE

	foreach t $tests {
		foreach n $procs {
			set pidlist ""
			sentinel_init

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
				# If we're running with timeouts, pause so that
				# locks will have a chance to time out.
				if { $timeout != 0 } {
					tclsleep 2
				}
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

			puts "\tDead$tnum: dead check ..."
			dead_check $t $n $timeout $dead $clean $other
		}
	}

	fileremove -f $testdir/dd.out
	# Remove log files
	for { set i 0 } { $i < $n } { incr i } {
		fileremove -f $testdir/dead$tnum.log.$i
	}
	error_check_good lock_env:close [$env close] 0
}
