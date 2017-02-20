# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$

source ./include.tcl

global upgrade_dir
# set upgrade_dir "$test_path/upgrade_test"
set upgrade_dir "$test_path/upgrade/databases"

global gen_upgrade
set gen_upgrade 0
global gen_dump
set gen_dump 0
global gen_chksum
set gen_chksum 0
global gen_upgrade_log
set gen_upgrade_log 0

global upgrade_dir
global upgrade_be
global upgrade_method
global upgrade_name

proc upgrade { { archived_test_loc "DEFAULT" } } {
	source ./include.tcl
	global test_names
	global upgrade_dir
	global tcl_platform
	global saved_logvers

	set saved_upgrade_dir $upgrade_dir

	# Identify endianness of the machine running upgrade.
	if { [big_endian] == 1 } {
		set myendianness be
	} else {
		set myendianness le
	}
	set e $tcl_platform(byteOrder)

	if { [file exists $archived_test_loc/logversion] == 1 } {
		set fd [open $archived_test_loc/logversion r]
		set saved_logvers [read $fd]
		close $fd
	} else {
		puts "Old log version number must be available \
		    in $archived_test_loc/logversion"
		return
	}

	fileremove -f UPGRADE.OUT
	set o [open UPGRADE.OUT a]

	puts -nonewline $o "Upgrade test started at: "
	puts $o [clock format [clock seconds] -format "%H:%M %D"]
	puts $o [berkdb version -string]
	puts $o "Testing $e files"

	puts -nonewline "Upgrade test started at: "
	puts [clock format [clock seconds] -format "%H:%M %D"]
	puts [berkdb version -string]
	puts "Testing $e files"

	if { $archived_test_loc == "DEFAULT" } {
		puts $o "Using default archived databases in $upgrade_dir."
		puts "Using default archived databases in $upgrade_dir."
	} else {
		set upgrade_dir $archived_test_loc
		puts $o "Using archived databases in $upgrade_dir."
		puts "Using archived databases in $upgrade_dir."
	}
	close $o

	foreach version [glob $upgrade_dir/*] {
		if { [string first CVS $version] != -1 } { continue }
		regexp \[^\/\]*$ $version version

		# Test only files where the endianness of the db matches
		# the endianness of the test platform.  These are the
		# meaningful tests:
		# 1.  File generated on le, tested on le
		# 2.  File generated on be, tested on be
		# 3.  Byte-swapped file generated on le, tested on be
		# 4.  Byte-swapped file generated on be, tested on le
		#
		set dbendianness [string range $version end-1 end]
		if { [string compare $myendianness $dbendianness] != 0 } {
			puts "Skipping test of $version \
			    on $myendianness platform."
		} else {
			set release [string trim $version -lbe]
			set o [open UPGRADE.OUT a]
			puts $o "Files created on release $release"
			close $o
			puts "Files created on release $release"

			foreach method [glob $upgrade_dir/$version/*] {
				regexp \[^\/\]*$ $method method
				set o [open UPGRADE.OUT a]
				puts $o "\nTesting $method files"
				close $o
				puts "\tTesting $method files"

				foreach file [lsort -dictionary \
				    [glob -nocomplain \
				    $upgrade_dir/$version/$method/*]] {
					regexp (\[^\/\]*)\.tar\.gz$ \
					    $file dummy name

					cleanup $testdir NULL 1
					set curdir [pwd]
					cd $testdir
					set tarfd [open "|tar xf -" w]
					cd $curdir

					catch {exec gunzip -c \
					    "$upgrade_dir/$version/$method/$name.tar.gz" \
					    >@$tarfd}
					close $tarfd

					set f [open $testdir/$name.tcldump \
					    {RDWR CREAT}]
					close $f

					# We exec a separate tclsh for each
					# separate subtest to keep the
					# testing process from consuming a
					# tremendous amount of memory.
					#
					# First we test the .db files.
					if { [file exists \
					    $testdir/$name-$myendianness.db] } {
						if { [catch {exec $tclsh_path \
						    << "source \
						    $test_path/test.tcl;\
						    _upgrade_test $testdir \
						    $version $method $name \
						    $myendianness" >>& \
						    UPGRADE.OUT } message] } {
							set o [open \
							    UPGRADE.OUT a]
							puts $o "FAIL: $message"
							close $o
						}
						if { [catch {exec $tclsh_path\
						    << "source \
						    $test_path/test.tcl;\
						    _db_load_test $testdir \
						    $version $method $name" >>&\
						    UPGRADE.OUT } message] } {
							set o [open \
							    UPGRADE.OUT a]
							puts $o "FAIL: $message"
							close $o
						}
					}
					# Then we test log files.
					if { [file exists \
					    $testdir/$name.prlog] } {
						if { [catch {exec $tclsh_path \
						    << "source \
						    $test_path/test.tcl;\
						    global saved_logvers;\
						    set saved_logvers \
						    $saved_logvers;\
						    _log_test $testdir \
						    $release $method \
						    $name" >>& \
						    UPGRADE.OUT } message] } {
							set o [open \
							    UPGRADE.OUT a]
							puts $o "FAIL: $message"
							close $o
						}
					}

					# Then we test any .dmp files.  Move
					# the saved file to the current working
					# directory.  Run the test locally.
					# Compare the dumps; they should match.
					if { [file exists $testdir/$name.dmp] } {
						file rename -force \
						    $testdir/$name.dmp $name.dmp

						foreach test $test_names(plat) {
							eval $test $method
						}

						# Discard lines that can differ.
						discardline $name.dmp \
						    TEMPFILE "db_pagesize="
						file copy -force \
						    TEMPFILE $name.dmp
						discardline $testdir/$test.dmp \
						    TEMPFILE "db_pagesize="
						file copy -force \
						    TEMPFILE $testdir/$test.dmp

						error_check_good compare_dump \
						    [filecmp $name.dmp \
						    $testdir/$test.dmp] 0

						fileremove $name.dmp
					}
				}
			}
		}
	}
	set upgrade_dir $saved_upgrade_dir

	set o [open UPGRADE.OUT a]
	puts -nonewline $o "Completed at: "
	puts $o [clock format [clock seconds] -format "%H:%M %D"]
	close $o

	puts -nonewline "Completed at: "
	puts [clock format [clock seconds] -format "%H:%M %D"]

	# Don't provide a return value.
	return
}

proc _upgrade_test { temp_dir version method file endianness } {
	source include.tcl
	global errorInfo
	global passwd
	global encrypt

	puts "Upgrade: $version $method $file $endianness"

	# Check whether we're working with an encrypted file.
	if { [string match c-* $file] } {
		set encrypt 1
	}

	# Open the database prior to upgrading.  If it fails,
	# it should fail with the DB_OLDVERSION message.
	set encargs ""
	set upgradeargs ""
	if { $encrypt == 1 } {
		set encargs " -encryptany $passwd "
		set upgradeargs " -P $passwd "
	}
	if { [catch \
	    { set db [eval {berkdb open} $encargs \
	    $temp_dir/$file-$endianness.db] } res] } {
	    	error_check_good old_version [is_substr $res DB_OLDVERSION] 1
	} else {
		error_check_good db_close [$db close] 0
	}

	# Now upgrade the database.
	set ret [catch {eval exec {$util_path/db_upgrade} $upgradeargs \
	    "$temp_dir/$file-$endianness.db" } message]
	error_check_good dbupgrade $ret 0

	error_check_good dbupgrade_verify [verify_dir $temp_dir "" 0 0 1] 0

	upgrade_dump "$temp_dir/$file-$endianness.db" "$temp_dir/temp.dump"

	error_check_good "Upgrade diff.$endianness: $version $method $file" \
	    [filecmp "$temp_dir/$file.tcldump" "$temp_dir/temp.dump"] 0
}

proc _db_load_test { temp_dir version method file } {
	source include.tcl
	global errorInfo

	puts "Db_load: $version $method $file"

	set ret [catch \
	    {exec $util_path/db_load -f "$temp_dir/$file.dump" \
	    "$temp_dir/upgrade.db"} message]
	error_check_good \
	    "Upgrade load: $version $method $file $message" $ret 0

	upgrade_dump "$temp_dir/upgrade.db" "$temp_dir/temp.dump"

	error_check_good "Upgrade diff.1.1: $version $method $file" \
	    [filecmp "$temp_dir/$file.tcldump" "$temp_dir/temp.dump"] 0
}

proc _log_test { temp_dir release method file } {
	source ./include.tcl
	global saved_logvers
	global passwd
	puts "Check log file: $temp_dir $release $method $file"

	# Get log version number of current system
	set env [berkdb_env -create -log -home $testdir]
	error_check_good is_valid_env [is_valid_env $env] TRUE
	set current_logvers [get_log_vers $env]
	error_check_good env_close [$env close] 0
	error_check_good env_remove [berkdb envremove -home $testdir] 0

	# Rename recd001-x-log.000000000n to log.000000000n.
	set logfiles [glob -nocomplain $temp_dir/*log.0*]
	foreach logfile $logfiles {
		set logname [string replace $logfile 0 \
		    [string last - $logfile]]
		file rename -force $logfile $temp_dir/$logname
	}

	# Use db_printlog to dump the logs.  If the current log file
	# version is greater than the saved log file version, the log
	# files are expected to be unreadable.  If the log file is
	# readable, check that the current printlog dump matches the
	# archived printlog.
 	#
	set ret [catch {exec $util_path/db_printlog -h $temp_dir \
	    > $temp_dir/logs.prlog} message]
	if { [is_substr $message "magic number"] } {
		# The failure is probably due to encryption, try
		# crypto printlog.
		set ret [catch {exec $util_path/db_printlog -h $temp_dir \
		    -P $passwd > $temp_dir/logs.prlog} message]
		if { $ret == 1 } {
			# If the failure is because of a historic
			# log version, that's okay.
			if { $current_logvers <= $saved_logvers } {
				puts "db_printlog failed: $message"
		 	}
		}
	}

	# Log versions prior to 8 can only be read by their own version.
	# Log versions of 8 or greater are readable by Berkeley DB 4.5
	# or greater, but the output of printlog does not match unless
	# the versions are identical.
	# 
	# As of Berkeley DB 4.8, we'll only try to read back to log
	# version 11, which came out with 4.4.  Backwards compatibility 
	# now only extends back to 4.4 because of page changes. 
	# 
	set logoldver 11
	if { $current_logvers > $saved_logvers &&\
	    $saved_logvers < $logoldver } {
		error_check_good historic_log_version \
		    [is_substr $message "historic log version"] 1
	} elseif { $current_logvers > $saved_logvers } {
		error_check_good db_printlog:$message $ret 0
	} elseif { $current_logvers == $saved_logvers  } {
		error_check_good db_printlog:$message $ret 0
		# Compare logs.prlog and $file.prlog (should match)
		error_check_good "Compare printlogs" [filecmp \
		    "$temp_dir/logs.prlog" "$temp_dir/$file.prlog"] 0
	} elseif { $current_logvers < $saved_logvers } {
		puts -nonewline "FAIL: current log version $current_logvers "
		puts "cannot be less than saved log version $save_logvers."
	}
}

proc gen_upgrade { dir { save_crypto 1 } { save_non_crypto 1 } } {
	global gen_upgrade
	global gen_upgrade_log
	global gen_chksum
	global gen_dump
	global upgrade_dir
	global upgrade_be
	global upgrade_method
	global upgrade_name
	global valid_methods
	global test_names
	global parms
	global encrypt
	global passwd
	source ./include.tcl

	set upgrade_dir $dir
	env_cleanup $testdir

	fileremove -f GENERATE.OUT
	set o [open GENERATE.OUT a]

	puts -nonewline $o "Generating upgrade files.  Started at: "
	puts $o [clock format [clock seconds] -format "%H:%M %D"]
	puts $o [berkdb version -string]

	puts -nonewline "Generating upgrade files.  Started at: "
	puts [clock format [clock seconds] -format "%H:%M %D"]
	puts [berkdb version -string]

	close $o

	# Create a file that contains the log version number.
	# If necessary, create the directory to contain the file.
	set env [berkdb_env -create -log -home $testdir]
	error_check_good is_valid_env [is_valid_env $env] TRUE

	if { [file exists $dir] == 0 } {
		file mkdir $dir
	}
	set lv [open $dir/logversion w]
	puts $lv [get_log_vers $env]
	close $lv

	error_check_good env_close [$env close] 0

	# Generate test databases for each access method and endianness.
	foreach method $valid_methods {
		set o [open GENERATE.OUT a]
		puts $o "\nGenerating $method files"
		close $o
		puts "\tGenerating $method files"
		set upgrade_method $method

		# We piggyback testing of dumped sequence files on upgrade
		# testing because this is the only place that we ship files
		# from one machine to another.  Create files for both
		# endiannesses, because who knows what platform we'll
		# be testing on.

		set gen_dump 1
		foreach test $test_names(plat) {
			set upgrade_name $test
			foreach upgrade_be { 0 1 } {
				eval $test $method
				cleanup $testdir NULL
			}
		}
		set gen_dump 0

#set test_names(test) ""
		set gen_upgrade 1
		foreach test $test_names(test) {
			if { [info exists parms($test)] != 1 } {
				continue
			}

			set o [open GENERATE.OUT a]
			puts $o "\t\tGenerating files for $test"
			close $o
			puts "\t\tGenerating files for $test"

			if { $save_non_crypto == 1 } {
				set encrypt 0
				foreach upgrade_be { 0 1 } {
					set upgrade_name $test
					if [catch {exec $tclsh_path \
					    << "source $test_path/test.tcl;\
					    global gen_upgrade upgrade_be;\
					    global upgrade_method upgrade_name;\
					    global encrypt;\
					    set encrypt $encrypt;\
					    set gen_upgrade 1;\
					    set upgrade_be $upgrade_be;\
					    set upgrade_method $upgrade_method;\
					    set upgrade_name $upgrade_name;\
					    run_method -$method $test" \
					    >>& GENERATE.OUT} res] {
						puts "FAIL: run_method \
						    $test $method"
					}
					cleanup $testdir NULL 1
				}
				# Save checksummed files for only one test.
				# Checksumming should work in all or no cases.
				set gen_chksum 1
				foreach upgrade_be { 0 1 } {
					set upgrade_name $test
					if { $test == "test001" } {
						if { [catch {exec $tclsh_path \
						    << "source $test_path/test.tcl;\
						    global gen_upgrade;\
						    global upgrade_be;\
						    global upgrade_method;\
						    global upgrade_name;\
						    global encrypt gen_chksum;\
						    set encrypt $encrypt;\
						    set gen_upgrade 1;\
						    set gen_chksum 1;\
						    set upgrade_be $upgrade_be;\
						    set upgrade_method \
						    $upgrade_method;\
						    set upgrade_name \
						    $upgrade_name;\
						    run_method -$method $test \
						    0 1 stdout -chksum" \
						    >>& GENERATE.OUT} res] } {
							puts "FAIL: run_method \
							    $test $method \
							    -chksum: $res"
						}
						cleanup $testdir NULL 1
					}
				}
				set gen_chksum 0
			}
			# Save encrypted db's only of native endianness.
			# Encrypted files are not portable across endianness.
			if { $save_crypto == 1 } {
				set upgrade_be [big_endian]
				set encrypt 1
				set upgrade_name $test
				if [catch {exec $tclsh_path \
				    << "source $test_path/test.tcl;\
				    global gen_upgrade upgrade_be;\
				    global upgrade_method upgrade_name;\
				    global encrypt passwd;\
				    set encrypt $encrypt;\
				    set passwd $passwd;\
				    set gen_upgrade 1;\
				    set upgrade_be $upgrade_be;\
				    set upgrade_method $upgrade_method;\
				    set upgrade_name $upgrade_name;\
				    run_secmethod $method $test" \
				    >>& GENERATE.OUT} res] {
					puts "FAIL: run_secmethod \
					    $test $method"
				}
				cleanup $testdir NULL 1
			}
		}
		set gen_upgrade 0
	}

	# Set upgrade_be to the native value so log files go to the
	# right place.
	set upgrade_be [big_endian]

	# Generate log files.
	set o [open GENERATE.OUT a]
	puts $o "\tGenerating log files"
	close $o
	puts "\tGenerating log files"

	set gen_upgrade_log 1
	# Pass the global variables and their values to the new tclsh.
	if { $save_non_crypto == 1 } {
		set encrypt 0
		if [catch {exec $tclsh_path  << "source $test_path/test.tcl;\
		    global gen_upgrade_log upgrade_be upgrade_dir;\
		    global encrypt;\
		    set encrypt $encrypt;\
		    set gen_upgrade_log $gen_upgrade_log; \
		    set upgrade_be $upgrade_be;\
		    set upgrade_dir $upgrade_dir;\
		    run_recds" >>& GENERATE.OUT} res] {
			puts "FAIL: run_recds: $res"
		}
	}
	if { $save_crypto == 1 } {
		set encrypt 1
		if [catch {exec $tclsh_path  << "source $test_path/test.tcl;\
		    global gen_upgrade_log upgrade_be upgrade_dir;\
		    global encrypt;\
		    set encrypt $encrypt;\
		    set gen_upgrade_log $gen_upgrade_log; \
		    set upgrade_be $upgrade_be;\
		    set upgrade_dir $upgrade_dir;\
		    run_recds "  >>& GENERATE.OUT} res] {
			puts "FAIL: run_recds with crypto: $res"
		}
	}
	set gen_upgrade_log 0

	set o [open GENERATE.OUT a]
	puts -nonewline $o "Completed at: "
	puts $o [clock format [clock seconds] -format "%H:%M %D"]
	puts -nonewline "Completed at: "
	puts [clock format [clock seconds] -format "%H:%M %D"]
	close $o
}

proc save_upgrade_files { dir } {
	global upgrade_dir
	global upgrade_be
	global upgrade_method
	global upgrade_name
	global gen_upgrade
	global gen_upgrade_log
	global gen_dump
	global encrypt
	global gen_chksum
	global passwd
	source ./include.tcl

	set vers [berkdb version]
	set maj [lindex $vers 0]
	set min [lindex $vers 1]

	# Is this machine big or little endian?  We want to mark
	# the test directories appropriately, since testing
	# little-endian databases generated by a big-endian machine,
	# and/or vice versa, is interesting.
	if { [big_endian] } {
		set myendianness be
	} else {
		set myendianness le
	}

	if { $upgrade_be == 1 } {
		set version_dir "$myendianness-$maj.${min}be"
		set en be
	} else {
		set version_dir "$myendianness-$maj.${min}le"
		set en le
	}

	set dest $upgrade_dir/$version_dir/$upgrade_method
	exec mkdir -p $dest

	if { $gen_upgrade == 1 } {
		# Save db files from test001 - testxxx.
		set dbfiles [glob -nocomplain $dir/*.db]
		set dumpflag ""
		# Encrypted files are identified by the prefix "c-".
		if { $encrypt == 1 } {
			set upgrade_name c-$upgrade_name
			set dumpflag " -P $passwd "
		}
		# Checksummed files are identified by the prefix "s-".
		if { $gen_chksum == 1 } {
			set upgrade_name s-$upgrade_name
		}
		foreach dbfile $dbfiles {
			set basename [string range $dbfile \
			    [expr [string length $dir] + 1] end-3]

			set newbasename $upgrade_name-$basename

			# db_dump file
			if { [catch {eval exec $util_path/db_dump -k $dumpflag \
			    $dbfile > $dir/$newbasename.dump} res] } {
				puts "FAIL: $res"
			}

			# tcl_dump file
			upgrade_dump $dbfile $dir/$newbasename.tcldump

			# Rename dbfile and any dbq files.
			file rename $dbfile $dir/$newbasename-$en.db
			foreach dbq \
			    [glob -nocomplain $dir/__dbq.$basename.db.*] {
				set s [string length $dir/__dbq.]
				set newname [string replace $dbq $s \
				    [expr [string length $basename] + $s - 1] \
				    $newbasename-$en]
				file rename $dbq $newname
			}
			set cwd [pwd]
			cd $dir
			catch {eval exec tar -cvf $dest/$newbasename.tar \
			    [glob $newbasename* __dbq.$newbasename-$en.db.*]}
			catch {exec gzip -9v $dest/$newbasename.tar} res
			cd $cwd
		}
	}

	if { $gen_upgrade_log == 1 } {
		# Save log files from recd tests.
		set logfiles [glob -nocomplain $dir/log.*]
		if { [llength $logfiles] > 0 } {
			# More than one log.0000000001 file may be produced
			# per recd test, so we generate unique names:
			# recd001-0-log.0000000001, recd001-1-log.0000000001,
			# and so on.
			# We may also have log.0000000001, log.0000000002,
			# and so on, and they will all be dumped together
			# by db_printlog.
			set count 0
			while { [file exists \
			    $dest/$upgrade_name-$count-log.tar.gz] \
			    == 1 } {
				incr count
			}
			set newname $upgrade_name-$count-log

			# Run db_printlog on all the log files
			if {[catch {exec $util_path/db_printlog -h $dir > \
			    $dir/$newname.prlog} res] != 0} {
				puts "Regular printlog failed, try encryption"
				eval {exec $util_path/db_printlog} -h $dir \
				    -P $passwd > $dir/$newname.prlog
			}

			# Rename each log file so we can identify which
			# recd test created it.
			foreach logfile $logfiles {
				set lognum [string range $logfile \
				    end-9 end]
				file rename $logfile $dir/$newname.$lognum
			}

			set cwd [pwd]
			cd $dir

			catch {eval exec tar -cvf $dest/$newname.tar \
			    [glob $newname*]}
			catch {exec gzip -9v $dest/$newname.tar}
			cd $cwd
		}
	}

	if { $gen_dump == 1 } {
		# Save dump files.  We require that the files have
		# been created with the extension .dmp.
		set dumpfiles [glob -nocomplain $dir/*.dmp]

		foreach dumpfile $dumpfiles {
			set basename [string range $dumpfile \
			    [expr [string length $dir] + 1] end-4]

			set newbasename $upgrade_name-$basename

			# Rename dumpfile.
			file rename $dumpfile $dir/$newbasename.dmp

			set cwd [pwd]
			cd $dir
			catch {eval exec tar -cvf $dest/$newbasename.tar \
			    [glob $newbasename.dmp]}
			catch {exec gzip -9v $dest/$newbasename.tar} res
			cd $cwd
		}
	}
}

proc upgrade_dump { database file {stripnulls 0} } {
	global errorInfo
	global encrypt
	global passwd

	set encargs ""
	if { $encrypt == 1 } {
		set encargs " -encryptany $passwd "
	}
	set db [eval {berkdb open} -rdonly $encargs $database]
	set dbc [$db cursor]

	set f [open $file w+]
	fconfigure $f -encoding binary -translation binary

	#
	# Get a sorted list of keys
	#
	set key_list ""
	set pair [$dbc get -first]

	while { 1 } {
		if { [llength $pair] == 0 } {
			break
		}
		set k [lindex [lindex $pair 0] 0]
		lappend key_list $k
		set pair [$dbc get -next]
	}

	# Discard duplicated keys;  we now have a key for each
	# duplicate, not each unique key, and we don't want to get each
	# duplicate multiple times when we iterate over key_list.
	set uniq_keys ""
	foreach key $key_list {
		if { [info exists existence_list($key)] == 0 } {
			lappend uniq_keys $key
		}
		set existence_list($key) 1
	}
	set key_list $uniq_keys

	set key_list [lsort -command _comp $key_list]

	#
	# Get the data for each key
	#
	set i 0
	foreach key $key_list {
		set pair [$dbc get -set $key]
		if { $stripnulls != 0 } {
			# the Tcl interface to db versions before 3.X
			# added nulls at the end of all keys and data, so
			# we provide functionality to strip that out.
			set key [strip_null $key]
		}
		set data_list {}
		catch { while { [llength $pair] != 0 } {
			set data [lindex [lindex $pair 0] 1]
			if { $stripnulls != 0 } {
				set data [strip_null $data]
			}
			lappend data_list [list $data]
			set pair [$dbc get -nextdup]
		} }
		#lsort -command _comp data_list
		set data_list [lsort -command _comp $data_list]
		puts -nonewline $f [binary format i [string length $key]]
		puts -nonewline $f $key
		puts -nonewline $f [binary format i [llength $data_list]]
		for { set j 0 } { $j < [llength $data_list] } { incr j } {
			puts -nonewline $f [binary format i [string length \
			    [concat [lindex $data_list $j]]]]
			puts -nonewline $f [concat [lindex $data_list $j]]
		}
		if { [llength $data_list] == 0 } {
			puts "WARNING: zero-length data list"
		}
		incr i
	}

	close $f
	error_check_good upgrade_dump_c_close [$dbc close] 0
	error_check_good upgrade_dump_db_close [$db close] 0
}

proc _comp { a b } {
	if { 0 } {
	# XXX
		set a [strip_null [concat $a]]
		set b [strip_null [concat $b]]
		#return [expr [concat $a] < [concat $b]]
	} else {
		set an [string first "\0" $a]
		set bn [string first "\0" $b]

		if { $an != -1 } {
			set a [string range $a 0 [expr $an - 1]]
		}
		if { $bn != -1 } {
			set b [string range $b 0 [expr $bn - 1]]
		}
	}
	#puts "$a $b"
	return [string compare $a $b]
}

proc strip_null { str } {
	set len [string length $str]
	set last [expr $len - 1]

	set termchar [string range $str $last $last]
	if { [string compare $termchar \0] == 0 } {
		set ret [string range $str 0 [expr $last - 1]]
	} else {
		set ret $str
	}

	return $ret
}

proc get_log_vers { env } {
	set stat [$env log_stat]
	foreach pair $stat {
		set msg [lindex $pair 0]
		set val [lindex $pair 1]
		if { $msg == "Log file Version" } {
			return $val
		}
	}
	puts "FAIL: Log file Version not found in log_stat"
	return 0
}

