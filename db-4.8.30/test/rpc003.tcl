# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rpc003
# TEST	Test RPC and secondary indices.
proc rpc003 { } {
	source ./include.tcl
	global dict nsecondaries
	global rpc_svc

	#
	# First set up the files.  Secondary indices only work readonly
	# over RPC.  So we need to create the databases first without
	# RPC.  Then run checking over RPC.
	#
	puts "Rpc003: Secondary indices over RPC"
	puts "Rpc003: Using $rpc_svc"
	if { [string compare $rpc_server "localhost"] != 0 } {
		puts "Cannot run to non-local RPC server.  Skipping."
		return
	}
	cleanup $testdir NULL
	puts "\tRpc003.a: Creating local secondary index databases"

	# Primary method/args.
	set pmethod btree
	set pomethod [convert_method $pmethod]
	set pargs ""
	set methods {dbtree dbtree}
	set argses [convert_argses $methods ""]
	set omethods [convert_methods $methods]

	set nentries 500

	puts "\tRpc003.b: ($pmethod/$methods) $nentries equal key/data pairs"
	set pname "primary003.db"
	set snamebase "secondary003"

	# Open an environment
	# XXX if one is not supplied!
	set env [berkdb_env -create -home $testdir]
	error_check_good env_open [is_valid_env $env] TRUE

	# Open the primary.
	set pdb [eval {berkdb_open -create -env} $env $pomethod $pargs $pname]
	error_check_good primary_open [is_valid_db $pdb] TRUE

	# Open and associate the secondaries
	set sdbs {}
	for { set i 0 } { $i < [llength $omethods] } { incr i } {
		set sdb [eval {berkdb_open -create -env} $env \
		    [lindex $omethods $i] [lindex $argses $i] $snamebase.$i.db]
		error_check_good second_open($i) [is_valid_db $sdb] TRUE

		error_check_good db_associate($i) \
		    [$pdb associate [callback_n $i] $sdb] 0
		lappend sdbs $sdb
	}

	set did [open $dict]
	for { set n 0 } { [gets $did str] != -1 && $n < $nentries } { incr n } {
		if { [is_record_based $pmethod] == 1 } {
			set key [expr $n + 1]
			set datum $str
		} else {
			set key $str
			gets $did datum
		}
		set keys($n) $key
		set data($n) [pad_data $pmethod $datum]

		set ret [eval {$pdb put} {$key [chop_data $pmethod $datum]}]
		error_check_good put($n) $ret 0
	}
	close $did
	foreach sdb $sdbs {
		error_check_good secondary_close [$sdb close] 0
	}
	error_check_good primary_close [$pdb close] 0
	error_check_good env_close [$env close] 0

	#
	# We have set up our databases, so now start the server and
	# read them over RPC.
	#
	set dpid [rpc_server_start]
	puts "\tRpc003.c: Started server, pid $dpid"

	#
	# Wrap the remainder of the test in a catch statement so we
	# can still kill the rpc server even if the test fails.
	#
	set status [catch {
		tclsleep 2
		set home [file tail $rpc_testdir]
		set env [eval {berkdb_env_noerr -create -mode 0644 \
		    -home $home -server $rpc_server}]
		error_check_good lock_env:open [is_valid_env $env] TRUE

		#
		# Attempt to send in a NULL callback to associate.  It will
		# fail if the primary and secondary are not both read-only.
		#
		set msg "\tRpc003.d"
		puts "$msg: Using r/w primary and r/w secondary"
		set popen "berkdb_open_noerr -env $env $pomethod $pargs $pname"
		set sopen "berkdb_open_noerr -create -env $env \
		    [lindex $omethods 0] [lindex $argses 0] $snamebase.0.db"
		rpc003_assoc_err $popen $sopen $msg

		set msg "\tRpc003.e"
		puts "$msg: Using r/w primary and read-only secondary"
		set popen "berkdb_open_noerr -env $env $pomethod $pargs $pname"
		set sopen "berkdb_open_noerr -env $env -rdonly \
		    [lindex $omethods 0] [lindex $argses 0] $snamebase.0.db"
		rpc003_assoc_err $popen $sopen $msg

		set msg "\tRpc003.f"
		puts "$msg: Using read-only primary and r/w secondary"
		set popen "berkdb_open_noerr -env $env \
		    $pomethod -rdonly $pargs $pname"
		set sopen "berkdb_open_noerr -create -env $env \
		    [lindex $omethods 0] [lindex $argses 0] $snamebase.0.db"
		rpc003_assoc_err $popen $sopen $msg

		# Open and associate the secondaries
		puts "\tRpc003.g: Checking secondaries, both read-only"
		set pdb [eval {berkdb_open_noerr -env} $env \
		    -rdonly $pomethod $pargs $pname]
		error_check_good primary_open2 [is_valid_db $pdb] TRUE

		set sdbs {}
		for { set i 0 } { $i < [llength $omethods] } { incr i } {
			set sdb [eval {berkdb_open -env} $env -rdonly \
			    [lindex $omethods $i] [lindex $argses $i] \
			    $snamebase.$i.db]
			error_check_good second_open2($i) \
			    [is_valid_db $sdb] TRUE
			error_check_good db_associate2($i) \
			    [eval {$pdb associate} "" $sdb] 0
			lappend sdbs $sdb
		}
		check_secondaries $pdb $sdbs $nentries keys data "Rpc003.h"

		foreach sdb $sdbs {
			error_check_good secondary_close [$sdb close] 0
		}
		error_check_good primary_close [$pdb close] 0
		error_check_good env_close [$env close] 0
	} res]
	if { $status != 0 } {
		puts $res
	}
	tclkill $dpid
}

proc rpc003_assoc_err { popen sopen msg } {
	global rpc_svc

	set pdb [eval $popen]
	error_check_good assoc_err_popen [is_valid_db $pdb] TRUE

	puts "$msg.0: NULL callback"
	set sdb [eval $sopen]
	error_check_good assoc_err_sopen [is_valid_db $sdb] TRUE
	set stat [catch {eval {$pdb associate} "" $sdb} ret]
	error_check_good db_associate:rdonly $stat 1
	error_check_good db_associate:inval [is_substr $ret invalid] 1

	# The Java and JE RPC servers support callbacks.
	if { $rpc_svc == "berkeley_db_svc" || \
	     $rpc_svc == "berkeley_db_cxxsvc" } {
		puts "$msg.1: non-NULL callback"
		set stat [catch {eval $pdb associate [callback_n 0] $sdb} ret]
		error_check_good db_associate:callback $stat 1
		error_check_good db_associate:inval [is_substr $ret invalid] 1
	}

	error_check_good assoc_sclose [$sdb close] 0
	error_check_good assoc_pclose [$pdb close] 0
}
