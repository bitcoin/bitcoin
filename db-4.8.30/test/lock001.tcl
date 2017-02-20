# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#

# TEST	lock001
# TEST	Make sure that the basic lock tests work.  Do some simple gets
# TEST	and puts for a single locker.
proc lock001 { {iterations 1000} } {
	source ./include.tcl
	global lock_curid
	global lock_maxid

	set save_curid $lock_curid
	set save_maxid $lock_maxid

	# Set defaults
	# Adjusted to make exact match of isqrt
	#set conflicts { 3 0 0 0 0 0 1 0 1 1}
	#set conflicts { 3 0 0 0 0 1 0 1 1}

	set conflicts { 0 0 0 0 0 1 0 1 1}
	set nmodes [isqrt [llength $conflicts]]

	# Cleanup
	env_cleanup $testdir

	# Open the region we'll use for testing.
	set eflags "-create -lock -home $testdir -mode 0644 \
	    -lock_conflict {$nmodes {$conflicts}}"
	set env [eval {berkdb_env} $eflags]
	error_check_good env [is_valid_env $env] TRUE
	error_check_good lock_id_set \
	     [$env lock_id_set $lock_curid $lock_maxid] 0

	puts "Lock001: test basic lock operations"
	set locker [$env lock_id]
	# Get and release each type of lock
	puts "\tLock001.a: get and release each type of lock"
	foreach m {ng write read} {
		set obj obj$m
		set lockp [$env lock_get $m $locker $obj]
		error_check_good lock_get:a [is_blocked $lockp] 0
		error_check_good lock_get:a [is_substr $lockp $env] 1
		set ret [ $lockp put ]
		error_check_good lock_put $ret 0
	}

	# Get a bunch of locks for the same locker; these should work
	set obj OBJECT
	puts "\tLock001.b: Get a bunch of locks for the same locker"
	foreach m {ng write read} {
		set lockp [$env lock_get $m $locker $obj ]
		lappend locklist $lockp
		error_check_good lock_get:b [is_blocked $lockp] 0
		error_check_good lock_get:b [is_substr $lockp $env] 1
	}
	release_list $locklist

	set locklist {}
	# Check that reference counted locks work
	puts "\tLock001.c: reference counted locks."
	for {set i 0} { $i < 10 } {incr i} {
		set lockp [$env lock_get -nowait write $locker $obj]
		error_check_good lock_get:c [is_blocked $lockp] 0
		error_check_good lock_get:c [is_substr $lockp $env] 1
		lappend locklist $lockp
	}
	release_list $locklist

	# Finally try some failing locks
	set locklist {}
	foreach i {ng write read} {
		set lockp [$env lock_get $i $locker $obj]
		lappend locklist $lockp
		error_check_good lock_get:d [is_blocked $lockp] 0
		error_check_good lock_get:d [is_substr $lockp $env] 1
	}

	# Change the locker
	set locker [$env lock_id]
	set blocklist {}
	# Skip NO_LOCK lock.
	puts "\tLock001.d: Change the locker, acquire read and write."
	foreach i {write read} {
		catch {$env lock_get -nowait $i $locker $obj} ret
		error_check_good lock_get:e [is_substr $ret "not granted"] 1
		#error_check_good lock_get:e [is_substr $lockp $env] 1
		#error_check_good lock_get:e [is_blocked $lockp] 0
	}
	# Now release original locks
	release_list $locklist

	# Now re-acquire blocking locks
	set locklist {}
	puts "\tLock001.e: Re-acquire blocking locks."
	foreach i {write read} {
		set lockp [$env lock_get -nowait $i $locker $obj ]
		error_check_good lock_get:f [is_substr $lockp $env] 1
		error_check_good lock_get:f [is_blocked $lockp] 0
		lappend locklist $lockp
	}

	# Now release new locks
	release_list $locklist
	error_check_good free_id [$env lock_id_free $locker] 0

	error_check_good envclose [$env close] 0

}

# Blocked locks appear as lockmgrN.lockM\nBLOCKED
proc is_blocked { l } {
	if { [string compare $l BLOCKED ] == 0 } {
		return 1
	} else {
		return 0
	}
}
