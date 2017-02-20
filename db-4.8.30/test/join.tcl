# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	jointest
# TEST	Test duplicate assisted joins.  Executes 1, 2, 3 and 4-way joins
# TEST	with differing index orders and selectivity.
# TEST
# TEST	We'll test 2-way, 3-way, and 4-way joins and figure that if those
# TEST	work, everything else does as well.  We'll create test databases
# TEST	called join1.db, join2.db, join3.db, and join4.db.  The number on
# TEST	the database describes the duplication -- duplicates are of the
# TEST	form 0, N, 2N, 3N, ...  where N is the number of the database.
# TEST	Primary.db is the primary database, and null.db is the database
# TEST	that has no matching duplicates.
# TEST
# TEST	We should test this on all btrees, all hash, and a combination thereof
proc jointest { {psize 8192} {with_dup_dups 0} {flags 0} } {
	global testdir
	global rand_init
	source ./include.tcl

	env_cleanup $testdir
	berkdb srand $rand_init

	# Use one environment for all database opens so we don't
	# need oodles of regions.
	set env [berkdb_env -create -home $testdir]
	error_check_good env_open [is_valid_env $env] TRUE

	# With the new offpage duplicate code, we don't support
	# duplicate duplicates in sorted dup sets.  Thus, if with_dup_dups
	# is greater than one, run only with "-dup".
	if { $with_dup_dups > 1 } {
		set doptarray {"-dup"}
	} else {
		set doptarray {"-dup -dupsort" "-dup" RANDOMMIX RANDOMMIX }
	}

	# NB: these flags are internal only, ok
	foreach m "DB_BTREE DB_HASH DB_BOTH" {
		# run with two different random mixes.
		foreach dopt $doptarray {
			set opt [list "-env" $env $dopt]

			puts "Join test: ($m $dopt) psize $psize,\
			    $with_dup_dups dup\
			    dups, flags $flags."

			build_all $m $psize $opt oa $with_dup_dups

			# null.db is db_built fifth but is referenced by
			# zero;  set up the option array appropriately.
			set oa(0) $oa(5)

			# Build the primary
			puts "\tBuilding the primary database $m"
			set oflags "-create -truncate -mode 0644 -env $env\
			    [conv $m [berkdb random_int 1 2]]"
			set db [eval {berkdb_open} $oflags primary.db]
			error_check_good dbopen [is_valid_db $db] TRUE
			for { set i 0 } { $i < 1000 } { incr i } {
				set key [format "%04d" $i]
				set ret [$db put $key stub]
				error_check_good "primary put" $ret 0
			}
			error_check_good "primary close" [$db close] 0
			set did [open $dict]
			gets $did str
			do_join primary.db "1 0" $str oa $flags\
			    $with_dup_dups
			gets $did str
			do_join primary.db "2 0" $str oa $flags\
			    $with_dup_dups
			gets $did str
			do_join primary.db "3 0" $str oa $flags\
			    $with_dup_dups
			gets $did str
			do_join primary.db "4 0" $str oa $flags\
			    $with_dup_dups
			gets $did str
			do_join primary.db "1" $str oa $flags $with_dup_dups
			gets $did str
			do_join primary.db "2" $str oa $flags $with_dup_dups
			gets $did str
			do_join primary.db "3" $str oa $flags $with_dup_dups
			gets $did str
			do_join primary.db "4" $str oa $flags $with_dup_dups
			gets $did str
			do_join primary.db "1 2" $str oa $flags\
			    $with_dup_dups
			gets $did str
			do_join primary.db "1 2 3" $str oa $flags\
			    $with_dup_dups
			gets $did str
			do_join primary.db "1 2 3 4" $str oa $flags\
			    $with_dup_dups
			gets $did str
			do_join primary.db "2 1" $str oa $flags\
			    $with_dup_dups
			gets $did str
			do_join primary.db "3 2 1" $str oa $flags\
			    $with_dup_dups
			gets $did str
			do_join primary.db "4 3 2 1" $str oa $flags\
			    $with_dup_dups
			gets $did str
			do_join primary.db "1 3" $str oa $flags $with_dup_dups
			gets $did str
			do_join primary.db "3 1" $str oa $flags $with_dup_dups
			gets $did str
			do_join primary.db "1 4" $str oa $flags $with_dup_dups
			gets $did str
			do_join primary.db "4 1" $str oa $flags $with_dup_dups
			gets $did str
			do_join primary.db "2 3" $str oa $flags $with_dup_dups
			gets $did str
			do_join primary.db "3 2" $str oa $flags $with_dup_dups
			gets $did str
			do_join primary.db "2 4" $str oa $flags $with_dup_dups
			gets $did str
			do_join primary.db "4 2" $str oa $flags $with_dup_dups
			gets $did str
			do_join primary.db "3 4" $str oa $flags $with_dup_dups
			gets $did str
			do_join primary.db "4 3" $str oa $flags $with_dup_dups
			gets $did str
			do_join primary.db "2 3 4" $str oa $flags\
			    $with_dup_dups
			gets $did str
			do_join primary.db "3 4 1" $str oa $flags\
			    $with_dup_dups
			gets $did str
			do_join primary.db "4 2 1" $str oa $flags\
			    $with_dup_dups
			gets $did str
			do_join primary.db "0 2 1" $str oa $flags\
			    $with_dup_dups
			gets $did str
			do_join primary.db "3 2 0" $str oa $flags\
			    $with_dup_dups
			gets $did str
			do_join primary.db "4 3 2 1" $str oa $flags\
			    $with_dup_dups
			gets $did str
			do_join primary.db "4 3 0 1" $str oa $flags\
			    $with_dup_dups
			gets $did str
			do_join primary.db "3 3 3" $str oa $flags\
			    $with_dup_dups
			gets $did str
			do_join primary.db "2 2 3 3" $str oa $flags\
			    $with_dup_dups
			gets $did str2
			gets $did str
			do_join primary.db "1 2" $str oa $flags\
			    $with_dup_dups "3" $str2

			# You really don't want to run this section
			# with $with_dup_dups > 2.
			if { $with_dup_dups <= 2 } {
				gets $did str2
				gets $did str
				do_join primary.db "1 2 3" $str\
				    oa $flags $with_dup_dups "3 3 1" $str2
				gets $did str2
				gets $did str
				do_join primary.db "4 0 2" $str\
				    oa $flags $with_dup_dups "4 3 3" $str2
				gets $did str2
				gets $did str
				do_join primary.db "3 2 1" $str\
				    oa $flags $with_dup_dups "0 2" $str2
				gets $did str2
				gets $did str
				do_join primary.db "2 2 3 3" $str\
				    oa $flags $with_dup_dups "1 4 4" $str2
				gets $did str2
				gets $did str
				do_join primary.db "2 2 3 3" $str\
				    oa $flags $with_dup_dups "0 0 4 4" $str2
				gets $did str2
				gets $did str
				do_join primary.db "2 2 3 3" $str2\
				    oa $flags $with_dup_dups "2 4 4" $str
				gets $did str2
				gets $did str
				do_join primary.db "2 2 3 3" $str2\
				    oa $flags $with_dup_dups "0 0 4 4" $str
			}
			close $did
		}
	}

	error_check_good env_close [$env close] 0
}

proc build_all { method psize opt oaname with_dup_dups {nentries 100} } {
	global testdir
	db_build join1.db $nentries 50 1 [conv $method 1]\
	    $psize $opt $oaname $with_dup_dups
	db_build join2.db $nentries 25 2 [conv $method 2]\
	    $psize $opt $oaname $with_dup_dups
	db_build join3.db $nentries 16 3 [conv $method 3]\
	    $psize $opt $oaname $with_dup_dups
	db_build join4.db $nentries 12 4 [conv $method 4]\
	    $psize $opt $oaname $with_dup_dups
	db_build null.db $nentries 0 5 [conv $method 5]\
	    $psize $opt $oaname $with_dup_dups
}

proc conv { m i } {
	switch -- $m {
		DB_HASH { return "-hash"}
		"-hash" { return "-hash"}
		DB_BTREE { return "-btree"}
		"-btree" { return "-btree"}
		DB_BOTH {
			if { [expr $i % 2] == 0 } {
				return "-hash";
			} else {
				return "-btree";
			}
		}
	}
}

proc random_opts { } {
	set j [berkdb random_int 0 1]
	if { $j == 0 } {
		return " -dup"
	} else {
		return " -dup -dupsort"
	}
}

proc db_build { name nkeys ndups dup_interval method psize lopt oaname \
    with_dup_dups } {
	source ./include.tcl

	# Get array of arg names (from two levels up the call stack)
	upvar 2 $oaname oa

	# Search for "RANDOMMIX" in $opt, and if present, replace
	# with " -dup" or " -dup -dupsort" at random.
	set i [lsearch $lopt RANDOMMIX]
	if { $i != -1 } {
		set lopt [lreplace $lopt $i $i [random_opts]]
	}

	# Save off db_open arguments for this database.
	set opt [eval concat $lopt]
	set oa($dup_interval) $opt

	# Create the database and open the dictionary
	set oflags "-create -truncate -mode 0644 $method\
	    -pagesize $psize"
	set db [eval {berkdb_open} $oflags $opt $name]
	error_check_good dbopen [is_valid_db $db] TRUE
	set did [open $dict]
	set count 0
	puts -nonewline "\tBuilding $name: $nkeys keys "
	puts -nonewline "with $ndups duplicates at interval of $dup_interval"
	if { $with_dup_dups > 0 } {
		puts ""
		puts "\t\tand $with_dup_dups duplicate duplicates."
	} else {
		puts "."
	}
	for { set count 0 } { [gets $did str] != -1 && $count < $nkeys } {
	    incr count} {
		set str $str$name
		# We need to make sure that the dups are inserted in a
		# random, or near random, order.  Do this by generating
		# them and putting each in a list, then sorting the list
		# at random.
		set duplist {}
		for { set i 0 } { $i < $ndups } { incr i } {
			set data [format "%04d" [expr $i * $dup_interval]]
			lappend duplist $data
		}
		# randomize the list
		for { set i 0 } { $i < $ndups } {incr i } {
		#	set j [berkdb random_int $i [expr $ndups - 1]]
			set j [expr ($i % 2) + $i]
			if { $j >= $ndups } { set j $i }
			set dupi [lindex $duplist $i]
			set dupj [lindex $duplist $j]
			set duplist [lreplace $duplist $i $i $dupj]
			set duplist [lreplace $duplist $j $j $dupi]
		}
		foreach data $duplist {
			if { $with_dup_dups != 0 } {
				for { set j 0 }\
				    { $j < $with_dup_dups }\
				    {incr j} {
					set ret [$db put $str $data]
					error_check_good put$j $ret 0
				}
			} else {
				set ret [$db put $str $data]
				error_check_good put $ret 0
			}
		}

		if { $ndups == 0 } {
			set ret [$db put $str NODUP]
			error_check_good put $ret 0
		}
	}
	close $did
	error_check_good close:$name [$db close] 0
}

proc do_join { primary dbs key oanm flags with_dup_dups {dbs2 ""} {key2 ""} } {
	global testdir
	source ./include.tcl

	upvar $oanm oa

	puts -nonewline "\tJoining: $dbs on $key"
	if { $dbs2 == "" } {
	    puts ""
	} else {
	    puts " with $dbs2 on $key2"
	}

	# Open all the databases
	set p [berkdb_open -unknown $testdir/$primary]
	error_check_good "primary open" [is_valid_db $p] TRUE

	set dblist ""
	set curslist ""

	set ndx [llength $dbs]

	foreach i [concat $dbs $dbs2] {
		set opt $oa($i)
		set db [eval {berkdb_open -unknown} $opt [n_to_name $i]]
		error_check_good "[n_to_name $i] open" [is_valid_db $db] TRUE
		set curs [$db cursor]
		error_check_good "$db cursor" \
		    [is_substr $curs "$db.c"] 1
		lappend dblist $db
		lappend curslist $curs

		if { $ndx > 0 } {
		    set realkey [concat $key[n_to_name $i]]
		} else {
		    set realkey [concat $key2[n_to_name $i]]
		}

		set pair [$curs get -set $realkey]
		error_check_good cursor_set:$realkey:$pair \
			[llength [lindex $pair 0]] 2

		incr ndx -1
	}

	set join_curs [eval {$p join} $curslist]
	error_check_good join_cursor \
	    [is_substr $join_curs "$p.c"] 1

	# Calculate how many dups we expect.
	# We go through the list of indices.  If we find a 0, then we
	# expect 0 dups.  For everything else, we look at pairs of numbers,
	# if the are relatively prime, multiply them and figure out how
	# many times that goes into 50.  If they aren't relatively prime,
	# take the number of times the larger goes into 50.
	set expected 50
	set last 1
	foreach n [concat $dbs $dbs2] {
		if { $n == 0 } {
			set expected 0
			break
		}
		if { $last == $n } {
			continue
		}

		if { [expr $last % $n] == 0 || [expr $n % $last] == 0 } {
			if { $n > $last } {
				set last $n
				set expected [expr 50 / $last]
			}
		} else {
			set last [expr $n * $last / [gcd $n $last]]
			set expected [expr 50 / $last]
		}
	}

	# If $with_dup_dups is greater than zero, each datum has
	# been inserted $with_dup_dups times.  So we expect the number
	# of dups to go up by a factor of ($with_dup_dups)^(number of databases)

	if { $with_dup_dups > 0 } {
		foreach n [concat $dbs $dbs2] {
			set expected [expr $expected * $with_dup_dups]
		}
	}

	set ndups 0
	if { $flags == " -join_item"} {
		set l 1
	} else {
		set flags ""
		set l 2
	}
	for { set pair [eval {$join_curs get} $flags] } { \
		[llength [lindex $pair 0]] == $l } {
	    set pair [eval {$join_curs get} $flags] } {
		set k [lindex [lindex $pair 0] 0]
		foreach i $dbs {
			error_check_bad valid_dup:$i:$dbs $i 0
			set kval [string trimleft $k 0]
			if { [string length $kval] == 0 } {
				set kval 0
			}
			error_check_good valid_dup:$i:$dbs [expr $kval % $i] 0
		}
		incr ndups
	}
	error_check_good number_of_dups:$dbs $ndups $expected

	error_check_good close_primary [$p close] 0
	foreach i $curslist {
		error_check_good close_cursor:$i [$i close] 0
	}
	foreach i $dblist {
		error_check_good close_index:$i [$i close] 0
	}
}

proc n_to_name { n } {
global testdir
	if { $n == 0 } {
		return null.db;
	} else {
		return join$n.db;
	}
}

proc gcd { a b } {
	set g 1

	for { set i 2 } { $i <= $a } { incr i } {
		if { [expr $a % $i] == 0 && [expr $b % $i] == 0 } {
			set g $i
		}
	}
	return $g
}
