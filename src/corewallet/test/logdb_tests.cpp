// Copyright (c) 2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base58.h"
#include "key.h"
#include "corewallet/logdb.h"
#include "random.h"
#include "util.h"
#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>

using namespace std;
static const string strSecret1     ("Kwr371tjA9u2rFSMZjTNun2PXXP3WPZu2afRHTcta6KxEUdm1vEw");
static const string strSecret2    ("L3Hq7a8FEQwJkW1M2GNKDW28546Vp5miewcCzSqUD9kCAXrJdS3g");

BOOST_FIXTURE_TEST_SUITE(logdb_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(logdb_test_1)
{
    boost::filesystem::path tmpPath = GetTempPath() / strprintf("test_bitcoin_logdb_%lu_%i.logdb", (unsigned long)GetTime(), (int)(GetRand(100000)));    
    std::string dbFile = tmpPath.string();
    
    
    LogPrintf("%s", dbFile);
    
    int max_loops = 1;
    
    for(int i = 0;i<max_loops;i++)
    {
        
        // simple value test
        CLogDB *aDB = new CLogDB(dbFile, false);
        aDB->Load();
        aDB->Write(std::string("testvalue"), std::string("aValue"));

        std::string returnString;
        aDB->Read(std::string("testvalue"), returnString);
        BOOST_CHECK_EQUAL(returnString, std::string("aValue"));
        
        aDB->Flush(true); //shutdown, close the file
        aDB->Close();
        delete aDB;
        
        aDB = new CLogDB(dbFile, false);
        aDB->Load();
        aDB->Read(std::string("testvalue"), returnString);
        BOOST_CHECK_EQUAL(returnString, std::string("aValue"));
        aDB->Write(std::string("testvalue"), std::string("aValue2"));
        aDB->Read(std::string("testvalue"), returnString);
        BOOST_CHECK_EQUAL(returnString, std::string("aValue2"));
        aDB->Flush(true); //shutdown, close the file
        aDB->Close();
        delete aDB;
        
        aDB = new CLogDB(dbFile, false);
        aDB->Load();
        aDB->Erase(std::string("testvalue"));
        
        std::string returnString2;
        aDB->Read(std::string("testvalue"), returnString2);
        BOOST_CHECK(returnString2 == "");
        aDB->Flush(true); //shutdown, close the file
        aDB->Close();
        delete aDB;

        // store a key
        CLogDB *aDB2 = new CLogDB(dbFile, false);
        aDB2->Load();
        aDB2->Read(std::string("testvalue"), returnString);
        BOOST_CHECK_EQUAL(returnString, std::string("aValue2"));
        
        CBitcoinSecret bsecret;
        BOOST_CHECK( bsecret.SetString (strSecret1));
        CKey key1  = bsecret.GetKey();
        CPubKey pKey = key1.GetPubKey();
        aDB2->Write(std::string("keyOverwriteTest"), pKey);
        aDB2->Flush(true); //shutdown, close the file
        aDB2->Close();
        delete aDB2;
        
        // overwrite existing key with a new pubkey
        aDB2 = new CLogDB(dbFile, false);
        aDB2->Load();
        bsecret.SetString(strSecret2);
        CKey key2  = bsecret.GetKey();
        CPubKey pKey2 = key2.GetPubKey();
        aDB2->Write(std::string("keyOverwriteTest"), pKey2);
        aDB2->Flush(true); //shutdown, close the file
        aDB2->Close();
        delete aDB2;

        // persistance and erase test
        aDB2 = new CLogDB(dbFile, false);
        aDB2->Load();
        CPubKey pKeyTest;
        aDB2->Read(std::string("keyOverwriteTest"), pKeyTest);
        
        BOOST_CHECK(pKey2 == pKeyTest);
        BOOST_CHECK(pKey != pKeyTest);
        BOOST_CHECK(pKeyTest.IsValid() == true);

        aDB2->Erase(std::string("keyOverwriteTest"));
        CPubKey pKeyTestEmpty;
        aDB2->Read(std::string("keyOverwriteTest"), pKeyTestEmpty);
        BOOST_CHECK(pKeyTestEmpty.IsValid() == false);
        aDB2->Flush(true); //shutdown, close the file
        aDB2->Close();
        delete aDB2;
        
        // check persistance of Erase
        aDB2 = new CLogDB(dbFile, false);
        aDB2->Load();
        CPubKey pKeyTestEmpty2;
        aDB2->Read(std::string("keyOverwriteTest"), pKeyTestEmpty2);
        BOOST_CHECK(pKeyTestEmpty2.IsValid() == false);
        
        //maxKeySize
        std::string longKey = std::string(999,'a');
        bool state = aDB2->Write(longKey, std::string("newValue"));
        BOOST_CHECK(state);
        aDB2->Flush(true); //shutdown, close the file
        aDB2->Close();
        delete aDB2;
        
        aDB2 = new CLogDB(dbFile, false);
        aDB2->Load();
        std::string newString;
        aDB2->Read(longKey, newString);
        BOOST_CHECK(newString == "newValue");
        aDB2->Flush(true); //shutdown, close the file
        
        std::string longKey2 = std::string(0x1001,'b');
        state = aDB2->Write(longKey2, std::string("newValue2"));
        BOOST_CHECK(state == false);
        
        std::string newString2;
        aDB2->Read(longKey2, newString2);
        BOOST_CHECK(newString2 == "");
        
        aDB2->Close();
        delete aDB2;
    }
}


BOOST_AUTO_TEST_CASE(logdb_test_rewrite)
{
    boost::filesystem::path tmpPath = GetTempPath() / strprintf("test_bitcoin_logdb_compact_%lu_%i.logdb", (unsigned long)GetTime(), (int)(GetRand(100000)));
    
    std::string dbFile = tmpPath.string();
    std::string dbFileRewritten = dbFile+".tmp";
    

    CLogDB *aDB = new CLogDB(dbFile, false);
    aDB->Load();
    aDB->Write(std::string("testvalue"), std::string("aValue"));
    aDB->Flush(true); //shutdown, close the file
    aDB->Close();
    delete aDB;
    
    aDB = new CLogDB(dbFile, false);
    aDB->Load();
    std::string returnString;
    aDB->Read(std::string("testvalue"), returnString);
    BOOST_CHECK_EQUAL(returnString, std::string("aValue"));
    aDB->Write(std::string("testvalue"), std::string("aValue2")); //overwrite value
    aDB->Read(std::string("testvalue"), returnString);
    BOOST_CHECK_EQUAL(returnString, std::string("aValue2"));
    
    aDB->Rewrite(dbFileRewritten); //this new db should only containe key "testvalue" once
    
    aDB->Flush(true); //shutdown, close the file
    aDB->Close();
    delete aDB;
    
    
    //open the new compact db and check of the value
    aDB = new CLogDB(dbFileRewritten, false);
    aDB->Load();
    aDB->Read(std::string("testvalue"), returnString);
    BOOST_CHECK_EQUAL(returnString, std::string("aValue2"));
    aDB->Close();
    delete aDB;

    
    // compare filesizes
    FILE *fh = fopen(dbFile.c_str(), "rb");
    fseek(fh, 0L, SEEK_END);
    size_t oldFileSize = ftell(fh);
    fclose(fh);
    
    fh = fopen(dbFileRewritten.c_str(), "rb");
    fseek(fh, 0L, SEEK_END);
    size_t newFileSize = ftell(fh);
    fclose(fh);
    
    BOOST_CHECK(newFileSize > (oldFileSize-8)/2.0); // file size must be half the size minus the ~8 header (depends on version int length) bytes
}

BOOST_AUTO_TEST_SUITE_END()
