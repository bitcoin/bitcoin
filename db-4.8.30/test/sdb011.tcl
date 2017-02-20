# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	sdb011
# TEST	Test deleting Subdbs with overflow pages
# TEST	Create 1 db with many large subdbs.
# TEST	Test subdatabases with overflow pages.
proc sdb011 { method {ndups 13} {nsubdbs 10} args} {
	global names
	source ./include.tcl
	global rand_init
	error_check_good set_random_seed [berkdb srand $rand_init] 0

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	if { [is_queue $method] == 1 || [is_fixed_length $method] == 1 } {
		puts "Subdb011: skipping for method $method"
		return
	}
	set txnenv 0
	set envargs ""
	set max_files 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/subdb011.db
		set env NULL
		set tfpath $testfile
	} else {
		set testfile subdb011.db
		incr eindex
		set env [lindex $args $eindex]
		set envargs " -env $env "
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
			append envargs " -auto_commit "
			set max_files 50
			if { $ndups == 13 } {
				set ndups 7
			}
		}
		set testdir [get_home $env]
		set tfpath $testdir/$testfile
	}

	# Create the database and open the dictionary

	cleanup $testdir $env
	set txn ""

	# Here is the loop where we put and get each key/data pair
	set file_list [get_file_list]
	set flen [llength $file_list]
	puts "Subdb011: $method ($args) $ndups overflow dups with \
	    $flen filename=key filecontents=data pairs"

	puts "\tSubdb011.a: Create each of $nsubdbs subdbs and dups"
	set slist {}
	set i 0
	set count 0
	foreach f $file_list {
		set i [expr $i % $nsubdbs]
		if { [is_record_based $method] == 1 } {
			set key [expr $count + 1]
			set names([expr $count + 1]) $f
		} else {
			set key $f
		}
		# Should really catch errors
		set fid [open $f r]
		fconfigure $fid -translation binary
		set filecont [read $fid]
		set subdb subdb$i
		lappend slist $subdb
		close $fid
		set db [eval {berkdb_open -create -mode 0644} \
		    $args {$omethod $testfile $subdb}]
		error_check_good dbopen [is_valid_db $db] TRUE
		for {set dup 0} {$dup < $ndups} {incr dup} {
			set data $dup:$filecont
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			set ret [eval {$db put} $txn {$key \
			    [chop_data $method $data]}]
			error_check_good put $ret 0
			if { $txnenv == 1 } {
				error_check_good txn [$t commit] 0
			}
		}
		error_check_good dbclose [$db close] 0
		incr i
		incr count
	}

	puts "\tSubdb011.b: Verify overflow pages"
	foreach subdb $slist {
		set db [eval {berkdb_open -create -mode 0644} \
		    $args {$omethod $testfile $subdb}]
		error_check_good dbopen [is_valid_db $db] TRUE
		set stat [$db stat]

		# What everyone else calls overflow pages, hash calls "big
		# pages", so we need to special-case hash here.  (Hash
		# overflow pages are additional pages after the first in a
		# bucket.)
		if { [string compare [$db get_type] hash] == 0 } {
			error_check_bad overflow \
			    [is_substr $stat "{{Number of big pages} 0}"] 1
		} else {
			error_check_bad overflow \
			    [is_substr $stat "{{Overflow pages} 0}"] 1
		}
		error_check_good dbclose [$db close] 0
	}

	puts "\tSubdb011.c: Delete subdatabases"
	for {set i $nsubdbs} {$i > 0} {set i [expr $i - 1]} {
		#
		# Randomly delete a subdatabase
		set sindex [berkdb random_int 0 [expr $i - 1]]
		set subdb [lindex $slist $sindex]
		#
		# Delete the one we did from the list
		set slist [lreplace $slist $sindex $sindex]
		error_check_good file_exists_before [file exists $tfpath] 1
		error_check_good db_remove [eval {berkdb dbremove} $envargs \
		    {$testfile $subdb}] 0
	}
}

