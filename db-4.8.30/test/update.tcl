# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$

source ./include.tcl
global update_dir
set update_dir "$test_path/update_test"

proc update { } {
	source ./include.tcl
	global update_dir

	foreach version [glob $update_dir/*] {
		regexp \[^\/\]*$ $version version
		foreach method [glob $update_dir/$version/*] {
			regexp \[^\/\]*$ $method method
			foreach file [glob $update_dir/$version/$method/*] {
				regexp (\[^\/\]*)\.tar\.gz$ $file dummy name
				foreach endianness {"le" "be"} {
					puts "Update:\
					    $version $method $name $endianness"
					set ret [catch {_update $update_dir $testdir $version $method $name $endianness 1 1} message]
					if { $ret != 0 } {
						puts $message
					}
				}
			}
		}
	}
}

proc _update { source_dir temp_dir \
    version method file endianness do_db_load_test do_update_test } {
	source include.tcl
	global errorInfo

	cleanup $temp_dir NULL

	exec sh -c \
"gzcat $source_dir/$version/$method/$file.tar.gz | (cd $temp_dir && tar xf -)"

	if { $do_db_load_test } {
		set ret [catch \
		    {exec $util_path/db_load -f "$temp_dir/$file.dump" \
		    "$temp_dir/update.db"} message]
		error_check_good \
		    "Update load: $version $method $file $message" $ret 0

		set ret [catch \
		    {exec $util_path/db_dump -f "$temp_dir/update.dump" \
		    "$temp_dir/update.db"} message]
		error_check_good \
		    "Update dump: $version $method $file $message" $ret 0

		error_check_good "Update diff.1.1: $version $method $file" \
		    [filecmp "$temp_dir/$file.dump" "$temp_dir/update.dump"] 0
		error_check_good \
		    "Update diff.1.2: $version $method $file" $ret ""
	}

	if { $do_update_test } {
		set ret [catch \
		    {berkdb open -update "$temp_dir/$file-$endianness.db"} db]
		if { $ret == 1 } {
			if { ![is_substr $errorInfo "version upgrade"] } {
				set fnl [string first "\n" $errorInfo]
				set theError \
				    [string range $errorInfo 0 [expr $fnl - 1]]
				error $theError
			}
		} else {
			error_check_good dbopen [is_valid_db $db] TRUE
			error_check_good dbclose [$db close] 0

			set ret [catch \
			    {exec $util_path/db_dump -f \
			    "$temp_dir/update.dump" \
			    "$temp_dir/$file-$endianness.db"} message]
			error_check_good "Update\
			    dump: $version $method $file $message" $ret 0

			error_check_good \
			    "Update diff.2: $version $method $file" \
			    [filecmp "$temp_dir/$file.dump" \
			    "$temp_dir/update.dump"] 0
			error_check_good \
			    "Update diff.2: $version $method $file" $ret ""
		}
	}
}
