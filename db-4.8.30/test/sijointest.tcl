# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	sijointest: Secondary index and join test.
# TEST 	This used to be si005.tcl.
proc sijointest { methods {nentries 1000} args } {
	source ./include.tcl

	# Primary method/args.
	set pmethod [lindex $methods 0]
	set pargs [convert_args $pmethod $args]
	set pomethod [convert_method $pmethod]

	# Si005 does a join within a simulated database schema
	# in which the primary index maps a record ID to a ZIP code and
	# name in the form "XXXXXname", and there are two secondaries:
	# one mapping ZIP to ID, the other mapping name to ID.
	# The primary may be of any database type;  the two secondaries
	# must be either btree or hash.

	# Method/args for all the secondaries.  If only one method
	# was specified, assume the same method for the two secondaries.
	set methods [lrange $methods 1 end]
	if { [llength $methods] == 0 } {
		for { set i 0 } { $i < 2 } { incr i } {
			lappend methods $pmethod
		}
	} elseif { [llength $methods] != 2 } {
		puts "FAIL: Sijoin requires exactly two secondaries."
		return
	}

	set argses [convert_argses $methods $args]
	set omethods [convert_methods $methods]

	puts "Secondary index join test."
	puts "sijoin  \{\[ list $pmethod $methods \]\} $nentries"
	env_cleanup $testdir

	set pname "sijoin-primary.db"
	set zipname "sijoin-zip.db"
	set namename "sijoin-name.db"

	# Open an environment
	# XXX if one is not supplied!
	set env [berkdb_env -create -home $testdir]
	error_check_good env_open [is_valid_env $env] TRUE

	# Open the databases.
	set pdb [eval {berkdb_open -create -env} $env $pomethod $pargs $pname]
	error_check_good primary_open [is_valid_db $pdb] TRUE

	set zipdb [eval {berkdb_open -create -dup -env} $env \
	    [lindex $omethods 0] [lindex $argses 0] $zipname]
	error_check_good zip_open [is_valid_db $zipdb] TRUE
	error_check_good zip_associate [$pdb associate sj_getzip $zipdb] 0

	set namedb [eval {berkdb_open -create -dup -env} $env \
	    [lindex $omethods 1] [lindex $argses 1] $namename]
	error_check_good name_open [is_valid_db $namedb] TRUE
	error_check_good name_associate [$pdb associate sj_getname $namedb] 0

	puts "\tSijoin.a: Populate database with $nentries \"names\""
	sj_populate $pdb $nentries
	puts "\tSijoin.b: Perform a join on each \"name\" and \"ZIP\""
	sj_jointest $pdb $zipdb $namedb

	error_check_good name_close [$namedb close] 0
	error_check_good zip_close [$zipdb close] 0
	error_check_good primary_close [$pdb close] 0
	error_check_good env_close [$env close] 0
}

proc sj_jointest { pdb zipdb namedb } {
	set pdbc [$pdb cursor]
	error_check_good pdb_cursor [is_valid_cursor $pdbc $pdb] TRUE
	for { set dbt [$pdbc get -first] } { [llength $dbt] > 0 } \
	    { set dbt [$pdbc get -next] } {
		set item [lindex [lindex $dbt 0] 1]
		set retlist [sj_dojoin $item $pdb $zipdb $namedb]
	}
}

proc sj_dojoin { item pdb zipdb namedb } {
	set name [sj_getname "" $item]
	set zip [sj_getzip "" $item]

	set zipc [$zipdb cursor]
	error_check_good zipc($item) [is_valid_cursor $zipc $zipdb] TRUE

	set namec [$namedb cursor]
	error_check_good namec($item) [is_valid_cursor $namec $namedb] TRUE

	set pc [$pdb cursor]
	error_check_good pc($item) [is_valid_cursor $pc $pdb] TRUE

	set ret [$zipc get -set $zip]
	set zd [lindex [lindex $ret 0] 1]
	error_check_good zipset($zip) [sj_getzip "" $zd] $zip

	set ret [$namec get -set $name]
	set nd [lindex [lindex $ret 0] 1]
	error_check_good nameset($name) [sj_getname "" $nd] $name

	set joinc [$pdb join $zipc $namec]

	set anyreturned 0
	for { set dbt [$joinc get] } { [llength $dbt] > 0 } \
	    { set dbt [$joinc get] } {
		set ritem [lindex [lindex $dbt 0] 1]
		error_check_good returned_item($item) $ritem $item
		incr anyreturned
	}
	error_check_bad anyreturned($item) $anyreturned 0

	error_check_good joinc_close($item) [$joinc close] 0
	error_check_good pc_close($item) [$pc close] 0
	error_check_good namec_close($item) [$namec close] 0
	error_check_good zipc_close($item) [$zipc close] 0
}

proc sj_populate { db nentries } {
	global dict

	set did [open $dict]
	for { set i 1 } { $i <= $nentries } { incr i } {
		gets $did word
		if { [string length $word] < 3 } {
			gets $did word
			if { [string length $word] < 3 } {
				puts "FAIL:\
				    unexpected pair of words < 3 chars long"
			}
		}
		set datalist [sj_name2zips $word]
		foreach data $datalist {
			error_check_good db_put($data) [$db put $i $data$word] 0
		}
	}
	close $did
}

proc sj_getzip { key data } { return [string range $data 0 4] }
proc sj_getname { key data } { return [string range $data 5 end] }

# The dirty secret of this test is that the ZIP code is a function of the
# name, so we can generate a database and then verify join results easily
# without having to consult actual data.
#
# Any word passed into this function will generate from 1 to 26 ZIP
# entries, out of the set {00000, 01000 ... 99000}.  The number of entries
# is just the position in the alphabet of the word's first letter;  the
# entries are then hashed to the set {00, 01 ... 99} N different ways.
proc sj_name2zips { name } {
	global alphabet

	set n [expr [string first [string index $name 0] $alphabet] + 1]
	error_check_bad starts_with_abc($name) $n -1

	set ret {}
	for { set i 0 } { $i < $n } { incr i } {
		set b 0
		for { set j 1 } { $j < [string length $name] } \
		    { incr j } {
			set b [sj_nhash $name $i $j $b]
		}
		lappend ret [format %05u [expr $b % 100]000]
	}
	return $ret
}
proc sj_nhash { name i j b } {
	global alphabet

	set c [string first [string index $name $j] $alphabet']
	return [expr (($b * 991) + ($i * 997) + $c) % 10000000]
}
