# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	bigfile001
# TEST	Create a database greater than 4 GB in size.  Close, verify.
# TEST	Grow the database somewhat.  Close, reverify.  Lather, rinse,
# TEST	repeat.  Since it will not work on all systems, this test is
# TEST	not run by default.
proc bigfile001 { { itemsize 4096 } \
    { nitems 1048576 } { growby 5000 } { growtms 2 } args } {
	source ./include.tcl

	set method "btree"
	set args [convert_args $method $args]
	set omethod [convert_method $method]
	global is_fat32
	if { $is_fat32 } {
		puts "Skipping bigfile001 for FAT32 file system."
		return
	}
	puts "Bigfile001: $method ($args) $nitems * $itemsize bytes of data"

	env_cleanup $testdir

	# Create the database.  Use 64K pages;  we want a good fill
	# factor, and page size doesn't matter much.  Use a 50MB
	# cache;  that should be manageable, and will help
	# performance.
	set dbname $testdir/big.db

	set db [eval {berkdb_open -create} {-pagesize 65536 \
	    -cachesize {0 50000000 0}} $omethod $args $dbname]
	error_check_good db_open [is_valid_db $db] TRUE

	puts "\tBigfile001.a: Creating database..."
	flush stdout

	set data [string repeat z $itemsize]

	for { set i 0 } { $i < $nitems } { incr i } {
		set key key[format %08u $i]

		error_check_good db_put($i) [$db put $key $data] 0

		if { $i % 50000 == 0 } {
			set pct [expr 100 * $i / $nitems]
			puts "\tBigfile001.a: $pct%..."
			flush stdout
		}
	}
	puts "\tBigfile001.a: 100%."
	error_check_good db_close [$db close] 0

	puts "\tBigfile001.b: Verifying database..."
	error_check_good verify \
	    [verify_dir $testdir "\t\t" 0 0 1 50000000] 0

	puts "\tBigfile001.c: Grow database $growtms times by $growby items"

	for { set j 0 } { $j < $growtms } { incr j } {
		set db [eval {berkdb_open} {-cachesize {0 50000000 0}} $dbname]
		error_check_good db_open [is_valid_db $db] TRUE
		puts -nonewline "\t\tBigfile001.c.1: Adding $growby items..."
		flush stdout
		for { set i 0 } { $i < $growby } { incr i } {
			set key key[format %08u $i].$j
			error_check_good db_put($j.$i) [$db put $key $data] 0
		}
		error_check_good db_close [$db close] 0
		puts "done."

		puts "\t\tBigfile001.c.2: Verifying database..."
		error_check_good verify($j) \
		    [verify_dir $testdir "\t\t\t" 0 0 1 50000000] 0
	}
}
