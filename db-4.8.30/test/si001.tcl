# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	si001
# TEST	Secondary index put/delete with lorder test
# TEST
# TEST  Put data in primary db and check that pget on secondary
# TEST  index finds the right entries.  Alter the primary in the
# TEST  following ways, checking for correct data each time:
# TEST          Overwrite data in primary database.
# TEST          Delete half of entries through primary.
# TEST          Delete half of remaining entries through secondary.
# TEST          Append data (for record-based primaries only).
proc si001 { methods {nentries 200} {tnum "001"} args } {
	source ./include.tcl
	global dict nsecondaries
	global default_pagesize

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

	set mutexargs " -mutex_set_max 10000 "
	if { $default_pagesize <= 2048 } {
		set mutexargs "-mutex_set_max 40000 "
	}
	# If we are given an env, use it.  Otherwise, open one.
	set eindex [lsearch -exact $args "-env"]
	if { $eindex == -1 } {
		env_cleanup $testdir
		set cacheargs " -cachesize {0 4194304 1} "
		set env [eval {berkdb_env} -create \
		    $cacheargs $mutexargs -home $testdir]
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

			# Open and associate the secondaries
			set sdbs {}
			for { set i 0 } \
			    { $i < [llength $omethods] } { incr i } {
				set sdb [eval {berkdb_open -create -env} $env \
				    [lindex $omethods $i] [lindex $argses $i] \
				    $sbyteargs $snamebase.$i.db]
				error_check_good\
				    second_open($i) [is_valid_db $sdb] TRUE

				error_check_good db_associate($i) \
				    [$pdb associate [callback_n $i] $sdb] 0
				lappend sdbs $sdb
			}

			puts "\tSi$tnum.a: Put loop"
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
			close $did

			check_secondaries\
			    $pdb $sdbs $nentries keys data "Si$tnum.a"

			puts "\tSi$tnum.b: Put/overwrite loop"
			for { set n 0 } { $n < $nentries } { incr n } {
				set newd $data($n).$keys($n)
				set ret [eval {$pdb put}\
				    {$keys($n) [chop_data $pmethod $newd]}]
				error_check_good put_overwrite($n) $ret 0
				set data($n) [pad_data $pmethod $newd]
			}
			check_secondaries\
			    $pdb $sdbs $nentries keys data "Si$tnum.b"

			# Delete the second half of the entries through
			# the primary.  We do the second half so we can
			# just pass keys(0 ... n/2) to check_secondaries.
			set half [expr $nentries / 2]
			puts "\tSi$tnum.c:\
			    Primary delete loop: deleting $half entries"
			for { set n $half } { $n < $nentries } { incr n } {
				set ret [$pdb del $keys($n)]
				error_check_good pdel($n) $ret 0
			}
			check_secondaries\
			    $pdb $sdbs $half keys data "Si$tnum.c"

			# Delete half of what's left through
			# the first secondary.
			set quar [expr $half / 2]
			puts "\tSi$tnum.d:\
			    Secondary delete loop: deleting $quar entries"
			set sdb [lindex $sdbs 0]
			set callback [callback_n 0]
			for { set n $quar } { $n < $half } { incr n } {
				set skey [$callback $keys($n)\
				    [pad_data $pmethod $data($n)]]
				set ret [$sdb del $skey]
				error_check_good sdel($n) $ret 0
			}
			check_secondaries\
			    $pdb $sdbs $quar keys data "Si$tnum.d"
			set left $quar

			# For queue and recno only, test append, adding back
			# a quarter of the original number of entries.
			if { [is_record_based $pmethod] == 1 } {
			 	set did [open $dict]
				puts "\tSi$tnum.e:\
				    Append loop: append $quar entries"
				for { set n 0 } { $n < $nentries } { incr n } {
				    	# Skip over the dictionary entries
					# we've already used.
					gets $did str
				}
				for { set n $quar } \
				    { [gets $did str] != -1 && $n < $half } \
				    { incr n } {
					set key [expr $n + 1]
					set datum $str
					set keys($n) $key
					set data($n) [pad_data $pmethod $datum]

					set ret [eval {$pdb put} \
					    {$key [chop_data $pmethod $datum]}]
					error_check_good put($n) $ret 0
				}
				close $did

				check_secondaries\
				    $pdb $sdbs $half keys data "Si$tnum.e"
				set left $half
			}


			puts "\tSi$tnum.f:\
			    Truncate primary, check secondaries are empty."
			error_check_good truncate [$pdb truncate] $left
			foreach sdb $sdbs {
				set scursor [$sdb cursor]
				error_check_good\
				    db_cursor [is_substr $scursor $sdb] 1
				set ret [$scursor get -first]
				error_check_good\
				    sec_empty [string length $ret] 0
				error_check_good cursor_close [$scursor close] 0
			}


			puts "\tSi$tnum.g: Closing/disassociating primary first"
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

				set env [eval {berkdb_env_noerr}\
				    -create $mutexargs -home $testdir]
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
