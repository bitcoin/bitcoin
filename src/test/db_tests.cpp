// Copyright (c) 2014-2018 Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dbmanager.h"
#include <boost/test/unit_test.hpp>
#include <fstream>

struct DbCleanupFixture
{
    DbCleanupFixture()
        : fileName("dbtest.dat")
    {
    }
    void RemoveFile() const
    {
        boost::filesystem::remove(GetDataDir() / fileName);
    }
    std::string fileName;
};

// Test class which has minimal required functions to use Load and Dump
struct DbTest
{
    DbTest()
        : m_testData(15)
        , m_checkedAndRemoved(false)
        , m_cleared(false)
    {
    }

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(m_testData);
    }
    void Clear()
    {
        m_cleared = true;
    }
    void CheckAndRemove()
    {
        m_checkedAndRemoved = true;
    }
    std::string ToString() const
    {
        return "DbTest";
    }
    bool operator == (const DbTest& dbTest)
    {
        return dbTest.m_testData == this->m_testData;
    }

    int m_testData;
    bool m_checkedAndRemoved;
    bool m_cleared;
};

// An exception is thrown during serialization to test 'IncorrectFormat' error
struct DbTestWithSerializeException : public DbTest
{
    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        throw std::runtime_error("Error");
    }
};

BOOST_FIXTURE_TEST_SUITE(db_tests, DbCleanupFixture)

    BOOST_AUTO_TEST_CASE(FileError)
    {
        // File doesn't exist
        DbTest db;
        ::details::ReadResult readResult = ::details::Read(db, fileName, "MagicMessage");
        BOOST_CHECK_EQUAL(readResult, ::details::FileError);
    }

    BOOST_AUTO_TEST_CASE(HashReadError)
    {
        DbTest db;

        // Create an empty file
        std::fstream fs;
        fs.open((GetDataDir() / fileName).string().c_str(), std::ios::out);
        fs.close();

        ::details::ReadResult readResult = ::details::Read(db, fileName, "MagicMessage");
        BOOST_CHECK_EQUAL(readResult, ::details::HashReadError);
        RemoveFile();
    }

    BOOST_AUTO_TEST_CASE(IncorrectMagicMessage)
    {
        DbTest db;
        Dump(db, fileName, "MagicMessage");
        
        // Try to read data with incorrect magic message
        ::details::ReadResult readResult = ::details::Read(db, fileName, "IncorrectMagicMessage");
        BOOST_CHECK_EQUAL(readResult, ::details::IncorrectMagicMessage);
        RemoveFile();
    }
    
    BOOST_AUTO_TEST_CASE(IncorrectMagicNumber)
    {
        DbTest db;
        Dump(db, fileName, "MagicMessage");
        const CBaseChainParams::Network prevParams = Params().NetworkID();
        // Change network params for magic number to be different
        SelectParams(CBaseChainParams::REGTEST);
        ::details::ReadResult readResult = ::details::Read(db, fileName, "MagicMessage");
        BOOST_CHECK_EQUAL(readResult, ::details::IncorrectMagicNumber);

        // Revert back network params
        SelectParams(prevParams);
        RemoveFile();
    }

    BOOST_AUTO_TEST_CASE(IncorrectFormat)
    {
        DbTest db;
        Dump(db, fileName, "MagicMessage");

        DbTestWithSerializeException dbWithException;
        ::details::ReadResult readResult = ::details::Read(dbWithException, fileName, "MagicMessage");
        BOOST_CHECK_EQUAL(readResult, ::details::IncorrectFormat);
        BOOST_CHECK(dbWithException.m_cleared);
        RemoveFile();
    }

    BOOST_AUTO_TEST_CASE(DbDumpAndLoad)
    {
        DbTest dbDump;
        BOOST_CHECK(Dump(dbDump, fileName, "MagicMessage"));

        DbTest dbLoad;
        BOOST_CHECK(Load(dbLoad, fileName, "MagicMessage"));

        BOOST_CHECK(dbDump == dbLoad);
        BOOST_CHECK(dbLoad.m_checkedAndRemoved);
        RemoveFile();
    }

BOOST_AUTO_TEST_SUITE_END()
