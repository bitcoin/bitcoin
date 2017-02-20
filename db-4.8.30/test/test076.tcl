# See the file LICENSE for redistribution information.
#
# Copyright (c) 2000-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test076
# TEST	Test creation of many small databases in a single environment. [#1528].
proc test076 { method { ndbs 1000 } { tnum "076" } args } {
	global is_qnx_test
	source ./include.tcl

	set args [convert_args $method $args]
	set encargs ""
	set args [split_encargs $args encargs]
	set omethod [convert_method $method]
	set pageargs ""
	split_pageargs $args pageargs

	if { [is_record_based $method] == 1 } {
		set key ""
	} else {
		set key "key"
	}
	set data "datamoredatamoredata"

	# Create an env if we weren't passed one.
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	if { $eindex == -1 } {
		set deleteenv 1
		env_cleanup $testdir
		set env [eval \
		    {berkdb_env -create -home} $testdir $pageargs $encargs]
		error_check_good env [is_valid_env $env] TRUE
		set args "$args -env $env"
	} else {
		set deleteenv 0
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
			if { $ndbs == 1000 } {
				set ndbs 100
			}
		}
		set testdir [get_home $env]
	}
	if { $is_qnx_test && $ndbs > 100 } {
		set ndbs 100
	}
	if { [is_queueext $method] } {
		set ndbs 500
	}

	puts -nonewline "Test$tnum $method ($args): "
	puts -nonewline "Create $ndbs"
	puts " small databases in one env."

	cleanup $testdir $env
	set txn ""

	for { set i 1 } { $i <= $ndbs } { incr i } {
		set testfile test$tnum.$i.db

		set db [eval {berkdb_open -create -mode 0644}\
		    $args $omethod $testfile]
		error_check_good db_open($i) [is_valid_db $db] TRUE

		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval {$db put} $txn {$key$i \
		    [chop_data $method $data$i]}]
		error_check_good db_put($i) $ret 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
		error_check_good db_close($i) [$db close] 0
	}

	if { $deleteenv == 1 } {
		error_check_good env_close [$env close] 0
	}

	puts "\tTest$tnum passed."
}
