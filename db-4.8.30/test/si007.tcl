# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	si007
# TEST	Secondary index put/delete with lorder test
# TEST
# TEST	This test is the same as si001 with the exception
# TEST	that we create and populate the primary and THEN
# TEST	create the secondaries and associate them with -create.

proc si007 { methods {nentries 10} {tnum "007"} args } {
	source ./include.tcl
	global dict nsecondaries

	# Primary method/args.
	set pmethod [lindex $methods 0]
	set pargs [convert_args $pmethod $args]
	if [big_endian] {
		set nativeargs " -lorder 4321"
		set swappedargs " -lorder 1234"
	} else {
		set swappedargs " -lorder 4321"
		set nativeargs " -lorder 1234"
	}
	set argtypes "{$nativeargs} {$swappedargs}"
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

	set pname "primary$tnum.db"
	set snamebase "secondary$tnum"

	foreach pbyteargs $argtypes {
		foreach sbyteargs $argtypes {
			if { $pbyteargs == $nativeargs } {
				puts "Si$tnum: Using native\
				    byteorder $nativeargs for primary."
			} else {
				puts "Si$tnum: Using swapped\
				    byteorder $swappedargs for primary."
			}
			if { $sbyteargs == $nativeargs } {
				puts "Si$tnum: Using native\
				    byteorder $nativeargs for secondaries."
			} else {
				puts "Si$tnum: Using swapped\
				    byteorder $swappedargs for secondaries."
			}

			puts "si$tnum\
			    \{\[ list $pmethod $methods \]\} $nentries"
			cleanup $testdir $env

			# Open primary.
			set pdb [eval {berkdb_open -create -env} $env \
			    $pomethod $pargs $pbyteargs $pname]
			error_check_good primary_open [is_valid_db $pdb] TRUE

			puts "\tSi$tnum.a: Populate primary."
			# Open dictionary.  Leave it open until done
			# with test .e so append won't require
			# configuration for duplicates.
			set did [open $dict]
			for { set n 0 } \
			    { [gets $did str] != -1 && $n < $nentries } \
			    { incr n } {
				if { [is_record_based $pmethod] == 1 } {
					set key [expr $n + 1]
					set datum $str
				} else {
					set key $str
					gets $did datum
				}
				set keys($n) $key
				set data($n) [pad_data $pmethod $datum]

				set ret [eval {$pdb put}\
				    {$key [chop_data $pmethod $datum]}]
				error_check_good put($n) $ret 0
			}

			# Open and associate the secondaries, with -create.
			puts "\tSi$tnum.b: Associate secondaries with -create."
			set sdbs {}
			for { set i 0 } \
			    { $i < [llength $omethods] } { incr i } {
				set sdb [eval {berkdb_open -create -env} $env \
				    [lindex $omethods $i] [lindex $argses $i] \
				    $sbyteargs $snamebase.$i.db]
				error_check_good\
				    second_open($i) [is_valid_db $sdb] TRUE

				error_check_good db_associate($i) \
				    [$pdb associate -create [callback_n $i] $sdb] 0
				lappend sdbs $sdb
			}
			check_secondaries\
			    $pdb $sdbs $nentries keys data "Si$tnum.c"

			puts "\tSi$tnum.c: Closing/disassociating primary first"
			error_check_good primary_close [$pdb close] 0
			foreach sdb $sdbs {
				error_check_good secondary_close [$sdb close] 0
			}

			# Don't close the env if this test was given one.
			# Skip the test of truncating the secondary since
			# we can't close and reopen the outside env.
			if { $eindex == -1 } {
				error_check_good env_close [$env close] 0

				# Reopen with _noerr for test of
				# truncate secondary.
				puts "\tSi$tnum.h:\
				    Truncate secondary (should fail)"

				set env [berkdb_env_noerr\
				    -create -home $testdir]
				error_check_good\
				    env_open [is_valid_env $env] TRUE

				set pdb [eval {berkdb_open_noerr -create -env}\
				    $env $pomethod $pargs $pname]
				set sdb [eval {berkdb_open_noerr -create -env}\
				    $env [lindex $omethods 0]\
				    [lindex $argses 0] $snamebase.0.db ]
				$pdb associate [callback_n 0] $sdb

				set ret [catch {$sdb truncate} ret]
				error_check_good trunc_secondary $ret 1

				error_check_good primary_close [$pdb close] 0
				error_check_good secondary_close [$sdb close] 0
			}
		}
	}
	# If this test made the last env, close it.
	if { $eindex == -1 } {
		error_check_good env_close [$env close] 0
	}
}
