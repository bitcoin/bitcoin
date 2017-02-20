# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	si006
# TEST
# TEST	Test -immutable_key interface.
# TEST
# TEST  DB_IMMUTABLE_KEY is an optimization to be used when a
# TEST	secondary key will not be changed.  It does not prevent
# TEST 	a deliberate change to the secondary key, it just does not
# TEST  propagate that change when it is made to the primary.
# TEST	This test verifies that a change to the primary is propagated
# TEST	to the secondary or not as specified by -immutable_key.

proc si006 { methods {nentries 200} {tnum "006"} args } {
	source ./include.tcl
	global dict

	# Primary method/args.
	set pmethod [lindex $methods 0]
	set pargs [convert_args $pmethod $args]
	set pomethod [convert_method $pmethod]

	# Renumbering recno databases can't be used as primaries.
	if { [is_rrecno $pmethod] == 1 } {
		puts "Skipping si$tnum for method $pmethod"
		return
	}

	# Method/args for all the secondaries.  If only one method
	# was specified, assume the same method (for btree or hash)
	# and a standard number of secondaries.  If primary is not
	# btree or hash, force secondaries to be one btree, one hash.
	set methods [lrange $methods 1 end]
	set nsecondaries 2
	if { [llength $methods] == 0 } {
		for { set i 0 } { $i < $nsecondaries } { incr i } {
			if { [is_btree $pmethod] || [is_hash $pmethod] } {
				lappend methods $pmethod
			} else {
				if { [expr $i % 2] == 0 } {
					lappend methods "-btree"
				} else {
					lappend methods "-hash"
				}
			}
		}
	}

	set argses [convert_argses $methods $args]
	set omethods [convert_methods $methods]

	# If we are given an env, use it.  Otherwise, open one.
	set eindex [lsearch -exact $args "-env"]
	if { $eindex == -1 } {
		env_cleanup $testdir
		set env [berkdb_env -create -home $testdir]
		error_check_good env_open [is_valid_env $env] TRUE
	} else {
		incr eindex
		set env [lindex $args $eindex]
		set envflags [$env get_open_flags]
		if { [lsearch -exact $envflags "-thread"] != -1 &&\
			[is_queue $pmethod] == 1 } {
			puts "Skipping si$tnum for threaded env"
			return
		}
		set testdir [get_home $env]
	}

	puts "si$tnum \{\[ list $pmethod $methods \]\} $nentries"
	cleanup $testdir $env

	set pname "primary$tnum.db"
	set snamebase "secondary$tnum"

	# Open the primary.
	set pdb [eval {berkdb_open -create -env} $env $pomethod $pargs $pname]
	error_check_good primary_open [is_valid_db $pdb] TRUE

	# Open and associate the secondaries, without -immutable_key.
	puts "\tSi$tnum.a: Open primary and secondary databases and associate."
	set sdbs {}

	set sdb1 [eval {berkdb_open -create -env} $env \
	    [lindex $omethods 0] [lindex $argses 0] $snamebase.1.db]
	error_check_good open_sdb1 [is_valid_db $sdb1] TRUE
	error_check_good sdb1_associate \
	    [$pdb associate [callback_n 0] $sdb1] 0
	lappend sdbs $sdb1

	set sdb2 [eval {berkdb_open -create -env} $env \
	    [lindex $omethods 1] [lindex $argses 1] $snamebase.2.db]
	error_check_good open_sdb2 [is_valid_db $sdb2] TRUE
	error_check_good sdb2_associate \
	    [$pdb associate [callback_n 1] $sdb2] 0
	lappend sdbs $sdb2

	puts "\tSi$tnum.b: Put loop on primary database."
	set did [open $dict]
	for { set n 0 } { [gets $did str] != -1 && $n < $nentries } { incr n } {
		if { [is_record_based $pmethod] == 1 } {
			set key [expr $n + 1]
			set datum $str
		} else {
			set key $str
			gets $did datum
		}
		set keys($n) $key
		set data($n) [pad_data $pmethod $datum]

		set ret [eval {$pdb put} {$key [chop_data $pmethod $datum]}]
		error_check_good put($n) $ret 0
	}
	close $did

	puts "\tSi$tnum.c: Check secondaries."
	check_secondaries $pdb $sdbs $nentries keys data "Si$tnum.c"

	puts "\tSi$tnum.d: Closing/disassociating primary first"
	error_check_good primary_close [$pdb close] 0
	foreach sdb $sdbs {
		error_check_good secondary_close [$sdb close] 0
	}

	puts "\tSi$tnum.e: Reopen databases."
	# Reopen the primary.
	set pdb [eval {berkdb_open -env} $env $pname]
	error_check_good primary_reopen [is_valid_db $pdb] TRUE

	# Reopen and associate secondary without -immutable_key.
	set mutable {}
	set sdb1 [eval {berkdb_open -create -env} $env \
	    [lindex $omethods 0] [lindex $argses 0] $snamebase.1.db]
	error_check_good open_sdb1 [is_valid_db $sdb1] TRUE
	error_check_good sdb1_associate \
	    [$pdb associate [callback_n 0] $sdb1] 0
	lappend goodsdbs $mutable

	# Reopen and associate second secondary with -immutable_key.
	set immutable {}
	set sdb2 [eval {berkdb_open -env} $env \
	    [lindex $omethods 1] [lindex $argses 1] $snamebase.2.db]
	error_check_good reopen_sdb2 [is_valid_db $sdb2] TRUE
	error_check_good sdb2_associate \
	    [$pdb associate -immutable_key [callback_n 1] $sdb2] 0
	lappend immutable $sdb2

	# Update primary.  This should write to sdb1, but not sdb2.
	puts "\tSi$tnum.f: Put loop on primary database."
	set str "OVERWRITTEN"
	for { set n 0 } { $n < $nentries } { incr n } {
		if { [is_record_based $pmethod] == 1 } {
			set key [expr $n + 1]
		} else {
			set key $keys($n)
		}
		set datum $str.$n
		set data($n) [pad_data $pmethod $datum]
		set ret [eval {$pdb put} {$key [chop_data $pmethod $datum]}]
		error_check_good put($n) $ret 0
	}

	puts "\tSi$tnum.g: Check secondaries without -immutable_key."
	check_secondaries $pdb $mutable $nentries keys data "Si$tnum.g"

	puts "\tSi$tnum.h: Check secondaries with -immutable_key."
	if { [catch {check_secondaries \
	    $pdb $immutable $nentries keys data "Si$tnum.h"} res] != 1 } {
	    	puts "FAIL: Immutable secondary key was changed."
	}

	puts "\tSi$tnum.i: Closing/disassociating primary first"
	error_check_good primary_close [$pdb close] 0
	error_check_good secondary1_close [$sdb1 close] 0
	error_check_good secondary2_close [$sdb2 close] 0

	# Don't close the env if this test was given one.
	if { $eindex == -1 } {
		error_check_good env_close [$env close] 0
	}
}

