# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	dbm
# TEST	Historic DBM interface test.  Use the first 1000 entries from the
# TEST	dictionary.  Insert each with self as key and data; retrieve each.
# TEST	After all are entered, retrieve all; compare output to original.
# TEST	Then reopen the file, re-retrieve everything.  Finally, delete
# TEST	everything.
proc dbm { { nentries 1000 } } {
	source ./include.tcl

	puts "DBM interfaces test: $nentries"

	# Create the database and open the dictionary
	set testfile $testdir/dbmtest
	set t1 $testdir/t1
	set t2 $testdir/t2
	set t3 $testdir/t3
	cleanup $testdir NULL

	error_check_good dbminit [berkdb dbminit $testfile] 0
	set did [open $dict]

	set flags ""
	set txn ""
	set count 0
	set skippednullkey 0

	puts "\tDBM.a: put/get loop"
	# Here is the loop where we put and get each key/data pair
	while { [gets $did str] != -1 && $count < $nentries } {
		# DBM can't handle zero-length keys
		if { [string length $str] == 0 } {
			set skippednullkey 1
			continue
		}

		set ret [berkdb store $str $str]
		error_check_good dbm_store $ret 0

		set d [berkdb fetch $str]
		error_check_good dbm_fetch $d $str
		incr count
	}
	close $did

	# Now we will get each key from the DB and compare the results
	# to the original.
	puts "\tDBM.b: dump file"
	set oid [open $t1 w]
	for { set key [berkdb firstkey] } { $key != -1 } {\
		 set key [berkdb nextkey $key] } {
		puts $oid $key
		set d [berkdb fetch $key]
		error_check_good dbm_refetch $d $key
	}

	# If we had to skip a zero-length key, juggle things to cover up
	# this fact in the dump.
	if { $skippednullkey == 1 } {
		puts $oid ""
		incr nentries 1
	}

	close $oid

	# Now compare the keys to see if they match the dictionary (or ints)
	set q q
	filehead $nentries $dict $t3
	filesort $t3 $t2
	filesort $t1 $t3

	error_check_good DBM:diff($t3,$t2) \
	    [filecmp $t3 $t2] 0

	puts "\tDBM.c: close, open, and dump file"

	# Now, reopen the file and run the last test again.
	error_check_good dbminit2 [berkdb dbminit $testfile] 0
	set oid [open $t1 w]

	for { set key [berkdb firstkey] } { $key != -1 } {\
		 set key [berkdb nextkey $key] } {
		puts $oid $key
		set d [berkdb fetch $key]
		error_check_good dbm_refetch $d $key
	}
	if { $skippednullkey == 1 } {
		puts $oid ""
	}
	close $oid

	# Now compare the keys to see if they match the dictionary (or ints)
	filesort $t1 $t3

	error_check_good DBM:diff($t2,$t3) \
	    [filecmp $t2 $t3] 0

	# Now, reopen the file and delete each entry
	puts "\tDBM.d: sequential scan and delete"

	error_check_good dbminit3 [berkdb dbminit $testfile] 0
	set oid [open $t1 w]

	for { set key [berkdb firstkey] } { $key != -1 } {\
		 set key [berkdb nextkey $key] } {
		puts $oid $key
		set ret [berkdb delete $key]
		error_check_good dbm_delete $ret 0
	}
	if { $skippednullkey == 1 } {
		puts $oid ""
	}
	close $oid

	# Now compare the keys to see if they match the dictionary (or ints)
	filesort $t1 $t3

	error_check_good DBM:diff($t2,$t3) \
	    [filecmp $t2 $t3] 0

	error_check_good "dbm_close" [berkdb dbmclose] 0
}
