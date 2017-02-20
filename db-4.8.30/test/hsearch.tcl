# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# Historic Hsearch interface test.
# Use the first 1000 entries from the dictionary.
# Insert each with self as key and data; retrieve each.
# After all are entered, retrieve all; compare output to original.
# Then reopen the file, re-retrieve everything.
# Finally, delete everything.
proc hsearch { { nentries 1000 } } {
	source ./include.tcl

	puts "HSEARCH interfaces test: $nentries"

	# Create the database and open the dictionary
	set t1 $testdir/t1
	set t2 $testdir/t2
	set t3 $testdir/t3
	cleanup $testdir NULL

	error_check_good hcreate [berkdb hcreate $nentries] 0
	set did [open $dict]
	set count 0

	puts "\tHSEARCH.a: put/get loop"
	# Here is the loop where we put and get each key/data pair
	while { [gets $did str] != -1 && $count < $nentries } {
		set ret [berkdb hsearch $str $str enter]
		error_check_good hsearch:enter $ret 0

		set d [berkdb hsearch $str 0 find]
		error_check_good hsearch:find $d $str
		incr count
	}
	close $did

	puts "\tHSEARCH.b: re-get loop"
	set did [open $dict]
	# Here is the loop where we retrieve each key
	while { [gets $did str] != -1 && $count < $nentries } {
		set d [berkdb hsearch $str 0 find]
		error_check_good hsearch:find $d $str
		incr count
	}
	close $did
	error_check_good hdestroy [berkdb hdestroy] 0
}
