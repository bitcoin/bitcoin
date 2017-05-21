// Copyright (c) 2017 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <inttypes.h>
#include <string>
#include <boost/algorithm/string.hpp>

#include "fast-cpp-csv-parser/csv.h"

#include "chainparamsbase.h"
#include "forks_csv.h"
#include "util.h"


using namespace std;
using namespace io;

// bip135 begin
/** File header for -dumpforks output */
const char *FORKS_CSV_FILE_HEADER = \
    "# forks.csv - Fork deployment configuration (from built-in defaults)\n" \
    "# This file defines the known consensus changes tracked by the software\n" \
    "# MODIFY AT OWN RISK - EXERCISE EXTREME CARE\n" \
    "# Line format:\n" \
    "# network,bit,name,starttime,timeout,windowsize,threshold,minlockedblocks,minlockedtime,gbtforce\n";
// bip135 end

/**
 * Read deployment CSV file and update consensus parameters
 */
bool ReadForksCsv(string activeNetworkID, istream& csvInput, Consensus::Params& consensusParams)
{
    string network, name, gbtforce;
    int bit, windowsize, threshold, minlockedblocks;
    int64_t starttime, timeout, minlockedtime;
    bool errors_found = false;

    // set up CSV reader instance
    CSVReader<10,
              trim_chars<' ', '\t'>,
              double_quote_escape<',','\"'>,
              throw_on_overflow,
              single_line_comment<'#', ';'> > in(FORKS_CSV_FILENAME, csvInput);

    // set meaning of fields - header in file should remain commented out
    in.set_header("network",
                  "bit",
                  "name",
                  "starttime",
                  "timeout",
                  "windowsize",
                  "threshold",
                  "minlockedblocks",
                  "minlockedtime",
                  "gbtforce");

    // read lines, skipping comments
    try {
        while(in.read_row(network,
                          bit,
                          name,
                          starttime,
                          timeout,
                          windowsize,
                          threshold,
                          minlockedblocks,
                          minlockedtime,
                          gbtforce))
        {

            bool line_validates_ok = true;
            ostringstream errStringStream;
            // prepare an error message prefix string
            errStringStream << "Error: " << FORKS_CSV_FILENAME << ":" << in.get_file_line() << " ";

            // validate all the input fields (if there was an error the parser
            // would throw an exception that is handled below
            if (!ValidateNetwork(network)) {
                string errStr = errStringStream.str() + "unknown network name '" + network + "'";
                LineValidationError(errStr);
                line_validates_ok = false;
            }

            if (!ValidateBit(bit)) {
                string errStr = errStringStream.str() + "invalid bit number (" + to_string(bit) + ")";
                LineValidationError(errStr);
                line_validates_ok = false;
            }

            if (!ValidateForkName(name)) {
                string errStr = errStringStream.str() + "invalid fork name '" + name + "'";
                LineValidationError(errStr);
                line_validates_ok = false;
            }

            if (!ValidateTimes(starttime, timeout)) {
                string errStr = errStringStream.str() + "invalid starttime/timeout";
                LineValidationError(errStr);
                line_validates_ok = false;
            }

            if (!ValidateWindowSize(windowsize)) {
                string errStr = errStringStream.str() + "invalid window size ("+ to_string(windowsize) + ")";
                LineValidationError(errStr);
                line_validates_ok = false;
            }

            if (!ValidateThreshold(threshold, windowsize)) {
                string errStr = errStringStream.str() + "invalid threshold ("+ to_string(threshold) + ")";
                LineValidationError(errStr);
                line_validates_ok = false;
            }

            if (!line_validates_ok) {
                // set errors and skip to next line if current one did not validate.
                errors_found = true;
                continue;
            }
            else {
                // Can do more overall validation of the line data here.
                // This is not yet implemented.
            }

            // update the fork parameters only if matching the active network
            // the rest are validated, but not activated.
            // NOTE: ValidateOverallParams() is stubbed.
            if (network == activeNetworkID && ValidateOverallParams(network)) {
                assert(bit >= 0 && bit < VERSIONBITS_NUM_BITS);
                // update deployment params for current network
                BIP9DeploymentInfo& vbinfo = VersionBitsDeploymentInfo[bit];

                // copy the data
                if (name.length()) {
                    char* c = new char[name.length() + 1];
                    strncpy(c, name.c_str(), name.length());
                    c[name.length()] = 0;
                    vbinfo.name = c;
                }
                consensusParams.vDeployments[bit].bit = bit;
                consensusParams.vDeployments[bit].nStartTime = starttime;
                consensusParams.vDeployments[bit].nTimeout = timeout;
                consensusParams.vDeployments[bit].windowsize = windowsize;
                consensusParams.vDeployments[bit].threshold = threshold;
                consensusParams.vDeployments[bit].minlockedblocks = minlockedblocks;
                consensusParams.vDeployments[bit].minlockedtime = minlockedtime;
            }
        }
    }
    catch (const std::exception& e) {
        string errStr = e.what();
        LineValidationError(errStr);
        return false;
    }
    return (!errors_found);
}


bool ValidateNetwork(const string& networkname)
{
    // check that network is one we know about
    if (networkname == CBaseChainParams::MAIN)
        return true;
    else if (networkname == CBaseChainParams::TESTNET)
        return true;
    else if (networkname == CBaseChainParams::REGTEST)
        return true;
    else
        return false;
}


bool ValidateForkName(const string& forkname)
{
    return (forkname.length() > 0);
}


bool ValidateGBTForce(const string& gbtforce)
{
    string gbtforceLower(gbtforce);
    boost::algorithm::to_lower(gbtforceLower);
    if (gbtforceLower == "false" || gbtforceLower == "true")
        return true;
    else
        return false;
}


bool ValidateBit(int bit)
{
    return (bit >= 0 && bit < VERSIONBITS_NUM_BITS);
}


bool ValidateWindowSize(int windowsize)
{
    return (windowsize > 1);
}


bool ValidateThreshold(int threshold, int window)
{
    return (ValidateWindowSize(window) && threshold > 0 && threshold <= window);
}


bool ValidateTimes(int64_t starttime, int64_t timeout)
{
    return (starttime >= 0 && starttime < timeout);
}


bool ValidateMinLockedBlocks(int minlockedblocks)
{
    return (minlockedblocks >= 0);
}


bool ValidateMinLockedTime(int64_t minlockedtime)
{
    return (minlockedtime >= 0);
}


bool ValidateOverallParams(const string& checkNetworkID)
{
    // Stubbed function which should cross-check all parameters for the
    // given network and return true only if they are correct and consistent.
    //
    // This includes checks like duplicate bits or fork names, and disjointness
    // of time ranges of forks.
    return true;
}


void LineValidationError(string errmsg)
{
    LogPrintf("%s\n", errmsg);
}
