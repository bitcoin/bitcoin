#!/usr/bin/env bash
#
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Check that all boost filesystem functions have necessary parameters, tests excluded

export LC_ALL=C

check_params () {
    RET_VAL=""
    EXPECTED=$(echo $2 | grep -o "," | tr -d "\n")
    GREP=$(git grep -n "$1(" -- "*.cpp")
    IFS=$'\n'

    for LINE in ${GREP}
    do
        # tests should not be checked
        if [[ ${LINE} != *"/test/"* ]]; then
            PARAMS=$(echo ${LINE} | grep -oP "(?<=$1\()([^|&{]+)(?=[|&{(\n)])")
            COUNT=$(echo ${PARAMS} | grep -o "," | tr -d "\n")
            if [[ $2 == *"fs::"* ]] && [[ ${PARAMS} != *"fs::"* ]]; then
                ${COUNT}=,${COUNT}
            fi
            if [[ ${#COUNT} -lt ${#EXPECTED} ]]; then
                RET_VAL+=${LINE}${IFS};
            fi
        fi
    done

    if [[ ${RET_VAL} != "" ]]; then
        echo "All calls to $1() should use error_code parameter"
        echo
        echo "${RET_VAL}"
        echo
        echo "Expected use:"
        echo "    boost::system::error_code ec;"
        echo "    retval = $1($2);"
        echo "    if (ec) {"
        echo "        ..."
        echo "    }"
        exit 1
    fi
}


check_params "fs::canonical" "path, ec"
#check_params "fs::canonical" "path, base, ec"

check_params "fs::copy" "from, to, ec"
check_params "fs::copy_directory" "from, to, ec"

# optional fs:: parameters can be detected
check_params "fs::copy_file" "pathFrom, pathTo[, fs::copy_option], ec"

check_params "fs::copy_symlink" "existingSymlink, newSymlink, ec"
check_params "fs::create_directories" "path, ec"
check_params "fs::create_directory" "path, ec"
check_params "fs::create_directory_symlink" "pathTo, newSymlink, ec"
check_params "fs::create_hardlink" "pathTo, newHardLink, ec"
check_params "fs::create_symlink" "pathTo, newSymlink, ec"

# at least error_code parameter
check_params "fs::current_path" "ec"
# path is mandatory if used standalone
check_params "  fs::current_path" "path, ec"

check_params " = fs::directory_iterator" "path, ec"

check_params "fs::exists" "path, ec"
check_params "fs::equivalent" "path1, path2, ec"
check_params "fs::file_size" "path, ec"
check_params "fs::hard_link_count" "path, ec"
check_params "fs::initial_path" "ec"
check_params "fs::is_directory" "path, ec"
check_params "fs::is_empty" "path, ec"
check_params "fs::is_other" "path, ec"
check_params "fs::is_regular_file" "path, ec"
check_params "fs::is_symlink" "path, ec"
check_params "fs::last_write_time" "path, new_time, ec"
check_params "fs::read_symlink" "path, ec"

# variable number of parameters
#check_params "fs::relative" "path, ec"
#check_params "fs::relative" "path, base, ec"

check_params "fs::remove" "path, ec"
check_params "fs::remove_all" "path, ec"
check_params "fs::rename" "from, to, ec"

check_params " = fs::recursive_directory_iterator" "path, ec"

check_params "fs::resize_file" "path, size, ec"
check_params "fs::space" "path, ec"
check_params "fs::status" "path, ec"
check_params "fs::symlink_status" "path, ec"
check_params "fs::system_complete" "path, ec"
check_params "fs::temp_directory_path" "ec"

# error code not necessary for default model
#check_params "fs::unique_path" "model, ec"

check_params "fs::weakly_canonical" "path, ec"
