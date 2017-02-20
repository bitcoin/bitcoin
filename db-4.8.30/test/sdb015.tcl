# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	sdb015
# TEST	Tests basic in-memory named database functionality
# TEST		Small keys, small data
# TEST		Put/get per key
# TEST
# TEST	Use the first 10,000 entries from the dictionary.
# TEST	Insert each with self as key and data; retrieve each.
# TEST	After all are entered, retrieve all; compare output to original.
# TEST	Close file, reopen, do retrieve and re-verify.
# TEST	Then repeat using an environment.
proc sdb015 { method {nentries 1000} args } {
	global passwd
	global has_crypto

	if { [is_queueext $method] == 1 } {
		puts "Subdb015: skipping for method $method"
		return
	}

	# Skip test if given an env - this test needs its own.
	set eindex [lsearch -exact $args "-env"]
	if { $eindex != -1 } {
		incr eindex
		set env [lindex $args $eindex]
		puts "Subdb015 skipping for env $env"
		return
	}

	# In-memory dbs never go to disk, so we can't do checksumming.
	# If the test module sent in the -chksum arg, get rid of it.
	set chkindex [lsearch -exact $args "-chksum"]
	if { $chkindex != -1 } {
		set args [lreplace $args $chkindex $chkindex]
	}

	set largs $args
	subdb015_main $method $nentries $largs

	# Skip remainder of test if release does not support encryption.
	if { $has_crypto == 0 } {
		return
	}

	append largs " -encryptaes $passwd "
	subdb015_main $method $nentries $largs
}

proc subdb015_main { method nentries largs } {
	source ./include.tcl
	global encrypt

	set largs [convert_args $method $largs]
	set omethod [convert_method $method]

	env_cleanup $testdir

	# Run convert_encrypt so that old_encrypt will be reset to
	# the proper value and cleanup will work.
	convert_encrypt $largs
	set encargs ""
	set largs [split_encargs $largs encargs]

	set env [eval {berkdb_env -create -cachesize {0 10000000 0} \
	    -mode 0644} -home $testdir $encargs]
	error_check_good env_open [is_valid_env $env] TRUE

	puts "Subdb015: $method ($largs) basic in-memory named db tests."
	subdb015_body $method $omethod $nentries $largs $env
	error_check_good env_close [$env close] 0
}

proc subdb015_body { method omethod nentries largs env } {
	global encrypt
	global passwd
	source ./include.tcl

	# Create the database and open the dictionary
	set subdb subdb0
	set db [eval {berkdb_open -create -mode 0644} $largs \
	    {-env $env $omethod "" $subdb}]
	error_check_good dbopen [is_valid_db $db] TRUE

	set did [open $dict]
	set pflags ""
	set gflags ""
	set count 0

	puts "\tSubdb015.a: put/get loop"
	# Here is the loop where we put and get each key/data pair
	while { [gets $did str] != -1 && $count < $nentries } {
		if { [is_record_based $method] == 1 } {
			global kvals

			set key [expr $count + 1]
			set kvals($key) [pad_data $method $str]
		} else {
			set key $str
		}
		set ret [eval \
		    {$db put} $pflags {$key [chop_data $method $str]}]
		error_check_good put $ret 0

		set ret [eval {$db get} $gflags {$key}]
		error_check_good \
		    get $ret [list [list $key [pad_data $method $str]]]
		incr count
	}
	close $did
	error_check_good db_close [$db close] 0
}

