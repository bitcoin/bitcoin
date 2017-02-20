# See the file LICENSE for redistribution information.
#
# Copyright (c) 2003-2009 Oracle.  All rights reserved.
#
# $Id$
#
proc do_op {omethod op names txn env {largs ""}} {
	switch -exact $op {
		delete { do_delete $names }
		rename { do_rename $names $txn $env }
		remove { do_remove $names $txn $env }
		noop { do_noop }
		open_create { do_create $omethod $names $txn $env $largs }
		open { do_open $omethod $names $txn $env $largs }
		open_excl { do_create_excl $omethod $names $txn $env $largs }
		truncate { do_truncate $omethod $names $txn $env $largs }
		default { puts "FAIL: operation $op not recognized" }
	}
}

proc do_subdb_op {omethod op names txn env {largs ""}} {
	#
	# The 'noop' and 'delete' actions are the same
	# for subdbs as for regular db files.
	#
	switch -exact $op {
		delete { do_delete $names }
		rename { do_subdb_rename $names $txn $env }
		remove { do_subdb_remove $names $txn $env }
		noop { do_noop }
		default { puts "FAIL: operation $op not recognized" }
	}
}

proc do_inmem_op {omethod op names txn env {largs ""}} {
	#
	# The in-memory versions of do_op are different in
	# that we don't need to pass in the filename, just
	# the subdb names.
	#
	switch -exact $op {
		delete { do_delete $names }
		rename { do_inmem_rename $names $txn $env }
		remove { do_inmem_remove $names $txn $env }
		noop { do_noop }
		open_create { do_inmem_create $omethod $names $txn $env $largs }
		open { do_inmem_open $omethod $names $txn $env $largs }
		open_excl { do_inmem_create_excl $omethod $names $txn $env $largs }
		truncate { do_inmem_truncate $omethod $names $txn $env $largs }
		default { puts "FAIL: operation $op not recognized" }
	}
}

proc do_delete {names} {
	#
	# This is the odd man out among the ops -- it's not a Berkeley
	# DB file operation, but mimics an operation done externally,
	# as if a user deleted a file with "rm" or "erase".
	#
	# We assume the file is found in $testdir.
	#
	global testdir

	if {[catch [fileremove -f $testdir/$names] result]} {
		return $result
	} else {
		return 0
	}
}

proc do_noop { } {
	# Do nothing.  Report success.
	return 0
}

proc do_rename {names txn env} {
	# Pull db names out of $names
	set oldname [lindex $names 0]
	set newname [lindex $names 1]

	if {[catch {eval $env dbrename -txn $txn \
	    $oldname $newname} result]} {
		return $result
	} else {
		return 0
	}
}

proc do_subdb_rename {names txn env} {
	# Pull db and subdb names out of $names
	set filename [lindex $names 0]
	set oldsname [lindex $names 1]
	set newsname [lindex $names 2]

	if {[catch {eval $env dbrename -txn $txn $filename \
	    $oldsname $newsname} result]} {
		return $result
	} else {
		return 0
	}
}

proc do_inmem_rename {names txn env} {
	# Pull db and subdb names out of $names
	set filename ""
	set oldsname [lindex $names 0]
	set newsname [lindex $names 1]
	if {[catch {eval $env dbrename -txn $txn {$filename} \
	    $oldsname $newsname} result]} {
		return $result
	} else {
		return 0
	}
}


proc do_remove {names txn env} {
	if {[catch {eval $env dbremove -txn $txn $names} result]} {
		return $result
	} else {
		return 0
	}
}

proc do_subdb_remove {names txn env} {
	set filename [lindex $names 0]
	set subname [lindex $names 1]
	if {[catch {eval $env dbremove -txn $txn $filename $subname} result]} {
		return $result
	} else {
		return 0
	}
}

proc do_inmem_remove {names txn env} {
	if {[catch {eval $env dbremove -txn $txn {""} $names} result]} {
		return $result
	} else {
		return 0
	}
}

proc do_create {omethod names txn env {largs ""}} {
	if {[catch {eval berkdb_open -create $omethod $largs -env $env \
	    -txn $txn $names} result]} {
		return $result
	} else {
		return 0
	}
}

proc do_inmem_create {omethod names txn env {largs ""}} {
	if {[catch {eval berkdb_open -create $omethod $largs -env $env \
	    -txn $txn "" $names} result]} {
		return $result
	} else {
		return 0
	}
}

proc do_open {omethod names txn env {largs ""}} {
	if {[catch {eval berkdb_open $omethod $largs -env $env \
	    -txn $txn $names} result]} {
		return $result
	} else {
		return 0
	}
}

proc do_inmem_open {omethod names txn env {largs ""}} {
	if {[catch {eval berkdb_open $omethod $largs -env $env \
	    -txn $txn {""} $names} result]} {
		return $result
	} else {
		return 0
	}
}

proc do_create_excl {omethod names txn env {largs ""}} {
	if {[catch {eval berkdb_open -create -excl $omethod $largs -env $env \
	    -txn $txn $names} result]} {
		return $result
	} else {
		return 0
	}
}

proc do_inmem_create_excl {omethod names txn env {largs ""}} {
	if {[catch {eval berkdb_open -create -excl $omethod $largs -env $env \
	    -txn $txn {""} $names} result]} {
		return $result
	} else {
		return 0
	}
}

proc do_truncate {omethod names txn env {largs ""}} {
	# First we have to get a handle.  We omit the -create flag
	# because testing of truncate is meaningful only in cases
	# where the database already exists.
	set db [eval {berkdb_open $omethod} $largs {-env $env -txn $txn $names}]
	error_check_good db_open [is_valid_db $db] TRUE

	if {[catch {$db truncate -txn $txn} result]} {
		return $result
	} else {
		return 0
	}
}

proc do_inmem_truncate {omethod names txn env {largs ""}} {
	set db [eval {berkdb_open $omethod} $largs {-env $env -txn $txn "" $names}]
	error_check_good db_open [is_valid_db $db] TRUE

	if {[catch {$db truncate -txn $txn} result]} {
		return $result
	} else {
		return 0
	}
}

proc create_tests { op1 op2 exists noexist open retval { end1 "" } } {
	set retlist {}
	switch $op1 {
		rename {
			# Use first element from exists list
			set from [lindex $exists 0]
			# Use first element from noexist list
			set to [lindex $noexist 0]

			# This is the first operation, which should succeed
			set op1ret [list $op1 "$from $to" 0 $end1]

			# Adjust 'exists' and 'noexist' list if and only if
			# txn1 was not aborted.
			if { $end1 != "abort" } {
				set exists [lreplace $exists 0 0 $to]
				set noexist [lreplace $noexist 0 0 $from]
			}
		}
		remove {
			set from [lindex $exists 0]
			set op1ret [list $op1 $from 0 $end1]

			if { $end1 != "abort" } {
				set exists [lreplace $exists 0 0]
				set noexist [lreplace $noexist 0 0 $from]
			}
		}
		open_create -
		open -
		truncate {
			set from [lindex $exists 0]
			set op1ret [list $op1 $from 0 $end1]

			if { $end1 != "abort" } {
				set exists [lreplace $exists 0 0]
				set open [list $from]
			}

			# Eliminate the 1st element in noexist: it is
			# equivalent to the 2nd element (neither ever exists).
			set noexist [lreplace $noexist 0 0]
		}
		open_excl {
			# Use first element from noexist list
			set from [lindex $noexist 0]
			set op1ret [list $op1 $from 0 $end1]

			if { $end1 != "abort" } {
				set noexist [lreplace $noexist 0 0]
				set open [list $from]
			}

			# Eliminate the 1st element in exists: it is
			# equivalent to the 2nd element (both already exist).
			set exists [lreplace $exists 0 0]
		}
	}

	# Generate possible second operations given the return value.
	set op2list [create_op2 $op2 $exists $noexist $open $retval]

	foreach o $op2list {
		lappend retlist [list $op1ret $o]
	}
	return $retlist
}

proc create_badtests { op1 op2 exists noexist open retval {end1 ""} } {
	set retlist {}
	switch $op1 {
		rename {
			# Use first element from exists list
			set from [lindex $exists 0]
			# Use first element from noexist list
			set to [lindex $noexist 0]

			# This is the first operation, which should fail
			set op1list1 \
			    [list $op1 "$to $to" "no such file" $end1]
			set op1list2 \
			    [list $op1 "$to $from" "no such file" $end1]
			set op1list3 \
			    [list $op1 "$from $from" "file exists" $end1]
			set op1list [list $op1list1 $op1list2 $op1list3]

			# Generate second operations given the return value.
			set op2list [create_op2 \
			    $op2 $exists $noexist $open $retval]
			foreach op1 $op1list {
				foreach op2 $op2list {
					lappend retlist [list $op1 $op2]
				}
			}
			return $retlist
		}
		remove -
		open -
		truncate {
			set file [lindex $noexist 0]
			set op1list [list $op1 $file "no such file" $end1]

			set op2list [create_op2 \
			    $op2 $exists $noexist $open $retval]
			foreach op2 $op2list {
				lappend retlist [list $op1list $op2]
			}
			return $retlist
		}
		open_excl {
			set file [lindex $exists 0]
			set op1list [list $op1 $file "file exists" $end1]
			set op2list [create_op2 \
			    $op2 $exists $noexist $open $retval]
			foreach op2 $op2list {
				lappend retlist [list $op1list $op2]
			}
			return $retlist
		}
	}
}

proc create_op2 { op2 exists noexist open retval } {
	set retlist {}
	switch $op2 {
		rename {
			# Successful renames arise from renaming existing
			# to non-existing files.
			if { $retval == 0 } {
				set old $exists
				set new $noexist
				set retlist \
				    [build_retlist $op2 $old $new $retval]
			}
			# "File exists" errors arise from renaming existing
			# to existing files.
			if { $retval == "file exists" } {
				set old $exists
				set new $exists
				set retlist \
				    [build_retlist $op2 $old $new $retval]
			}
			# "No such file" errors arise from renaming files
			# that don't exist.
			if { $retval == "no such file" } {
				set old $noexist
				set new $exists
				set retlist1 \
				    [build_retlist $op2 $old $new $retval]

				set old $noexist
				set new $noexist
				set retlist2 \
				    [build_retlist $op2 $old $new $retval]

				set retlist [concat $retlist1 $retlist2]
			}
		}
		remove {
			# Successful removes result from removing existing
			# files.
			if { $retval == 0 } {
				set file $exists
			}
			# "File exists" does not happen in remove.
			if { $retval == "file exists" } {
				return
			}
			# "No such file" errors arise from trying to remove
			# files that don't exist.
			if { $retval == "no such file" } {
				set file $noexist
			}
			set retlist [build_retlist $op2 $file "" $retval]
		}
		open_create {
			# Open_create should be successful with existing,
			# open, or non-existing files.
			if { $retval == 0 } {
				set file [concat $exists $open $noexist]
			}
			# "File exists" and "no such file"
			# do not happen in open_create.
			if { $retval == "file exists" || \
			    $retval == "no such file" } {
				return
			}
			set retlist [build_retlist $op2 $file "" $retval]
		}
		open {
			# Open should be successful with existing or open files.
			if { $retval == 0 } {
				set file [concat $exists $open]
			}
			# "No such file" errors arise from trying to open
			# non-existent files.
			if { $retval == "no such file" } {
				set file $noexist
			}
			# "File exists" errors do not happen in open.
			if { $retval == "file exists" } {
				return
			}
			set retlist [build_retlist $op2 $file "" $retval]
		}
		open_excl {
			# Open_excl should be successful with non-existent files.
			if { $retval == 0 } {
				set file $noexist
			}
			# "File exists" errors arise from trying to open
			# existing files.
			if { $retval == "file exists" } {
				set file [concat $exists $open]
			}
			# "No such file" errors do not arise in open_excl.
			if { $retval == "no such file" } {
				return
			}
			set retlist [build_retlist $op2 $file "" $retval]
		}
		truncate {
			# Truncate should be successful with existing files.
			if { $retval == 0 } {
				set file $exists
			}
			# No other return values are meaningful to test since
			# do_truncate starts with an open and we've already
			# tested open.
			if { $retval == "no such file" || \
			    $retval == "file exists" } {
				return
			}
			set retlist [build_retlist $op2 $file "" $retval]
		}
	}
	return $retlist
}

proc build_retlist { op2 file1 file2 retval } {
	set retlist {}
	if { $file2 == "" } {
		foreach f1 $file1 {
			lappend retlist [list $op2 $f1 $retval]
		}
	} else {
		foreach f1 $file1 {
			foreach f2 $file2 {
				lappend retlist [list $op2 "$f1 $f2" $retval]
			}
		}
	}
	return $retlist
}

proc extract_error { message } {
	if { [is_substr $message "exists"] == 1 } {
		set message "file exists"
	} elseif {[is_substr $message "no such file"] == 1 } {
		set message "no such file"
	}
	return $message
}
