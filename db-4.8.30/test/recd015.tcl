# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	recd015
# TEST	This is a recovery test for testing lots of prepared txns.
# TEST	This test is to force the use of txn_recover to call with the
# TEST	DB_FIRST flag and then DB_NEXT.
proc recd015 { method args } {
	source ./include.tcl
	global rand_init
	error_check_good set_random_seed [berkdb srand $rand_init] 0

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	puts "Recd015: $method ($args) prepared txns test"

	# Create the database and environment.

	set numtxns 1
	set testfile NULL

	set env_cmd "berkdb_env -create -txn -home $testdir"
	set msg "\tRecd015.a"
	foreach op { abort commit discard } {
		puts "$msg: Simple test to prepare $numtxns txn with $op "
		env_cleanup $testdir
		recd015_body $env_cmd $testfile $numtxns $msg $op
	}

	#
	# Now test large numbers of prepared txns to test DB_NEXT
	# on txn_recover.
	#
	set numtxns 10000
	set txnmax [expr $numtxns + 5]
	set env_cmd "berkdb_env -create -txn_max $txnmax \
	    -lock_max_lockers $txnmax -txn -home $testdir"

	set msg "\tRecd015.b"
	foreach op { abort commit discard } {
		puts "$msg: Large test to prepare $numtxns txn with $op"
		env_cleanup $testdir
		recd015_body $env_cmd $testfile $numtxns $msg $op
	}

	set stat [catch {exec $util_path/db_printlog -h $testdir \
	    > $testdir/LOG } ret]
	error_check_good db_printlog $stat 0
	fileremove $testdir/LOG
}

proc recd015_body { env_cmd testfile numtxns msg op } {
	source ./include.tcl

	sentinel_init
	set gidf $testdir/gidfile
	fileremove -f $gidf
	set pidlist {}
	puts "$msg.0: Executing child script to prepare txns"
	berkdb debug_check
	set p [exec $tclsh_path $test_path/wrap.tcl recd15scr.tcl \
	    $testdir/recdout $env_cmd $testfile $gidf $numtxns &]

	lappend pidlist $p
	watch_procs $pidlist 5
	set f1 [open $testdir/recdout r]
	set r [read $f1]
	puts $r
	close $f1
	fileremove -f $testdir/recdout

	berkdb debug_check
	puts -nonewline "$msg.1: Running recovery ... "
	flush stdout
	berkdb debug_check
	set env [eval $env_cmd -recover]
	error_check_good dbenv-recover [is_valid_env $env] TRUE
	puts "complete"

	puts "$msg.2: getting txns from txn_recover"
	set txnlist [$env txn_recover]
	error_check_good txnlist_len [llength $txnlist] $numtxns

	set gfd [open $gidf r]
	set i 0
	while { [gets $gfd gid] != -1 } {
		set gids($i) $gid
		incr i
	}
	close $gfd
	#
	# Make sure we have as many as we expect
	error_check_good num_gids $i $numtxns

	set i 0
	puts "$msg.3: comparing GIDs and $op txns"
	foreach tpair $txnlist {
		set txn [lindex $tpair 0]
		set gid [lindex $tpair 1]
		error_check_good gidcompare $gid $gids($i)
		error_check_good txn:$op [$txn $op] 0
		incr i
	}
	if { $op != "discard" } {
		error_check_good envclose [$env close] 0
		return
	}
	#
	# If we discarded, now do it again and randomly resolve some
	# until all txns are resolved.
	#
	puts "$msg.4: resolving/discarding txns"
	set txnlist [$env txn_recover]
	set len [llength $txnlist]
	set opval(1) "abort"
	set opcnt(1) 0
	set opval(2) "commit"
	set opcnt(2) 0
	set opval(3) "discard"
	set opcnt(3) 0
	while { $len != 0 } {
		set opicnt(1) 0
		set opicnt(2) 0
		set opicnt(3) 0
		#
		# Abort/commit or discard them randomly until
		# all are resolved.
		#
		for { set i 0 } { $i < $len } { incr i } {
			set t [lindex $txnlist $i]
			set txn [lindex $t 0]
			set newop [berkdb random_int 1 3]
			set ret [$txn $opval($newop)]
			error_check_good txn_$opval($newop):$i $ret 0
			incr opcnt($newop)
			incr opicnt($newop)
		}
#		puts "$opval(1): $opicnt(1) Total: $opcnt(1)"
#		puts "$opval(2): $opicnt(2) Total: $opcnt(2)"
#		puts "$opval(3): $opicnt(3) Total: $opcnt(3)"

		set txnlist [$env txn_recover]
		set len [llength $txnlist]
	}

	error_check_good envclose [$env close] 0
}
