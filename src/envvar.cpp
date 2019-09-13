// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <envvar.h>

void InstallEnvVars(ArgsManager& am)
{
    // Append other envvars here
    std::vector<std::string> vars = {
        "datadir",
        "conf"
    };

    for (unsigned int i = 0; i < vars.size(); ++i)
    {
        std::string element = "BITCOIN_";   // Prefix
        for (auto &c: vars[i]) {
            element += toupper(c);
        }
        const char *envvar = getenv(element.c_str());
        if (envvar != NULL)
        {
            if (!gArgs.IsArgSet("-" + vars[i])) {
                LogPrintf("Using environment variable %s=%s\n", element, envvar);
                am.SoftSetArg("-" + vars[i], std::string(envvar));
            }
        }
    }
}
