# See the file LICENSE for redistribution information.
#
# Copyright (c) 2003-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	lock006
# TEST	Test lock_vec interface.  We do all the same things that
# TEST	lock001 does, using lock_vec instead of lock_get and lock_put,
# TEST	plus a few more things like lock-coupling.
# TEST	1.  Get and release one at a time.
# TEST 	2.  Release with put_obj (all locks for a given locker/obj).
# TEST	3.  Release with put_all (all locks for a given locker).
# TEST	Regularly check lock_stat to verify all locks have been
# TEST	released.
proc lock006 { } {
	source ./include.tcl
	global lock_curid
	global lock_maxid

	set save_curid $lock_curid
	set save_maxid $lock_maxid

	# Cleanup
	env_cleanup $testdir

	# Open the region we'll use for testing.
	set eflags "-create -lock -home $testdir"
	set env [eval {berkdb_env} $eflags]
	error_check_good env [is_valid_env $env] TRUE
	error_check_good lock_id_set \
	     [$env lock_id_set $lock_curid $lock_maxid] 0

	puts "Lock006: test basic lock operations using lock_vec interface"
	set locker [$env lock_id]
	set modes {ng write read iwrite iread iwr}

	# Get and release each type of lock.
	puts "\tLock006.a: get and release one at a time"
	foreach m $modes {
		set obj obj$m
		set lockp [$env lock_vec $locker "get $obj $m"]
		error_check_good lock_vec_get:a [is_blocked $lockp] 0
		error_check_good lock_vec_get:a [is_valid_lock $lockp $env] TRUE
		error_check_good lock_vec_put:a \
		    [$env lock_vec $locker "put $lockp"] 0
	}
	how_many_locks 0 $env

	# Get a bunch of locks for the same locker; these should work
	set obj OBJECT
	puts "\tLock006.b: Get many locks for 1 locker,\
	    release with put_all."
	foreach m $modes {
		set lockp [$env lock_vec $locker "get $obj $m"]
		error_check_good lock_vec_get:b [is_blocked $lockp] 0
		error_check_good lock_vec_get:b [is_valid_lock $lockp $env] TRUE
	}
	how_many_locks 6 $env
	error_check_good release [$env lock_vec $locker put_all] 0
	how_many_locks 0 $env

	puts "\tLock006.c: Get many locks for 1 locker,\
	    release with put_obj."
	foreach m $modes {
		set lockp [$env lock_vec $locker "get $obj $m"]
		error_check_good lock_vec_get:b [is_blocked $lockp] 0
		error_check_good lock_vec_get:b [is_valid_lock $lockp $env] TRUE
	}
	error_check_good release [$env lock_vec $locker "put_obj $obj"] 0
#	how_many_locks 0 $env
	how_many_locks 6 $env

	# Get many locks for the same locker on more than one object.
	# Release with put_all.
	set obj2 OBJECT2
	puts "\tLock006.d: Get many locks on 2 objects for 1 locker,\
	    release with put_all."
	foreach m $modes {
		set lockp [$env lock_vec $locker "get $obj $m"]
		error_check_good lock_vec_get:b [is_blocked $lockp] 0
		error_check_good lock_vec_get:b [is_valid_lock $lockp $env] TRUE
	}
	foreach m $modes {
		set lockp [$env lock_vec $locker "get $obj2 $m"]
		error_check_good lock_vec_get:b [is_blocked $lockp] 0
		error_check_good lock_vec_get:b [is_valid_lock $lockp $env] TRUE
	}
	error_check_good release [$env lock_vec $locker put_all] 0
#	how_many_locks 0 $env
	how_many_locks 6 $env

	# Check that reference counted locks work.
	puts "\tLock006.e: reference counted locks."
	for {set i 0} { $i < 10 } {incr i} {
		set lockp [$env lock_vec -nowait $locker "get $obj write"]
		error_check_good lock_vec_get:c [is_blocked $lockp] 0
		error_check_good lock_vec_get:c [is_valid_lock $lockp $env] TRUE
	}
	error_check_good put_all [$env lock_vec $locker put_all] 0
#	how_many_locks 0 $env
	how_many_locks 6 $env

	# Lock-coupling.  Get a lock on object 1.  Get a lock on object 2,
	# release object 1, and so on.
	puts "\tLock006.f: Lock-coupling."
	set locker2 [$env lock_id]

	foreach m { read write iwrite iread iwr } {
		set lockp [$env lock_vec $locker "get OBJ0 $m"]
		set iter 0
		set nobjects 10
		while { $iter < 3 } {
			for { set i 1 } { $i <= $nobjects } { incr i } {
				set lockv [$env lock_vec $locker \
				    "get OBJ$i $m" "put $lockp"]

				# Make sure another locker can get an exclusive
				# lock on the object just released.
				set lock2p [$env lock_vec -nowait $locker2 \
				    "get OBJ[expr $i - 1] write" ]
				error_check_good release_lock2 [$env lock_vec \
				    $locker2 "put $lock2p"] 0

				# Make sure another locker can't get an exclusive
				# lock on the object just locked.
				catch {$env lock_vec -nowait $locker2 \
				    "get OBJ$i write"} ret
				error_check_good not_granted \
				    [is_substr $ret "not granted"] 1

				set lockp [lindex $lockv 0]
				if { $i == $nobjects } {
					incr iter
				}
			}
		}
		error_check_good lock_put [$env lock_vec $locker "put $lockp"] 0
#		how_many_locks 0 $env
		how_many_locks 6 $env
	}

	# Finally try some failing locks.  Set up a write lock on object.
	foreach m { write } {
		set lockp [$env lock_vec $locker "get $obj $m"]
		error_check_good lock_vec_get:d [is_blocked $lockp] 0
		error_check_good lock_vec_get:d [is_valid_lock $lockp $env] TRUE
	}

	# Change the locker
	set newlocker [$env lock_id]
	# Skip NO_LOCK.
	puts "\tLock006.g: Change the locker, try to acquire read and write."
	foreach m { read write iwrite iread iwr } {
		catch {$env lock_vec -nowait $newlocker "get $obj $m"} ret
		error_check_good lock_vec_get:d [is_substr $ret "not granted"] 1
	}

	# Now release original locks
	error_check_good put_all [$env lock_vec $locker {put_all}] 0
	error_check_good free_id [$env lock_id_free $locker] 0

	# Now re-acquire blocking locks
	puts "\tLock006.h: Re-acquire blocking locks."
	foreach m { read write iwrite iread iwr } {
		set lockp [$env lock_vec -nowait $newlocker "get $obj $m"]
		error_check_good lock_get:e [is_valid_lock $lockp $env] TRUE
		error_check_good lock_get:e [is_blocked $lockp] 0
	}

	# Now release new locks
	error_check_good put_all [$env lock_vec $newlocker {put_all}] 0
	error_check_good free_id [$env lock_id_free $newlocker] 0

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
