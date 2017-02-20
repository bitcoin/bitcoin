# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	sdb016
# TEST	Creates many in-memory named dbs and puts a small amount of
# TEST	data in each (many defaults to 100)
# TEST
# TEST	Use the first 100 entries from the dictionary as names.
# TEST	Insert each with entry as name of subdatabase and a partial list
# TEST	as key/data.  After all are entered, retrieve all; compare output
# TEST	to original.
proc sdb016 { method {nentries 100} args } {
	source ./include.tcl

	set tnum "016"
	set args [convert_args $method $args]
	set omethod [convert_method $method]

	if { [is_queueext $method] == 1 } {
		puts "Subdb$tnum skipping for method $method"
		return
	}

	puts "Subdb$tnum: $method ($args) many in-memory named databases"

        # Skip test if given an env - this test needs its own.
	set eindex [lsearch -exact $args "-env"]
	if { $eindex != -1 } {
		incr eindex
		set env [lindex $args $eindex]
		puts "\tSubdb$tnum skipping for env $env"
		return
	}

	# In-memory dbs never go to disk, so we can't do checksumming.
	# If the test module sent in the -chksum arg, get rid of it.
	set chkindex [lsearch -exact $args "-chksum"]
	if { $chkindex != -1 } {
		set args [lreplace $args $chkindex $chkindex]
	}

	env_cleanup $testdir

	# Set up env.  We'll need a big cache.
	set csize {0 16777216 1}
	set env [berkdb_env -create \
	    -cachesize $csize -home $testdir -mode 0644 -txn]
	error_check_good env_open [is_valid_env $env] TRUE

	set pflags ""
	set gflags ""
	set txn ""
	set fcount 0

	# Here is the loop where we put and get each key/data pair
	set ndataent 5
	set fdid [open $dict]
	puts "\tSubdb$tnum.a: Open $nentries in-memory databases."
	while { [gets $fdid str] != -1 && $fcount < $nentries } {
		if { $str == "" } {
			continue
		}
		set subdb $str
		set db [eval {berkdb_open -create -mode 0644} \
		    -env $env $args {$omethod "" $subdb}]
		error_check_good dbopen [is_valid_db $db] TRUE
		set count 0
		set did [open $dict]
		while { [gets $did str] != -1 && $count < $ndataent } {
			if { [is_record_based $method] == 1 } {
				global kvals

				set key [expr $count + 1]
				set kvals($key) [pad_data $method $str]
			} else {
				set key $str
			}
			set ret [eval {$db put} \
			    $txn $pflags {$key [chop_data $method $str]}]
			error_check_good put $ret 0
			set ret [eval {$db get} $gflags {$key}]
			error_check_good get $ret [list [list $key \
			    [pad_data $method $str]]]
			incr count
		}
		close $did
		error_check_good db_close [$db close] 0
		incr fcount
	}
	close $fdid

	puts "\tSubdb$tnum.b: Clean up."
	error_check_good env_close [$env close] 0
}

