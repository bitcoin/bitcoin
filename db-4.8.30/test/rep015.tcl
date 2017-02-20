# See the file LICENSE for redistribution information.
#
# Copyright (c) 2003-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep015
# TEST	Locking across multiple pages with replication.
# TEST
# TEST	Open master and client with small pagesize and
# TEST	generate more than one page and generate off-page
# TEST	dups on the first page (second key) and last page
# TEST	(next-to-last key).
# TEST	Within a single transaction, for each database, open
# TEST	2 cursors and delete the first and last entries (this
# TEST	exercises locks on regular pages).  Intermittently
# TEST	update client during the process.
# TEST	Within a single transaction, for each database, open
# TEST	2 cursors.  Walk to the off-page dups and delete one
# TEST	from each end (this exercises locks on off-page dups).
# TEST	Intermittently update client.
#
proc rep015 { method { nentries 100 } { tnum "015" } { ndb 3 } args } {
	global repfiles_in_memory
	global rand_init
	berkdb srand $rand_init

	source ./include.tcl
	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}

	# Run for btree only.
	if { $checking_valid_methods } {
		set test_methods { btree }
		return $test_methods
	}
	if { [is_btree $method] == 0 } {
		puts "Skipping rep$tnum for method $method."
		return
	}

	set args [convert_args $method $args]
	set logsets [create_logsets 2]

	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	# Run the body of the test with and without recovery.
	foreach r $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Rep$tnum: \
				    Skipping for in-memory logs with -recover."
				continue
			}
			puts "Rep$tnum ($method $r):\
			    Replication and locking $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep015_sub $method $nentries $tnum $ndb $l $r $args
		}
	}
}

proc rep015_sub { method nentries tnum ndb logset recargs largs } {
	global testdir
	global repfiles_in_memory
	global rep_verbose
	global verbose_type

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	set repmemargs ""
	if { $repfiles_in_memory } {
		set repmemargs "-rep_inmem_files "
	}

	env_cleanup $testdir
	set omethod [convert_method $method]

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR

	file mkdir $masterdir
	file mkdir $clientdir

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]

	# In-memory logs require a large log buffer, and cannot
	# be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs $m_logargs \
	    $verbargs -errpfx MASTER $repmemargs \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs -rep_master]

	# Open a client
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs $c_logargs \
	    $verbargs -errpfx CLIENT $repmemargs \
	    -home $clientdir -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd $recargs -rep_client]

	# Bring the clients online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	# Set up the master databases.  The small pagesize quickly
	# generates multiple pages and off-page dups.
	set pagesize 512
	puts "\tRep$tnum.a: Create and populate databases in master."
	for { set i 0 } { $i < $ndb } { incr i } {
		set db [eval berkdb_open_noerr -create $omethod -auto_commit \
		    -pagesize $pagesize -env $masterenv $largs -dup testdb$i.db]
		set dblist($i) $db
		#
		# Populate, being sure to create multiple pages.
		# The non-duplicate entries are pairs of the form
		# {1, data1} {2, data2}.  The duplicates are pairs of
		# the form {2, dup1} {2, dup2}, {2, dup3}, etc.
		#
		for { set j 1 } { $j <= $nentries } { incr j } {
			set t [$masterenv txn]
			error_check_good put_$db [eval $db put -txn $t \
			    $j [chop_data $method data$j]] 0
			error_check_good txn_commit [$t commit] 0
		}
		# Create off-page dups on key 2 and next-to-last key.
		set t [$masterenv txn]
		for { set j 1 } { $j <= $nentries } { incr j } {
			error_check_good put_second [eval $db put -txn $t \
			    2 [chop_data $method dup$j]] 0
			error_check_good put_next_to_last [eval $db put \
			    -txn $t \
			    [expr $nentries - 1] [chop_data $method dup$j]] 0
		}
		error_check_good txn_commit [$t commit] 0
		# Make sure there are off-page dups.
		set stat [$db stat]
		error_check_bad stat:offpage \
		    [is_substr $stat "{{Internal pages} 0}"] 1
	}

	puts "\tRep$tnum.b: Propagate setup to clients."
	process_msgs $envlist

	# Open client databases so we can exercise locking there too.
	for { set i 0 } { $i < $ndb } { incr i } {
		set cdb [eval {berkdb_open_noerr} -auto_commit \
		    -env $clientenv $largs testdb$i.db]
		set cdblist($i) $cdb
	}

	# Set up two cursors into each db.  Randomly select a cursor
	# and do the next thing:  position, delete, or close.
	foreach option { regular off-page } {
		puts "\tRep$tnum.c: Transactional cursor deletes ($option)."

		set t [$masterenv txn]
		# Set up two cursors into each db, and initialize the next
		# action to be done to POSITION.
		for { set i 0 } { $i < [expr $ndb * 2] } { incr i }  {
			set db $dblist([expr $i / 2])
			set mcurs($i) [eval {$db cursor} -txn $t]
			error_check_good mcurs$i \
			    [is_valid_cursor $mcurs($i) $db] TRUE
			set cnext($i) POSITION
		}

		set ct [$clientenv txn]
		# Set up two cursors into each client db.
		for { set i 0 } { $i < [expr $ndb * 2] } { incr i }  {
			set cdb $cdblist([expr $i / 2])
			set ccurs($i) [eval {$cdb cursor} -txn $ct]
			error_check_good ccurs$i \
			    [is_valid_cursor $ccurs($i) $cdb] TRUE
		}

		# Randomly pick a cursor to operate on and do the next thing.
		# At POSITION, we position that cursor.  At DELETE, we delete
		# the current item.  At CLOSE, we close the cursor.  At DONE,
		# we do nothing except check to see if all cursors have reached
		# DONE, and quit when they have.
		# On the off-page dup test, walk to reach an off-page entry,
		# and delete that one.
		set k 0
		while { 1 } {
			# Every nth time through, update the client.
#			set n 5
#			if {[expr $k % $n] == 0 } {
#				puts "Updating clients"
#				process_msgs $envlist
#			}
#			incr k
			set i [berkdb random_int 0 [expr [expr $ndb * 2] - 1]]
			set next $cnext($i)
			switch -exact -- $next {
				POSITION {
					do_position $mcurs($i) \
					    $i $nentries $option
					set cnext($i) DELETE
					# Position the client cursors too.
					do_position $ccurs($i) \
					    $i $nentries $option
				}
				DELETE {
					error_check_good c_del \
					    [$mcurs($i) del] 0
					set cnext($i) CLOSE
					# Update clients after a delete.
					process_msgs $envlist
				}
				CLOSE {
					error_check_good c_close.$i \
					    [$mcurs($i) close] 0
					set cnext($i) DONE
					# Close the client cursor too.
					error_check_good cc_close.$i \
					    [$ccurs($i) close] 0
				}
				DONE {
					set breakflag 1
					for { set j 0 } \
					    { $j < [expr $ndb * 2] } \
					    { incr j } {
						if { $cnext($j) != "DONE" } {
							set breakflag 0
						}
					}
					if { $breakflag == 1 } {
						break
					}
				}
				default {
					puts "FAIL: Unrecognized \
					    next action $next"
				}
			}
		}
		error_check_good txn_commit [$t commit] 0
		error_check_good clienttxn_commit [$ct commit] 0
		process_msgs $envlist
	}

	# Clean up.
	for { set i 0 } { $i < $ndb } { incr i } {
		set db $dblist($i)
		error_check_good close_$db [$db close] 0
		set cdb $cdblist($i)
		error_check_good close_$cdb [$cdb close] 0
	}

	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	replclose $testdir/MSGQUEUEDIR
	return
}

proc do_position { cursor i nentries option } {
	if { [expr $i % 2] == 0 } {
		if { $option == "regular" } {
			set ret [$cursor get -first]
			set key [lindex [lindex $ret 0] 0]
			set data [lindex [lindex $ret 0] 1]
			error_check_good get_first \
			    [string range $data 4 end] $key
		} elseif { $option == "off-page" } {
			set ret [$cursor get -set 2]
			error_check_good get_key_2 \
			    [lindex [lindex $ret 0] 0] 2
			error_check_good get_data_2 \
			    [lindex [lindex $ret 0] 1] data2
			for { set j 1 } { $j <= 95 } { incr j } {
				set ret [$cursor get -nextdup]
				error_check_good key_nextdup$j \
				    [lindex [lindex $ret 0] 0] 2
				error_check_good data_nextdup$j \
				    [lindex [lindex $ret 0] 1] dup$j
			}
		}
	} else {
		if { $option == "regular" } {
			set ret [$cursor get -set $nentries]
			set key [lindex [lindex $ret 0] 0]
			set data [lindex [lindex $ret 0] 1]
			error_check_good get_set_$nentries \
			    [string range $data 4 end] $key
		} elseif { $option == "off-page" } {
			set ret [$cursor get -last]
			set key [lindex [lindex $ret 0] 0]
			set data [lindex [lindex $ret 0] 1]
			error_check_good get_last \
			    [string range $data 3 end] [expr $key + 1]
			for { set j 1 } { $j <= 5 } { incr j } {
				set ret [$cursor get -prev]
				set key [lindex [lindex $ret 0] 0]
				set data [lindex [lindex $ret 0] 1]
				error_check_good get_prev \
				    [string range $data 3 end] \
				    [expr [expr $key + 1] - $j]
			}
		}
	}
}
