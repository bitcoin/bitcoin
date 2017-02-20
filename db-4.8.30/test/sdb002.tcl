# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	sdb002
# TEST	Tests basic subdb functionality
# TEST		Small keys, small data
# TEST		Put/get per key
# TEST		Dump file
# TEST		Close, reopen
# TEST		Dump file
# TEST
# TEST	Use the first 10,000 entries from the dictionary.
# TEST	Insert each with self as key and data; retrieve each.
# TEST	After all are entered, retrieve all; compare output to original.
# TEST	Close file, reopen, do retrieve and re-verify.
# TEST	Then repeat using an environment.
proc sdb002 { method {nentries 10000} args } {
	global passwd
	global has_crypto

	set eindex [lsearch -exact $args "-env"]
	if { $eindex != -1 } {
		set env NULL
		incr eindex
		set env [lindex $args $eindex]
		puts "Subdb002 skipping for env $env"
		return
	}
	set largs $args
	subdb002_main $method $nentries $largs
	append largs " -chksum "
	subdb002_main $method $nentries $largs

	# Skip remainder of test if release does not support encryption.
	if { $has_crypto == 0 } {
		return
	}

	append largs "-encryptaes $passwd "
	subdb002_main $method $nentries $largs
}

proc subdb002_main { method nentries largs } {
	source ./include.tcl
	global encrypt

	set largs [convert_args $method $largs]
	set omethod [convert_method $method]

	env_cleanup $testdir

	puts "Subdb002: $method ($largs) basic subdb tests"
	set testfile $testdir/subdb002.db
	subdb002_body $method $omethod $nentries $largs $testfile NULL

	# Run convert_encrypt so that old_encrypt will be reset to
	# the proper value and cleanup will work.
	convert_encrypt $largs
	set encargs ""
	set largs [split_encargs $largs encargs]

	cleanup $testdir NULL
	if { [is_queue $omethod] == 1 } {
		set sdb002_env berkdb_env_noerr
	} else {
		set sdb002_env berkdb_env
	}
	set env [eval {$sdb002_env -create -cachesize {0 10000000 0} \
	    -mode 0644} -home $testdir $encargs]
	error_check_good env_open [is_valid_env $env] TRUE
	puts "Subdb002: $method ($largs) basic subdb tests in an environment"

	# We're in an env--use default path to database rather than specifying
	# it explicitly.
	set testfile subdb002.db
	subdb002_body $method $omethod $nentries $largs $testfile $env
	error_check_good env_close [$env close] 0
}

proc subdb002_body { method omethod nentries largs testfile env } {
	global encrypt
	global passwd
	source ./include.tcl

	# Create the database and open the dictionary
	set subdb subdb0
	set t1 $testdir/t1
	set t2 $testdir/t2
	set t3 $testdir/t3

	if { [is_queue $omethod] == 1 } {
		set sdb002_open berkdb_open_noerr
	} else {
		set sdb002_open berkdb_open
	}

	if { $env == "NULL" } {
		set ret [catch {eval {$sdb002_open -create -mode 0644} $largs \
		    {$omethod $testfile $subdb}} db]
	} else {
		set ret [catch {eval {$sdb002_open -create -mode 0644} $largs \
		    {-env $env $omethod $testfile $subdb}} db]
	}

	#
	# If -queue method, we need to make sure that trying to
	# create a subdb fails.
	if { [is_queue $method] == 1 } {
		error_check_bad dbopen $ret 0
		puts "Subdb002: skipping remainder of test for method $method"
		return
	}

	error_check_good dbopen $ret 0
	error_check_good dbopen [is_valid_db $db] TRUE

	set did [open $dict]

	set pflags ""
	set gflags ""
	set count 0

	if { [is_record_based $method] == 1 } {
		set checkfunc subdb002_recno.check
		append gflags " -recno"
	} else {
		set checkfunc subdb002.check
	}
	puts "\tSubdb002.a: put/get loop"
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
	# Now we will get each key from the DB and compare the results
	# to the original.
	puts "\tSubdb002.b: dump file"
	set txn ""
	dump_file $db $txn $t1 $checkfunc
	error_check_good db_close [$db close] 0

	# Now compare the keys to see if they match the dictionary (or ints)
	if { [is_record_based $method] == 1 } {
		set oid [open $t2 w]
		for {set i 1} {$i <= $nentries} {set i [incr i]} {
			puts $oid $i
		}
		close $oid
		file rename -force $t1 $t3
	} else {
		set q q
		filehead $nentries $dict $t3
		filesort $t3 $t2
		filesort $t1 $t3
	}

	error_check_good Subdb002:diff($t3,$t2) \
	    [filecmp $t3 $t2] 0

	puts "\tSubdb002.c: close, open, and dump file"
	# Now, reopen the file and run the last test again.
	open_and_dump_subfile $testfile $env $t1 $checkfunc \
	    dump_file_direction "-first" "-next" $subdb
	if { [is_record_based $method] != 1 } {
		filesort $t1 $t3
	}

	error_check_good Subdb002:diff($t2,$t3) \
	    [filecmp $t2 $t3] 0

	# Now, reopen the file and run the last test again in the
	# reverse direction.
	puts "\tSubdb002.d: close, open, and dump file in reverse direction"
	open_and_dump_subfile $testfile $env $t1 $checkfunc \
	    dump_file_direction "-last" "-prev" $subdb

	if { [is_record_based $method] != 1 } {
		filesort $t1 $t3
	}

	error_check_good Subdb002:diff($t3,$t2) \
	    [filecmp $t3 $t2] 0

	puts "\tSubdb002.e: db_dump with subdatabase"
	set outfile $testdir/subdb002.dump
	set dumpargs " -f $outfile -s $subdb "
	if { $encrypt > 0 } {
		append dumpargs " -P $passwd "
	}
	if { $env != "NULL" } {
		append dumpargs " -h $testdir "
	}
	append dumpargs " $testfile"
	set stat [catch {eval {exec $util_path/db_dump} $dumpargs} ret]
	error_check_good dbdump.subdb $stat 0
}

# Check function for Subdb002; keys and data are identical
proc subdb002.check { key data } {
	error_check_good "key/data mismatch" $data $key
}

proc subdb002_recno.check { key data } {
	global dict
	global kvals

	error_check_good key"$key"_exists [info exists kvals($key)] 1
	error_check_good "key/data mismatch, key $key" $data $kvals($key)
}
