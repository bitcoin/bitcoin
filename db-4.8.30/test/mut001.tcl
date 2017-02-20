# See the file LICENSE for redistribution information.
#
# Copyright (c) 2009 Oracle.  All rights reserved.
#
# $Id$
#
#
# TEST	mut001
# TEST	Exercise the mutex API.
# 
# TEST	Allocate, lock, unlock, and free a bunch of mutexes. 
# TEST	Set basic configuration options and check mutex_stat and 
# TEST	the mutex getters for the correct values. 

proc mut001 { } {
	source ./include.tcl
	env_cleanup $testdir

	puts "Mut001: Basic mutex interface testing."

	# Open an env.
	set env [berkdb_env -create -home $testdir]
	
	# Allocate, lock, unlock, and free a bunch of mutexes.
	set nmutexes 100
	puts "\tMut001.a: Allocate a bunch of mutexes."
	for { set i 0 } { $i < $nmutexes } { incr i } {
		set mutexid($i) [$env mutex]
	}
	puts "\tMut001.b: Lock the mutexes."
	for { set i 0 } { $i < $nmutexes } { incr i } { 
		error_check_good mutex_lock [$env mutex_lock $mutexid($i)] 0
	}
	puts "\tMut001.c: Unlock the mutexes."
	for { set i 0 } { $i < $nmutexes } { incr i } { 
		error_check_good mutex_unlock [$env mutex_unlock $mutexid($i)] 0
	}
	puts "\tMut001.d: Free the mutexes."
	for { set i 0 } { $i < $nmutexes } { incr i } { 
		error_check_good mutex_free [$env mutex_free $mutexid($i)] 0
	}

	# Clean up the env.  We'll need new envs to test the configuration
	# options, because they cannot be set after the env is open.
	error_check_good env_close [$env close] 0	
	env_cleanup $testdir

	puts "\tMut001.e: Set the mutex alignment."
	set mutex_align 8
	set env [berkdb_env -create -home $testdir -mutex_set_align $mutex_align]

	set stat_align [stat_field $env mutex_stat "Mutex align"]
	set get_align [$env mutex_get_align]
	error_check_good stat_align $stat_align $mutex_align
	error_check_good get_align $get_align $mutex_align

	# Find the number of mutexes allocated by default.  We'll need
	# this later, when we try the "mutex_set_increment" option.
	set default_count [stat_field $env mutex_stat "Mutex count"]

	error_check_good env_close [$env close] 0	
	env_cleanup $testdir

	puts "\tMut001.f: Set the maximum number of mutexes."
	set mutex_count 2000
	set env [berkdb_env -create -home $testdir -mutex_set_max $mutex_count]

	set stat_count [stat_field $env mutex_stat "Mutex count"]
	set get_count [$env mutex_get_max]
	error_check_good stat_count $stat_count $mutex_count
	error_check_good get_count $get_count $mutex_count

	error_check_good env_close [$env close] 0	
	env_cleanup $testdir

	puts "\tMut001.g: Raise the maximum number of mutexes."
	set mutex_incr 500
	set mutex_count [expr $default_count + $mutex_incr]

	set env [berkdb_env -create -home $testdir -mutex_set_incr $mutex_incr]

	set stat_count [stat_field $env mutex_stat "Mutex count"]
	error_check_good stat_increment $stat_count $mutex_count 
	set get_count [$env mutex_get_max]
	error_check_good get_increment $get_count $mutex_count

	error_check_good env_close [$env close] 0	
	env_cleanup $testdir

	puts "\tMut001.h: Set and reset the number of TAS mutex spins."
	set mutex_tas_spins 50

	set env [berkdb_env -create -home $testdir -mutex_set_tas_spins $mutex_tas_spins] 
	set stat_spins [stat_field $env mutex_stat "Mutex TAS spins"]
	error_check_good stat_spins $stat_spins $mutex_tas_spins
	set get_spins [$env mutex_get_tas_spins]
	error_check_good get_spins $get_spins $mutex_tas_spins
	
	# TAS spins can be reset any time. 
	set mutex_tas_spins 1
	error_check_good reset_spins [$env mutex_set_tas_spins $mutex_tas_spins] 0
	set stat_spins [stat_field $env mutex_stat "Mutex TAS spins"]
	error_check_good stat_spins_reset $stat_spins $mutex_tas_spins
	set get_spins [$env mutex_get_tas_spins]
	error_check_good get_spins_reset $get_spins $mutex_tas_spins
	
	error_check_good env_close [$env close] 0	
	env_cleanup $testdir
}

