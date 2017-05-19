#!/bin/bash
#########################################################################
# helper script to bisect probabilistic test failures which can loop
# based on https://github.com/ftrader/bitcoin-misc-contrib/blob/master/bisection-tools/bisect_probabilistic_with_timeout.sh
# relicensed by freetrader under the MIT software license (see below)
#########################################################################
# Copyright (c) 2017 The Bitcoin Unlimited developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#########################################################################
# Usage:  bisect.sh [iterations] [timeout] [test_cmd]
#
# The optional parameters currently are:
#
# - iterations: the test command is repeated this many times - if it
#               passes all iterations, the test is considered truly passed.
#               This feature is for tests which fail randomly.
#               You can set this to 1 if you are sure the test will not
#               fail randomly.
# - timeout:    If a test iteration takes this long, it is terminated and
#               the test is considered failed.
# - test_cmd:   The test command. Use double quotes if multiple args.
#
# These are equivalent to the STOP_ITERATIONS, ITERATION_TIMEOUT_SECS
# and TEST_COMMAND variables which you can also set in the script below.
#
# There are some other parameters which you can set when you do a
# bisection. They are all described by comments, but this usage will briefly
# explain.
#
# If you are not passing the parameters in to this script, you need to
# set TEST_COMMAND first and foremost.
#
# If you might be bisecting across build system changes, you should set
# RECONFIGURE_BETWEEN_RUNS and set the appropriate autogen / configure
# commands in the do_configure() function which gets called prior to
# running 'make'.  Otherwise, only 'make' will be called between runs.
#
# NUM_CORES_TO_USE will passed to 'make -j' to speed up builds, and
# to test runners which support parallel execution (if it is detected
# that TEST_COMMAND is amenable to parallel testing).
#
# It is advised that you always call this script from the root of the
# source tree, but call a copy outside the tree when you use 'git bisect run'.
#
#########################################################################
# Limitations, Caveats etc.
#
# 1. CAUTION!!!! This kills any running bitcoind's as part of cleanup!
#    Only run in an isolated test environment!
#
# 2. Once adapted for a bisection run, you should copy the modified script
#    to a place outside of your working area, since otherwise it might
#    disappear during the run :-)
#
# 3. You may need to adapt PARALLEL_TEST_TOOL. This tool should also be
#    placed outside the working area, to protect it during runs.
#
#########################################################################

# using coreutils timeout(1) to abort jobs which are deemed to loop
TIMEOUT_CMD=/usr/bin/timeout

# check that we have the timeout command, else abort with code 2
[ -x "${TIMEOUT_CMD}" ] || {
    echo "Error: this script needs ${TIMEOUT_CMD}"
    exit 2
}

# optional tool for use with "test_bitcoin" tests
# You should find a copy in contrib/testtools/ , and copy it to
# somewhere in your PATH.
# If not present during running of this script, unit tests will be done
# sequentially.
PARALLEL_TEST_TOOL=gtest-parallel-bitcoin


############ Test run specific configuration ##########

# number of cores to use for building and parallel testing
NUM_CORES_TO_USE=1

# should we do autogen + configure before each build ?
# non-zero value means yes
RECONFIGURE_BETWEEN_RUNS=0

# if non-zero, remove the cache/ folder before each iteration
CLEAR_CACHE_BETWEEN_RUNS=1

# the command to test
# if not specified as $3, use the default below
TEST_COMMAND=${3:-"src/test/test_bitcoin"}
#TEST_COMMAND=${3:-"qa/pull-tester/rpc-tests.py txn_doublespend.py --mineblock"}

# number of iterations after which to stop and consider the test as passed
STOP_ITERATIONS=${1:-10}

# timeouts (in seconds) for one iteration
# if it takes longer than this, test is considered failed
ITERATION_TIMEOUT_SECS=${2:-1800}


# helper function to check if parameter is some form of test_bitcoin
# this function has been dumbed down for now.
is_test_bitcoin()
{
    if [ "$1" = "src/test/test_bitcoin" ]
    then
        return 0
    else
        return 1
    fi
}


############ Platform specific configure instruction ##########

# helper function which does the configure according to what user can do.
# Cannot code a one-size-fits-all-test-platforms system, so you need to
# modify this according to the system you are testing on.
# This function should abort the script with exit code 1 if there is
# problem during pre-build configuration.
do_configure()
{
    echo "Error: you have not set up how to configure on this system!"
    echo "You need to adapt the do_configure() function in the script."
    exit 1

    # for example:

    # sh autogen.sh || exit 1
    # ./configure || exit 1
}


############ beginning of actual test steps ############

# starting assumption is that git has just checked out a revision to test,
# and that our working area still contains build products from the test of
# the previous revision in the bisection.


# clear the build
echo "cleaning up from previous run..."
rm -rf cache
make clean


# if the reconfiguration has been enabled for this bisection, do it
if [ $RECONFIGURE_BETWEEN_RUNS -ne 0 ]
then
    # it is user's responsibility to ensure that the function below
    # does the right thing to configure for the build.

    # by default, this is stubbed to abort the run with an error message.
    do_configure
fi


# either way, must do a build now before we can test
echo "building..."
make -j ${NUM_CORES_TO_USE} || {
    echo "Build failed - the test could not be assessed."
    exit 1
}

# start the test runs...
echo "testing for at most ${STOP_ITERATIONS} iterations"
echo "timeout is ${ITERATION_TIMEOUT_SECS} seconds per run..."
iteration=0
while :;
do
    iteration=$((++iteration))
    echo "`date`: iteration ${iteration}"

    # clear cache if necessary
    if [ $CLEAR_CACHE_BETWEEN_RUNS -ne 0 ]
    then
        rm -rf cache
    fi

    # check if the test is src/test/test_bitcoin
    is_test_bitcoin "${TEST_COMMAND}"
    if [ $? -eq 0 ]
    then
        # check if we have parallel testing tool
        if [ -x "${PARALLEL_TEST_TOOL}" ]
        then
            # check if it is a unit test bisection, then do parallel if possible
            timeout --foreground ${ITERATION_TIMEOUT_SECS} \
                    ${PARALLEL_TEST_TOOL} -w ${NUM_CORES_TO_USE} \
                    "${TEST_COMMAND}"
        else
            # no parallel execution
            timeout --foreground ${ITERATION_TIMEOUT_SECS} ${TEST_COMMAND}
        fi
    else
        timeout --foreground ${ITERATION_TIMEOUT_SECS} ${TEST_COMMAND}
    fi
    run_exit_code=$?
    if [ ${run_exit_code} -ne 0 ]
    then
        if [ ${run_exit_code} -ne 124 ]
        then
            killall bitcoind
            echo "failed during iteration ${iteration} with exit code: ${run_exit_code}"
            exit 1
        else
            # timed out
            killall bitcoind
            echo "timed out after ${ITERATION_TIMEOUT_SECS} seconds during iteration ${iteration}"
            echo "check if you need to adjust the timeout to be higher!"
            exit 1
        fi
    fi
    if [ ${iteration} -eq ${STOP_ITERATIONS} ]
    then
        echo "accepted test as passed after ${STOP_ITERATIONS} successful iterations"
        exit 0
    fi
done
