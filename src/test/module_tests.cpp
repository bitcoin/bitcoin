// Copyright (c) 2012-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "module.h"

#include <boost/test/unit_test.hpp>

using namespace std;

class AModule : public CModuleInterface
{
public:
    void appendTest(string& aString)
    {
        aString += " test";
    }
    
    void registerModule()
    {
        Module::Signals.AppendHelpMessage.connect(boost::bind(&AModule::appendTest, this, _1));
    }
    
    void unregisterModule()
    {
        Module::Signals.AppendHelpMessage.disconnect(boost::bind(&AModule::appendTest, this, _1));
    }
};

BOOST_AUTO_TEST_SUITE(module_tests)

BOOST_AUTO_TEST_CASE(module_basics)
{
    AModule *newModule = new AModule();
    Module::AddModule(newModule);
    
    string newStr("START");
    Module::Signals.AppendHelpMessage(newStr);
    BOOST_CHECK(newStr == "START test");
    
    Module::RemoveAllModules();
    
    newStr = string("");
    Module::Signals.AppendHelpMessage(newStr);
    BOOST_CHECK(newStr == "");
    
    delete newModule;
}

BOOST_AUTO_TEST_SUITE_END()
