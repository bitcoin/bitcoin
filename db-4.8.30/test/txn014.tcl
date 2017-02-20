# See the file LICENSE for redistribution information.
#
# Copyright (c) 2005-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	txn014
# TEST	Test of parent and child txns working on the same database.
# TEST	A txn that will become a parent create a database.
# TEST	A txn that will not become a parent creates another database.
# TEST	Start a child txn of the 1st txn.
# TEST  Verify that the parent txn is disabled while child is open.
# TEST	1. Child reads contents with child handle (should succeed).
# TEST	2. Child reads contents with parent handle (should succeed).
# TEST  Verify that the non-parent txn can read from its database,
# TEST	and that the child txn cannot.
# TEST	Return to the child txn.
# TEST	3. Child writes with child handle (should succeed).
# TEST	4. Child writes with parent handle (should succeed).
# TEST
# TEST	Commit the child, verify that the parent can write again.
# TEST	Check contents of database with a second child.
proc txn014 { } {
	source ./include.tcl
	global default_pagesize

	set page_size $default_pagesize
	# If the page size is very small, we increase page size,
	# so we won't run out of lockers.
	if { $page_size < 2048 } {
		set page_size 2048
	}
	set tnum "014"
	puts "Txn$tnum: Test use of parent and child txns."
	set parentfile test$tnum.db
	set nonparentfile test$tnum.db.2
	set method "-btree"

	# Use 5000 entries so there will be new items on the wordlist
	# when we double nentries in part h.
	set nentries 5000

	env_cleanup $testdir

	puts "\tTxn$tnum.a: Create environment."
	set eflags "-create -mode 0644 -txn -home $testdir"
	set env [eval {berkdb_env_noerr} $eflags]
	error_check_good env [is_valid_env $env] TRUE

	# Open a database with parent txn and populate.  We populate
	# before starting up the child txn, because the only allowed
	# Berkeley DB calls for a parent txn are beginning child txns,
	# committing, or aborting.

	puts "\tTxn$tnum.b: Start parent txn and open database."
	set parent [$env txn]
	error_check_good parent_begin [is_valid_txn $parent $env] TRUE
	set db [berkdb_open_noerr -pagesize $page_size \
	    -env $env -txn $parent -create $method $parentfile]
	populate $db $method $parent $nentries 0 0

	puts "\tTxn$tnum.c: Start non-parent txn and open database."
	set nonparent [$env txn]
	error_check_good nonparent_begin [is_valid_txn $nonparent $env] TRUE
	set db2 [berkdb_open_noerr -pagesize $page_size \
	    -env $env -txn $nonparent -create $method $nonparentfile]
	populate $db2 $method $nonparent $nentries 0 0

	# Start child txn and open database.  Parent txn is not yet
	# committed, but the child should be able to read what's there.
	# The child txn should also be able to use the parent txn.

	puts "\tTxn$tnum.d: Start child txn."
	set child [$env txn -parent $parent]

	puts "\tTxn$tnum.e: Verify parent is disabled."
	catch {$db put -txn $parent a a} ret
	error_check_good \
	    parent_disabled [is_substr $ret "Child transaction is active"] 1

	puts "\tTxn$tnum.f: Get a handle on parent's database using child txn."
	set childdb [berkdb_open_noerr -pagesize $page_size \
	    -env $env -txn $child $method $parentfile]

	puts "\tTxn$tnum.g: Read database with child txn/child handle,"
	puts "\tTxn$tnum.g:     and with child txn/parent handle."
	set did [open $dict]
	set count 0
	while { [gets $did str] != -1 && $count < $nentries } {
		set key $str

		# First use child's handle.
		set ret [$childdb get -txn $child $key]
		error_check_good \
		    get $ret [list [list $key [pad_data $method $str]]]

		# Have the child use the parent's handle.
		set ret [$db get -txn $child $key]
		error_check_good \
		    get $ret [list [list $key [pad_data $method $str]]]
		incr count
	}
	close $did

	# Read the last key from the non-parent database, then try
	# to read the same key using the child txn.  It will fail.
	puts "\tTxn$tnum.h: Child cannot read data from non-parent."
	set ret [$db2 get -txn $nonparent $key]

	# Check the return against $key, because $str has gone on to
	# the next item in the wordlist.
	error_check_good \
	    np_get $ret [list [list $key [pad_data $method $key]]]
	catch {$db2 get -txn $child $key} ret
	error_check_good \
	    child_np_get [is_substr $ret "is still active"] 1

	# The child should also be able to update the database, using
	# either handle.
	puts "\tTxn$tnum.i: Write to database with child txn & child handle."
	populate $childdb $method $child $nentries 0 0
	puts "\tTxn$tnum.j: Write to database with child txn & parent handle."
	populate $db $method $child $nentries 0 0

	puts "\tTxn$tnum.k: Commit child, freeing parent."
	error_check_good child_commit [$child commit] 0
	error_check_good childdb_close [$childdb close] 0

	puts "\tTxn$tnum.l: Add more entries to db using parent txn."
	set nentries [expr $nentries * 2]
	populate $db $method $parent $nentries 0 0

	puts "\tTxn$tnum.m: Start new child txn and read database."
	set child2 [$env txn -parent $parent]
	set child2db [berkdb_open_noerr -pagesize $page_size \
	    -env $env -txn $child2 $method $parentfile]

	set did [open $dict]
	set count 0
	while { [gets $did str] != -1 && $count < $nentries } {
		set key $str
		set ret [$child2db get -txn $child2 $key]
		error_check_good \
		    get $ret [list [list $key [pad_data $method $str]]]	1
		incr count
	}
	close $did

	puts "\tTxn$tnum.n: Clean up."
	error_check_good child2_commit [$child2 commit] 0
	error_check_good nonparent_commit [$nonparent commit] 0
	error_check_good parent_commit [$parent commit] 0
	error_check_good db_close [$db close] 0
	error_check_good db2_close [$db2 close] 0
	error_check_good childdb_close [$child2db close] 0
	error_check_good env_close [$env close] 0
}

