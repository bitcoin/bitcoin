# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	si004
# TEST	si002 with secondaries created and closed mid-test
# TEST	Basic cursor-based secondary index put/delete test, with
# TEST	secondaries created mid-test.
proc si004 { methods {nentries 200} {tnum "004"} args } {
	source ./include.tcl
	global dict nsecondaries

	# There's no reason to run this test on large lists.
	if { $nentries > 1000 } {
		puts "Skipping si004 for large lists (over 1000 items)."
		return
	}

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
		if { [lsearch -exact $envflags "-thread"] != -1 } {
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

	# Populate with a cursor put, exercising keyfirst/keylast.
	set did [open $dict]
	puts -nonewline \
	    "\tSi$tnum.a: Cursor put (-keyfirst/-keylast) loop ... "
	set pdbc [$pdb cursor]
	error_check_good pdb_cursor [is_valid_cursor $pdbc $pdb] TRUE
	for { set n 0 } \
	    { [gets $did str] != -1 && $n < $nentries } { incr n } {
		if { [is_record_based $pmethod] == 1 } {
			set key [expr $n + 1]
			set datum $str
		} else {
			set key $str
			gets $did datum
		}

		set ns($key) $n
		set keys($n) $key
		set data($n) [pad_data $pmethod $datum]

		if { $n % 2 == 0 } {
			set pflag " -keyfirst "
		} else {
			set pflag " -keylast "
		}

		set ret [eval {$pdbc put} $pflag \
		    {$key [chop_data $pmethod $datum]}]
		error_check_good put($n) $ret 0
	}
	error_check_good pdbc_close [$pdbc close] 0
	close $did

	# Open and associate the secondaries
	set sdbs {}
	puts "\n\t\topening secondaries."
	for { set i 0 } { $i < [llength $omethods] } { incr i } {
		set sdb [eval {berkdb_open -create -env} $env \
		    [lindex $omethods $i] [lindex $argses $i] $snamebase.$i.db]
		error_check_good second_open($i) [is_valid_db $sdb] TRUE

		error_check_good db_associate($i) \
		    [$pdb associate -create [callback_n $i] $sdb] 0
		lappend sdbs $sdb
	}
	check_secondaries $pdb $sdbs $nentries keys data "Si$tnum.a"

	puts "\tSi$tnum.b: Cursor put overwrite (-current) loop"
	set pdbc [$pdb cursor]
	error_check_good pdb_cursor [is_valid_cursor $pdbc $pdb] TRUE
	for { set dbt [$pdbc get -first] } { [llength $dbt] > 0 } \
	    { set dbt [$pdbc get -next] } {
		set key [lindex [lindex $dbt 0] 0]
		set datum [lindex [lindex $dbt 0] 1]
		set newd $datum.$key
		set ret [eval {$pdbc put -current} [chop_data $pmethod $newd]]
		error_check_good put_overwrite($key) $ret 0
		set data($ns($key)) [pad_data $pmethod $newd]
	}
	error_check_good pdbc_close [$pdbc close] 0
	check_secondaries $pdb $sdbs $nentries keys data "Si$tnum.b"

	puts -nonewline "\tSi$tnum.c:\
	    Secondary c_pget/primary put overwrite loop ... "
	# We walk the first secondary, then put-overwrite each primary key/data
	# pair we find.  This doubles as a DBC->c_pget test.
	set sdb [lindex $sdbs 0]
	set sdbc [$sdb cursor]
	error_check_good sdb_cursor [is_valid_cursor $sdbc $sdb] TRUE
	for { set dbt [$sdbc pget -first] } { [llength $dbt] > 0 } \
	    { set dbt [$sdbc pget -next] } {
		set pkey [lindex [lindex $dbt 0] 1]
		set pdatum [lindex [lindex $dbt 0] 2]

		# Extended entries will be showing up underneath us, in
		# unpredictable places.  Keep track of which pkeys
		# we've extended, and don't extend them repeatedly.
		if { [info exists pkeys_done($pkey)] == 1 } {
			continue
		} else {
			set pkeys_done($pkey) 1
		}

		set newd $pdatum.[string range $pdatum 0 2]
		set ret [eval {$pdb put} {$pkey [chop_data $pmethod $newd]}]
		error_check_good pdb_put($pkey) $ret 0
		set data($ns($pkey)) [pad_data $pmethod $newd]
	}
	error_check_good sdbc_close [$sdbc close] 0

	# Close the secondaries again.
	puts "\n\t\tclosing secondaries."
	for { set sdb [lindex $sdbs end] } { [string length $sdb] > 0 } \
	    { set sdb [lindex $sdbs end] } {
		error_check_good second_close($sdb) [$sdb close] 0
		set sdbs [lrange $sdbs 0 end-1]
		check_secondaries \
		    $pdb $sdbs $nentries keys data "Si$tnum.c"
	}

	# Delete the second half of the entries through the primary.
	# We do the second half so we can just pass keys(0 ... n/2)
	# to check_secondaries.
	set half [expr $nentries / 2]
	puts -nonewline "\tSi$tnum.d:\
	    Primary cursor delete loop: deleting $half entries ... "
	set pdbc [$pdb cursor]
	error_check_good pdb_cursor [is_valid_cursor $pdbc $pdb] TRUE
	set dbt [$pdbc get -first]
	for { set i 0 } { [llength $dbt] > 0 && $i < $half } { incr i } {
		error_check_good pdbc_del [$pdbc del] 0
		set dbt [$pdbc get -next]
	}
	error_check_good pdbc_close [$pdbc close] 0

	set sdbs {}
	puts "\n\t\topening secondaries."
	for { set i 0 } { $i < [llength $omethods] } { incr i } {
		set sdb [eval {berkdb_open -create -env} $env \
		    [lindex $omethods $i] [lindex $argses $i] \
		    $snamebase.r2.$i.db]
		error_check_good second_open($i) [is_valid_db $sdb] TRUE

		error_check_good db_associate($i) \
		    [$pdb associate -create [callback_n $i] $sdb] 0
		lappend sdbs $sdb
	}
	cursor_check_secondaries $pdb $sdbs $half "Si$tnum.d"

	# Delete half of what's left, through the first secondary.
	set quar [expr $half / 2]
	puts "\tSi$tnum.e:\
	    Secondary cursor delete loop: deleting $quar entries"
	set sdb [lindex $sdbs 0]
	set sdbc [$sdb cursor]
	set dbt [$sdbc get -first]
	for { set i 0 } { [llength $dbt] > 0 && $i < $quar } { incr i } {
		error_check_good sdbc_del [$sdbc del] 0
		set dbt [$sdbc get -next]
	}
	error_check_good sdbc_close [$sdbc close] 0
	cursor_check_secondaries $pdb $sdbs $quar "Si$tnum.e"

	foreach sdb $sdbs {
		error_check_good secondary_close [$sdb close] 0
	}
	error_check_good primary_close [$pdb close] 0

	# Close the env if it was created within this test.
	if { $eindex == -1 } {
		error_check_good env_close [$env close] 0
	}
}
