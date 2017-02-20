# See the file LICENSE for redistribution information.
#
# Copyright (c) 2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	mut003
# TEST	Try doing mutex operations out of order.  Make sure 
# TEST	we get appropriate errors.

proc mut003 { } {
	source ./include.tcl
	env_cleanup $testdir

	puts "Mut003: Out of order mutex operations."

	# Allocate a mutex.  Try to unlock it before it's locked.
	puts "\tMut003.a: Try to unlock a mutex that's not locked."
	set env [berkdb_env_noerr -create -home $testdir]
	set mutex [$env mutex]
	catch { $env mutex_unlock $mutex } res
	error_check_good \
	    already_unlocked [is_substr $res "lock already unlocked"] 1
	env_cleanup $testdir
	
	# Allocate and lock a mutex.  Try to unlock it twice.
	puts "\tMut003.b: Try to unlock a mutex twice."
	set env [berkdb_env_noerr -create -home $testdir]
	set mutex [$env mutex]
	error_check_good mutex_lock [$env mutex_lock $mutex] 0
	error_check_good mutex_unlock [$env mutex_unlock $mutex] 0
	catch { $env mutex_unlock $mutex } res
	error_check_good \
	    already_unlocked [is_substr $res "lock already unlocked"] 1
	env_cleanup $testdir
	
	# Allocate and free a mutex.  Then try to lock it.
	puts "\tMut003.c: Try to lock a freed mutex."
	set env [berkdb_env_noerr -create -home $testdir]
	set mutex [$env mutex]
	error_check_good mutex_free [$env mutex_free $mutex] 0
	catch { $env mutex_lock $mutex } res

#	error_check_good \
#	    allocation_error [is_substr $res "lock already unlocked"] 1
	env_cleanup $testdir

	# Allocate and lock a mutex.  Try to free it before it's unlocked. 
	puts "\tMut003.d: Try to free a still-locked mutex."
	set env [berkdb_env_noerr -create -home $testdir]
	set mutex [$env mutex]
	error_check_good mutex_lock [$env mutex_lock $mutex] 0
	catch { $env mutex_free $mutex } res

#	error_check_good \
#	    allocation_error [is_substr $res "lock already unlocked"] 1
	env_cleanup $testdir
}

