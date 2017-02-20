# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rpc001
# TEST	Test RPC server timeouts for cursor, txn and env handles.
proc rpc001 { } {
	global __debug_on
	global __debug_print
	global errorInfo
	global is_je_test
	global rpc_svc
	source ./include.tcl

	#
	# First test timeouts on server.
	#
	set ttime 5
	set itime 10
	puts "Rpc001: Server timeouts: resource $ttime sec, idle $itime sec"
	puts "Rpc001: Using $rpc_svc"
	set dpid [rpc_server_start 0 30 -t $ttime -I $itime]
	puts "\tRpc001.a: Started server, pid $dpid"

	#
	# Wrap the whole test in a catch statement so we can still kill
	# the rpc server even if the test fails.
	#
	set status [catch {
		tclsleep 2
		remote_cleanup $rpc_server $rpc_testdir $testdir

		puts "\tRpc001.b: Creating environment"

		set testfile "rpc001.db"
		set home [file tail $rpc_testdir]

		set env [eval {berkdb_env -create -mode 0644 -home $home \
		    -server $rpc_server -client_timeout 10000 -txn}]
		error_check_good lock_env:open [is_valid_env $env] TRUE

		puts "\tRpc001.c: Opening a database"
		#
		# NOTE: the type of database doesn't matter, just use btree.
		set db [eval {berkdb_open -auto_commit -create -btree \
		    -mode 0644} -env $env $testfile]
		error_check_good dbopen [is_valid_db $db] TRUE

		set curs_list {}
		set txn_list {}
		puts "\tRpc001.d: Basic timeout test"
		puts "\tRpc001.d1: Starting a transaction"
		set txn [$env txn]
		error_check_good txn_begin [is_valid_txn $txn $env] TRUE
		lappend txn_list $txn

		puts "\tRpc001.d2: Open a cursor in that transaction"
		set dbc [$db cursor -txn $txn]
		error_check_good db_cursor [is_valid_cursor $dbc $db] TRUE
		lappend curs_list $dbc

		puts "\tRpc001.d3: Duplicate that cursor"
		set dbc [$dbc dup]
		error_check_good db_cursor [is_valid_cursor $dbc $db] TRUE
		lappend curs_list $dbc

		if { !$is_je_test } {
			puts "\tRpc001.d4: Starting a nested transaction"
			set txn [$env txn -parent $txn]
			error_check_good txn_begin [is_valid_txn $txn $env] TRUE
			set txn_list [linsert $txn_list 0 $txn]
		}

		puts "\tRpc001.d5: Create a cursor, no transaction"
		set dbc [$db cursor]
		error_check_good db_cursor [is_valid_cursor $dbc $db] TRUE
		lappend curs_list $dbc

		puts "\tRpc001.d6: Timeout cursor and transactions"
		set sleeptime [expr $ttime + 2]
		tclsleep $sleeptime

		#
		# Perform a generic db operations to cause the timeout routine
		# to trigger.
		#
		set stat [catch {$db stat} ret]
		error_check_good dbstat $stat 0

		#
		# Check that every handle we opened above is timed out
		#
		foreach c $curs_list {
			set stat [catch {$c close} ret]
			error_check_good dbc_close:$c $stat 1
			error_check_good dbc_timeout:$c \
			    [is_substr $errorInfo "DB_NOSERVER_ID"] 1
		}
		foreach t $txn_list {
			set stat [catch {$t commit} ret]
			error_check_good txn_commit:$t $stat 1
			error_check_good txn_timeout:$t \
			    [is_substr $errorInfo "DB_NOSERVER_ID"] 1
		}

		set txn_list {}

		if { !$is_je_test } {
			set ntxns 8
			puts "\tRpc001.e: Nested ($ntxns x $ntxns) txn activity test"
			puts "\tRpc001.e1: Starting parent transaction"
			set txn [$env txn]
			error_check_good txn_begin [is_valid_txn $txn $env] TRUE
			set txn_list [linsert $txn_list 0 $txn]
			set last_txn $txn
			set parent_txn $txn

			#
			# First set a breadth of 'ntxns'
			# We need 2 from this set for testing later on.  Just
			# set them up separately first.
			#
			puts "\tRpc001.e2: Creating $ntxns child transactions"
			set child0 [$env txn -parent $parent_txn]
			error_check_good txn_begin \
				[is_valid_txn $child0 $env] TRUE
			set child1 [$env txn -parent $parent_txn]
			error_check_good txn_begin \
				[is_valid_txn $child1 $env] TRUE

			for {set i 2} {$i < $ntxns} {incr i} {
				set txn [$env txn -parent $parent_txn]
				error_check_good txn_begin \
					[is_valid_txn $txn $env] TRUE
				set txn_list [linsert $txn_list 0 $txn]
			}

			#
			# Now make one 'ntxns' deeply nested.
			# Add one more for testing later on separately.
			#
			puts "\tRpc001.e3: Creating $ntxns nested child transactions"
			for {set i 0} {$i < $ntxns} {incr i} {
				set txn [$env txn -parent $last_txn]
				error_check_good txn_begin \
					[is_valid_txn $txn $env] TRUE
				set txn_list [linsert $txn_list 0 $txn]
				set last_txn $txn
			}
			set last_parent $last_txn
			set last_txn [$env txn -parent $last_parent]
			error_check_good txn_begin \
				[is_valid_txn $last_txn $env] TRUE

			puts "\tRpc001.e4: Open a cursor in deepest transaction"
			set dbc [$db cursor -txn $last_txn]
			error_check_good db_cursor \
				[is_valid_cursor $dbc $db] TRUE

			puts "\tRpc001.e5: Duplicate that cursor"
			set dbcdup [$dbc dup]
			error_check_good db_cursor \
				[is_valid_cursor $dbcdup $db] TRUE
			lappend curs_list $dbcdup

			puts "\tRpc001.f: Timeout then activate duplicate cursor"
			tclsleep $sleeptime
			set stat [catch {$dbcdup close} ret]
			error_check_good dup_close:$dbcdup $stat 0
			error_check_good dup_close:$dbcdup $ret 0

			#
			# Make sure that our parent txn is not timed out.  We
			# will try to begin another child tnx using the parent.
			# We expect that to succeed.  Immediately commit that
			# txn.
			#
			set stat [catch {$env txn -parent $parent_txn} newchild]
			error_check_good newchildtxn $stat 0
			error_check_good newcommit [$newchild commit] 0

			puts "\tRpc001.g: Timeout, then activate cursor"
			tclsleep $sleeptime
			set stat [catch {$dbc close} ret]
			error_check_good dbc_close:$dbc $stat 0
			error_check_good dbc_close:$dbc $ret 0

			#
			# Make sure that our parent txn is not timed out.  We
			# will try to begin another child tnx using the parent.
			# We expect that to succeed.  Immediately commit that
			# txn.
			#
			set stat [catch {$env txn -parent $parent_txn} newchild]
			error_check_good newchildtxn $stat 0
			error_check_good newcommit [$newchild commit] 0

			puts "\tRpc001.h: Timeout, then activate child txn"
			tclsleep $sleeptime
			set stat [catch {$child0 commit} ret]
			error_check_good child_commit $stat 0
			error_check_good child_commit:$child0 $ret 0

			#
			# Make sure that our nested txn is not timed out.  We
			# will try to begin another child tnx using the parent.
			# We expect that to succeed.  Immediately commit that
			# txn.
			#
			set stat \
				[catch {$env txn -parent $last_parent} newchild]
			error_check_good newchildtxn $stat 0
			error_check_good newcommit [$newchild commit] 0

			puts "\tRpc001.i: Timeout, then activate nested txn"
			tclsleep $sleeptime
			set stat [catch {$last_txn commit} ret]
			error_check_good lasttxn_commit $stat 0
			error_check_good lasttxn_commit:$child0 $ret 0

			#
			# Make sure that our child txn is not timed out.  We
			# should be able to commit it.
			#
			set stat [catch {$child1 commit} ret]
			error_check_good child_commit:$child1 $stat 0
			error_check_good child_commit:$child1 $ret 0

			#
			# Clean up.  They were inserted in LIFO order, so we
			# should just be able to commit them all.
			#
			foreach t $txn_list {
				set stat [catch {$t commit} ret]
				error_check_good txn_commit:$t $stat 0
				error_check_good txn_commit:$t $ret 0
			}
		}

		set stat [catch {$db close} ret]
		error_check_good db_close $stat 0

		rpc_timeoutjoin $env "Rpc001.j" $sleeptime 0
		rpc_timeoutjoin $env "Rpc001.k" $sleeptime 1

		puts "\tRpc001.l: Timeout idle env handle"
		set sleeptime [expr $itime + 2]
		tclsleep $sleeptime

		#
		# We need to do another operation to time out the environment
		# handle.  Open another environment, with an invalid home
		# directory.
		#
		set stat [catch {eval {berkdb_env_noerr -home "$home.fail" \
		    -server $rpc_server}} ret]
		error_check_good env_open $stat 1

		set stat [catch {$env close} ret]
		error_check_good env_close $stat 1
		error_check_good env_timeout \
		    [is_substr $errorInfo "DB_NOSERVER_ID"] 1
	} res]
	if { $status != 0 } {
		puts $res
	}
	tclkill $dpid
}

proc rpc_timeoutjoin {env msg sleeptime use_txn} {
	#
	# Check join cursors now.
	#
	puts -nonewline "\t$msg: Test join cursors and timeouts"
	if { $use_txn } {
		puts " (using txns)"
		set txnflag "-auto_commit"
	} else {
		puts " (without txns)"
		set txnflag ""
	}
	#
	# Set up a simple set of join databases
	#
	puts "\t${msg}0: Set up join databases"
	set fruit {
	    {blue blueberry}
	    {red apple} {red cherry} {red raspberry}
	    {yellow lemon} {yellow pear}
	}
	set price {
	    {expen blueberry} {expen cherry} {expen raspberry}
	    {inexp apple} {inexp lemon} {inexp pear}
	}
	set dessert {
	    {blueberry cobbler} {cherry cobbler} {pear cobbler}
	    {apple pie} {raspberry pie} {lemon pie}
	}
	set fdb [eval {berkdb_open -create -btree -mode 0644} \
	    $txnflag -env $env -dup -dupsort fruit.db]
	error_check_good dbopen [is_valid_db $fdb] TRUE
	set pdb [eval {berkdb_open -create -btree -mode 0644} \
	    $txnflag -env $env -dup -dupsort price.db]
	error_check_good dbopen [is_valid_db $pdb] TRUE
	set ddb [eval {berkdb_open -create -btree -mode 0644} \
	    $txnflag -env $env -dup -dupsort dessert.db]
	error_check_good dbopen [is_valid_db $ddb] TRUE
	foreach kd $fruit {
		set k [lindex $kd 0]
		set d [lindex $kd 1]
		set ret [eval {$fdb put} {$k $d}]
		error_check_good fruit_put $ret 0
	}
	error_check_good sync [$fdb sync] 0
	foreach kd $price {
		set k [lindex $kd 0]
		set d [lindex $kd 1]
		set ret [eval {$pdb put} {$k $d}]
		error_check_good price_put $ret 0
	}
	error_check_good sync [$pdb sync] 0
	foreach kd $dessert {
		set k [lindex $kd 0]
		set d [lindex $kd 1]
		set ret [eval {$ddb put} {$k $d}]
		error_check_good dessert_put $ret 0
	}
	error_check_good sync [$ddb sync] 0

	rpc_join $env $msg $sleeptime $fdb $pdb $ddb $use_txn 0
	rpc_join $env $msg $sleeptime $fdb $pdb $ddb $use_txn 1

	error_check_good ddb:close [$ddb close] 0
	error_check_good pdb:close [$pdb close] 0
	error_check_good fdb:close [$fdb close] 0
	error_check_good ddb:remove [$env dbremove dessert.db] 0
	error_check_good pdb:remove [$env dbremove price.db] 0
	error_check_good fdb:remove [$env dbremove fruit.db] 0
}

proc rpc_join {env msg sleep fdb pdb ddb use_txn op} {
	global errorInfo
	global is_je_test

	#
	# Start a parent and child transaction.  We'll do our join in
	# the child transaction just to make sure everything gets timed
	# out correctly.
	#
	set curs_list {}
	set txn_list {}
	set msgnum [expr $op * 2 + 1]
	if { $use_txn } {
		puts "\t$msg$msgnum: Set up txns and join cursor"
		set txn [$env txn]
		error_check_good txn_begin [is_valid_txn $txn $env] TRUE
		set txn_list [linsert $txn_list 0 $txn]
		if { !$is_je_test } {
			set child0 [$env txn -parent $txn]
			error_check_good txn_begin \
				[is_valid_txn $child0 $env] TRUE
			set txn_list [linsert $txn_list 0 $child0]
			set child1 [$env txn -parent $txn]
			error_check_good txn_begin \
				[is_valid_txn $child1 $env] TRUE
			set txn_list [linsert $txn_list 0 $child1]
		} else {
			set child0 $txn
			set child1 $txn
		}
		set txncmd "-txn $child0"
	} else {
		puts "\t$msg$msgnum: Set up join cursor"
		set txncmd ""
	}

	#
	# Start a cursor, (using txn child0 in the fruit and price dbs, if
	# needed).  # Just pick something simple to join on.
	# Then call join on the dessert db.
	#
	set fkey yellow
	set pkey inexp
	set fdbc [eval $fdb cursor $txncmd]
	error_check_good fdb_cursor [is_valid_cursor $fdbc $fdb] TRUE
	set ret [$fdbc get -set $fkey]
	error_check_bad fget:set [llength $ret] 0
	set k [lindex [lindex $ret 0] 0]
	error_check_good fget:set:key $k $fkey
	set curs_list [linsert $curs_list 0 $fdbc]

	set pdbc [eval $pdb cursor $txncmd]
	error_check_good pdb_cursor [is_valid_cursor $pdbc $pdb] TRUE
	set ret [$pdbc get -set $pkey]
	error_check_bad pget:set [llength $ret] 0
	set k [lindex [lindex $ret 0] 0]
	error_check_good pget:set:key $k $pkey
	set curs_list [linsert $curs_list 0 $pdbc]

	set jdbc [$ddb join $fdbc $pdbc]
	error_check_good join_cursor [is_valid_cursor $jdbc $ddb] TRUE
	set ret [$jdbc get]
	error_check_bad jget [llength $ret] 0

	set msgnum [expr $op * 2 + 2]
	if { $op == 1 } {
		puts -nonewline "\t$msg$msgnum: Timeout all cursors"
		if { $use_txn } {
			puts " and txns"
		} else {
			puts ""
		}
	} else {
		puts "\t$msg$msgnum: Timeout, then activate join cursor"
	}

	tclsleep $sleep

	if { $op == 1 } {
		#
		# Perform a generic db operations to cause the timeout routine
		# to trigger.
		#
		set stat [catch {$fdb stat} ret]
		error_check_good fdbstat $stat 0

		#
		# Check that join cursor is timed out.
		#
		set stat [catch {$jdbc close} ret]
		error_check_good dbc_close:$jdbc $stat 1
		error_check_good dbc_timeout:$jdbc \
		    [is_substr $errorInfo "DB_NOSERVER_ID"] 1

		#
		# Now the server may or may not timeout constituent
		# cursors when it times out the join cursor.  So, just
		# sleep again and then they should timeout.
		#
		tclsleep $sleep
		set stat [catch {$fdb stat} ret]
		error_check_good fdbstat $stat 0

		foreach c $curs_list {
			set stat [catch {$c close} ret]
			error_check_good dbc_close:$c $stat 1
			error_check_good dbc_timeout:$c \
			    [is_substr $errorInfo "DB_NOSERVER_ID"] 1
		}

		foreach t $txn_list {
			set stat [catch {$t commit} ret]
			error_check_good txn_commit:$t $stat 1
			error_check_good txn_timeout:$t \
			    [is_substr $errorInfo "DB_NOSERVER_ID"] 1
		}
	} else {
		set stat [catch {$jdbc get} ret]
		error_check_good jget.stat $stat 0
		error_check_bad jget [llength $ret] 0
		set curs_list [linsert $curs_list 0 $jdbc]
		foreach c $curs_list {
			set stat [catch {$c close} ret]
			error_check_good dbc_close:$c $stat 0
			error_check_good dbc_close:$c $ret 0
		}

		foreach t $txn_list {
			set stat [catch {$t commit} ret]
			error_check_good txn_commit:$t $stat 0
			error_check_good txn_commit:$t $ret 0
		}
	}
}
