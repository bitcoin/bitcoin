# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# The procs in this file are used for replication messaging
# ONLY when the default mechanism of setting up a queue of
# messages in a environment is not possible.  This situation
# is fairly rare, but it is necessary when a replication
# test simultaneously runs different versions of Berkeley DB,
# because different versions cannot share an env.
#
# Note, all procs should be named with the suffix _noenv
# so it's explicit that we are using them.
#
# Close up a replication group - close all message dbs.
proc replclose_noenv { queuedir } {
	global queuedbs machids

	set dbs [array names queuedbs]
	foreach tofrom $dbs {
		set handle $queuedbs($tofrom)
		error_check_good db_close [$handle close] 0
		unset queuedbs($tofrom)
	}

	set machids {}
}

# Create a replication group for testing.
proc replsetup_noenv { queuedir } {
	global queuedbs machids

	file mkdir $queuedir

	# If there are any leftover handles, get rid of them.
	set dbs [array names queuedbs]
	foreach tofrom $dbs {
		unset queuedbs($tofrom)
	}
	set machids {}
}

# Send function for replication.
proc replsend_noenv { control rec fromid toid flags lsn } {
	global is_repchild
	global queuedbs machids
	global drop drop_msg
	global perm_sent_list
	global anywhere
	global qtestdir testdir

	if { ![info exists qtestdir] } {
		set qtestdir $testdir
	}
	set queuedir $qtestdir/MSGQUEUEDIR
	set permflags [lsearch $flags "perm"]
	if { [llength $perm_sent_list] != 0 && $permflags != -1 } {
#		puts "replsend_noenv sent perm message, LSN $lsn"
		lappend perm_sent_list $lsn
	}

	#
	# If we are testing with dropped messages, then we drop every
	# $drop_msg time.  If we do that just return 0 and don't do
	# anything.
	#
	if { $drop != 0 } {
		incr drop
		if { $drop == $drop_msg } {
			set drop 1
			return 0
		}
	}
	# XXX
	# -1 is DB_BROADCAST_EID
	if { $toid == -1 } {
		set machlist $machids
	} else {
		set m NULL
		# If we can send this anywhere, send it to the first id
		# we find that is neither toid or fromid.  If we don't
		# find any other candidates, this falls back to the
		# original toid.
		if { $anywhere != 0 } {
			set anyflags [lsearch $flags "any"]
			if { $anyflags != -1 } {
				foreach m $machids {
					if { $m == $fromid || $m == $toid } {
						continue
					}
					set machlist [list $m]
					break
				}
			}
		}
		#
		# If we didn't find a different site, fall back
		# to the toid.
		#
		if { $m == "NULL" } {
			set machlist [list $toid]
		}
	}
	foreach m $machlist {
		# Do not broadcast to self.
		if { $m == $fromid } {
			continue
		}
		# Find the handle for the right message file.
		set pid [pid]
		set db $queuedbs($m.$fromid.$pid)
		set stat [catch {$db put -append [list $control $rec $fromid]} ret]
	}
	if { $is_repchild } {
		replready_noenv $fromid from
	}

	return 0
}

#
proc replmsglen_noenv { machid {tf "to"}} {
	global queuedbs qtestdir testdir

	if { ![info exists qtestdir] } {
		set qtestdir $testdir
	}
	set queuedir $qtestdir/MSGQUEUEDIR
	set orig [pwd]

	cd $queuedir
	if { $tf == "to" } {
		set msgdbs [glob -nocomplain ready.$machid.*]
	} else {
		set msgdbs [glob -nocomplain ready.*.$machid.*]
	}
	cd $orig
	return [llength $msgdbs]
}

# Discard all the pending messages for a particular site.
proc replclear_noenv { machid {tf "to"}} {
	global queuedbs qtestdir testdir

	if { ![info exists qtestdir] } {
		set qtestdir $testdir
	}
	set queuedir $qtestdir/MSGQUEUEDIR
	set orig [pwd]

	cd $queuedir
	if { $tf == "to" } {
		set msgdbs [glob -nocomplain ready.$machid.*]
	} else {
		set msgdbs [glob -nocomplain ready.*.$machid.*]
	}
	foreach m $msgdbs {
		file delete -force $m
	}
	cd $orig
	set dbs [array names queuedbs]
	foreach tofrom $dbs {
		# Process only messages _to_ the specified machid.
		if { [string match $machid.* $tofrom] == 1 } {
			set db $queuedbs($tofrom)
			set dbc [$db cursor]
			for { set dbt [$dbc get -first] } \
			    { [llength $dbt] > 0 } \
			    { set dbt [$dbc get -next] } {
				error_check_good \
				    replclear($machid)_del [$dbc del] 0
			}
			error_check_good replclear($db)_dbc_close [$dbc close] 0
		}
	}
	cd $queuedir
	if { $tf == "to" } {
		set msgdbs [glob -nocomplain temp.$machid.*]
	} else {
		set msgdbs [glob -nocomplain temp.*.$machid.*]
	}
	foreach m $msgdbs {
#		file delete -force $m
	}
	cd $orig
}

# Makes messages available to replprocessqueue by closing and
# renaming the message files.  We ready the files for one machine
# ID at a time -- just those "to" or "from" the machine we want to
# process, depending on 'tf'.
proc replready_noenv { machid tf } {
	global queuedbs machids
	global counter
	global qtestdir testdir

	if { ![info exists qtestdir] } {
		set qtestdir $testdir
	}
	set queuedir $qtestdir/MSGQUEUEDIR

	set pid [pid]
	#
	# Close the temporary message files for the specified machine.
	# Only close it if there are messages available.
	#
	set dbs [array names queuedbs]
	set closed {}
	foreach tofrom $dbs {
		set toidx [string first . $tofrom]
		set toid [string replace $tofrom $toidx end]
		set fidx [expr $toidx + 1]
		set fromidx [string first . $tofrom $fidx]
		#
		# First chop off the end, then chop off the toid
		# in the beginning.
		#
		set fromid [string replace $tofrom $fromidx end]
		set fromid [string replace $fromid 0 $toidx]
		if { ($tf == "to" && $machid == $toid) || \
		    ($tf == "from" && $machid == $fromid) } {
			set nkeys [stat_field $queuedbs($tofrom) \
			    stat "Number of keys"]
			if { $nkeys != 0 } {
				lappend closed \
				    [list $toid $fromid temp.$tofrom]
		 		error_check_good temp_close \
				    [$queuedbs($tofrom) close] 0
			}
		}
	}

	# Rename the message files.
	set cwd [pwd]
	foreach filename $closed {
		set toid [lindex $filename 0]
		set fromid [lindex $filename 1]
		set fname [lindex $filename 2]
		set tofrom [string replace $fname 0 4]
		incr counter($machid)
		cd $queuedir
# puts "$queuedir: Msg ready $fname to ready.$tofrom.$counter($machid)"
		file rename -force $fname ready.$tofrom.$counter($machid)
		cd $cwd
		replsetuptempfile_noenv $toid $fromid $queuedir

	}
}

# Add a machine to a replication environment.  This checks
# that we have not already established that machine id, and
# adds the machid to the list of ids.
proc repladd_noenv { machid } {
	global queuedbs machids counter qtestdir testdir

	if { ![info exists qtestdir] } {
		set qtestdir $testdir
	}
	set queuedir $qtestdir/MSGQUEUEDIR
	if { [info exists machids] } {
		if { [lsearch -exact $machids $machid] >= 0 } {
			error "FAIL: repladd_noenv: machid $machid already exists."
		}
	}

	set counter($machid) 0
	lappend machids $machid

	# Create all the databases that receive messages sent _to_
	# the new machid.
	replcreatetofiles_noenv $machid $queuedir

	# Create all the databases that receive messages sent _from_
	# the new machid.
	replcreatefromfiles_noenv $machid $queuedir
}

# Creates all the databases that a machid needs for receiving messages
# from other participants in a replication group.  Used when first
# establishing the temp files, but also used whenever replready_noenv moves
# the temp files away, because we'll need new files for any future messages.
proc replcreatetofiles_noenv { toid queuedir } {
	global machids

	foreach m $machids {
		# We don't need a file for a machid to send itself messages.
		if { $m == $toid } {
			continue
		}
		replsetuptempfile_noenv $toid $m $queuedir
	}
}

# Creates all the databases that a machid needs for sending messages
# to other participants in a replication group.  Used when first
# establishing the temp files only.  Replready moves files based on
# recipient, so we recreate files based on the recipient, also.
proc replcreatefromfiles_noenv { fromid queuedir } {
	global machids

	foreach m $machids {
		# We don't need a file for a machid to send itself messages.
		if { $m == $fromid } {
			continue
		}
		replsetuptempfile_noenv $m $fromid $queuedir
	}
}

proc replsetuptempfile_noenv { to from queuedir } {
	global queuedbs

	set pid [pid]
# puts "Open new temp.$to.$from.$pid"
	set queuedbs($to.$from.$pid) [berkdb_open -create -excl -recno\
	    -renumber $queuedir/temp.$to.$from.$pid]
	error_check_good open_queuedbs [is_valid_db $queuedbs($to.$from.$pid)] TRUE
}

# Process a queue of messages, skipping every "skip_interval" entry.
# We traverse the entire queue, but since we skip some messages, we
# may end up leaving things in the queue, which should get picked up
# on a later run.
proc replprocessqueue_noenv { dbenv machid { skip_interval 0 } { hold_electp NONE } \
    { dupmasterp NONE } { errp NONE } } {
	global errorCode
	global perm_response_list
	global qtestdir testdir

	# hold_electp is a call-by-reference variable which lets our caller
	# know we need to hold an election.
	if { [string compare $hold_electp NONE] != 0 } {
		upvar $hold_electp hold_elect
	}
	set hold_elect 0

	# dupmasterp is a call-by-reference variable which lets our caller
	# know we have a duplicate master.
	if { [string compare $dupmasterp NONE] != 0 } {
		upvar $dupmasterp dupmaster
	}
	set dupmaster 0

	# errp is a call-by-reference variable which lets our caller
	# know we have gotten an error (that they expect).
	if { [string compare $errp NONE] != 0 } {
		upvar $errp errorp
	}
	set errorp 0

	set nproced 0

	set queuedir $qtestdir/MSGQUEUEDIR
# puts "replprocessqueue_noenv: Make ready messages to eid $machid"

	# Change directories temporarily so we get just the msg file name.
	set cwd [pwd]
	cd $queuedir
	set msgdbs [glob -nocomplain ready.$machid.*]
# puts "$queuedir.$machid: My messages: $msgdbs"
	cd $cwd

	foreach msgdb $msgdbs {
		set db [berkdb_open $queuedir/$msgdb]
		set dbc [$db cursor]

		error_check_good process_dbc($machid) \
		    [is_valid_cursor $dbc $db] TRUE

		for { set dbt [$dbc get -first] } \
		    { [llength $dbt] != 0 } \
		    { set dbt [$dbc get -next] } {
			set data [lindex [lindex $dbt 0] 1]
			set recno [lindex [lindex $dbt 0] 0]

			# If skip_interval is nonzero, we want to process
			# messages out of order.  We do this in a simple but
			# slimy way -- continue walking with the cursor
			# without processing the message or deleting it from
			# the queue, but do increment "nproced".  The way
			# this proc is normally used, the precise value of
			# nproced doesn't matter--we just don't assume the
			# queues are empty if it's nonzero.  Thus, if we
			# contrive to make sure it's nonzero, we'll always
			# come back to records we've skipped on a later call
			# to replprocessqueue.  (If there really are no records,
			# we'll never get here.)
			#
			# Skip every skip_interval'th record (and use a
			# remainder other than zero so that we're guaranteed
			# to really process at least one record on every call).
			if { $skip_interval != 0 } {
				if { $nproced % $skip_interval == 1 } {
					incr nproced
					set dbt [$dbc get -next]
					continue
				}
			}

			# We need to remove the current message from the
			# queue, because we're about to end the transaction
			# and someone else processing messages might come in
			# and reprocess this message which would be bad.
			#
			error_check_good queue_remove [$dbc del] 0

			# We have to play an ugly cursor game here:  we
			# currently hold a lock on the page of messages, but
			# rep_process_message might need to lock the page with
			# a different cursor in order to send a response.  So
			# save the next recno, close the cursor, and then
			# reopen and reset the cursor.  If someone else is
			# processing this queue, our entry might have gone
			# away, and we need to be able to handle that.
			#
#			error_check_good dbc_process_close [$dbc close] 0

			set ret [catch {$dbenv rep_process_message \
			    [lindex $data 2] [lindex $data 0] \
			    [lindex $data 1]} res]

			# Save all ISPERM and NOTPERM responses so we can
			# compare their LSNs to the LSN in the log.  The
			# variable perm_response_list holds the entire
			# response so we can extract responses and LSNs as
			# needed.
			#
			if { [llength $perm_response_list] != 0 && \
			    ([is_substr $res ISPERM] || [is_substr $res NOTPERM]) } {
				lappend perm_response_list $res
			}

			if { $ret != 0 } {
				if { [string compare $errp NONE] != 0 } {
					set errorp "$dbenv $machid $res"
				} else {
					error "FAIL:[timestamp]\
					    rep_process_message returned $res"
				}
			}

			incr nproced
			if { $ret == 0 } {
				set rettype [lindex $res 0]
				set retval [lindex $res 1]
				#
				# Do nothing for 0 and NEWSITE
				#
				if { [is_substr $rettype HOLDELECTION] } {
					set hold_elect 1
				}
				if { [is_substr $rettype DUPMASTER] } {
					set dupmaster "1 $dbenv $machid"
				}
				if { [is_substr $rettype NOTPERM] || \
				    [is_substr $rettype ISPERM] } {
					set lsnfile [lindex $retval 0]
					set lsnoff [lindex $retval 1]
				}
			}

			if { $errorp != 0 } {
				# Break on an error, caller wants to handle it.
				break
			}
			if { $hold_elect == 1 } {
				# Break on a HOLDELECTION, for the same reason.
				break
			}
			if { $dupmaster == 1 } {
				# Break on a DUPMASTER, for the same reason.
				break
			}

		}
		error_check_good dbc_close [$dbc close] 0

		#
		# Check the number of keys remaining because we only
		# want to rename to done, message file that are
		# fully processed.  Some message types might break
		# out of the loop early and we want to process
		# the remaining messages the next time through.
		#
		set nkeys [stat_field $db stat "Number of keys"]
		error_check_good db_close [$db close] 0

		if { $nkeys == 0 } {
			set dbname [string replace $msgdb 0 5 done.]
			#
			# We have to do a special dance to get rid of the
			# empty messaging files because of the way Windows
			# handles open files marked for deletion.
			# On Windows, a file is marked for deletion but
			# does not actually get deleted until the last handle
			# is closed.  This causes a problem when a test tries
			# to create a new file with a previously-used name,
			# and Windows believes the old file still exists.
			# Therefore, we rename the files before deleting them,
			# to guarantee they are out of the way.
			#
			file rename -force $queuedir/$msgdb $queuedir/$dbname
			file delete -force $queuedir/$dbname
		}
	}
	# Return the number of messages processed.
	return $nproced
}

