# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# Random lock tester.
# Usage: lockscript dir numiters numobjs sleepint degree readratio
# dir: lock directory.
# numiters: Total number of iterations.
# numobjs: Number of objects on which to lock.
# sleepint: Maximum sleep interval.
# degree: Maximum number of locks to acquire at once
# readratio: Percent of locks that should be reads.

source ./include.tcl
source $test_path/test.tcl

set usage "lockscript dir numiters numobjs sleepint degree readratio"

# Verify usage
if { $argc != 6 } {
	puts stderr "FAIL:[timestamp] Usage: $usage"
	exit
}

# Initialize arguments
set dir [lindex $argv 0]
set numiters [ lindex $argv 1 ]
set numobjs [ lindex $argv 2 ]
set sleepint [ lindex $argv 3 ]
set degree [ lindex $argv 4 ]
set readratio [ lindex $argv 5 ]

# Initialize random number generator
global rand_init
berkdb srand $rand_init


catch { berkdb_env -create -lock -home $dir } e
error_check_good env_open [is_substr $e env] 1
catch { $e lock_id } locker
error_check_good locker [is_valid_locker $locker] TRUE

puts -nonewline "Beginning execution for $locker: $numiters $numobjs "
puts "$sleepint $degree $readratio"
flush stdout

for { set iter 0 } { $iter < $numiters } { incr iter } {
	set nlocks [berkdb random_int 1 $degree]
	# We will always lock objects in ascending order to avoid
	# deadlocks.
	set lastobj 1
	set locklist {}
	set objlist {}
	for { set lnum 0 } { $lnum < $nlocks } { incr lnum } {
		# Pick lock parameters
		set obj [berkdb random_int $lastobj $numobjs]
		set lastobj [expr $obj + 1]
		set x [berkdb random_int 1 100 ]
		if { $x <= $readratio } {
			set rw read
		} else {
			set rw write
		}
		puts "[timestamp -c] $locker $lnum: $rw $obj"

		# Do get; add to list
		catch {$e lock_get $rw $locker $obj} lockp
		error_check_good lock_get [is_valid_lock $lockp $e] TRUE

		# Create a file to flag that we've a lock of the given
		# type, after making sure only other read locks exist
		# (if we're read locking) or no other locks exist (if
		# we're writing).
		lock003_vrfy $rw $obj
		lock003_create $rw $obj
		lappend objlist [list $obj $rw]

		lappend locklist $lockp
		if {$lastobj > $numobjs} {
			break
		}
	}
	# Pick sleep interval
	puts "[timestamp -c] $locker sleeping"
	# We used to sleep 1 to $sleepint seconds.  This makes the test
	# run for hours.  Instead, make it sleep for 10 to $sleepint * 100
	# milliseconds, for a maximum sleep time of 0.5 s.
	after [berkdb random_int 10 [expr $sleepint * 100]]
	puts "[timestamp -c] $locker awake"

	# Now release locks
	puts "[timestamp -c] $locker released locks"

	# Delete our locking flag files, then reverify.  (Note that the
	# locking flag verification function assumes that our own lock
	# is not currently flagged.)
	foreach pair $objlist {
		set obj [lindex $pair 0]
		set rw [lindex $pair 1]
		lock003_destroy $obj
		lock003_vrfy $rw $obj
	}

	release_list $locklist
	flush stdout
}

set ret [$e close]
error_check_good env_close $ret 0

puts "[timestamp -c] $locker Complete"
flush stdout

exit
