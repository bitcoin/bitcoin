/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 */
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Threading;
using System.Xml;
using NUnit.Framework;
using BerkeleyDB;

namespace CsharpAPITest
{
    [TestFixture]
    public class SecondaryBTreeDatabaseTest
    {
        private string testFixtureHome;
        private string testFixtureName;
        private string testName;
        private string testHome;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "SecondaryBTreeDatabaseTest";
            testFixtureHome = "./TestOut/" + testFixtureName;

            Configuration.ClearDir(testFixtureHome);
        }

        [Test]
        public void TestOpen()
        {
            testName = "TestOpen";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            string dbSecFileName = testHome + "/" +
                testName + "_sec.db";

            Configuration.ClearDir(testHome);

            XmlElement xmlElem = Configuration.TestSetUp(
                testFixtureName, testName);

            // Open a primary btree database.
            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.IF_NEEDED;
            BTreeDatabase btreeDB = BTreeDatabase.Open(
                dbFileName, btreeDBConfig);

            // Open a secondary btree database.
            SecondaryBTreeDatabaseConfig secDBConfig =
                new SecondaryBTreeDatabaseConfig(btreeDB, null);

            SecondaryBTreeDatabaseConfigTest.Config(xmlElem,
                ref secDBConfig, true);
            SecondaryBTreeDatabase secDB =
               SecondaryBTreeDatabase.Open(dbSecFileName,
               secDBConfig);

            // Confirm its flags configured in secDBConfig.
            Confirm(xmlElem, secDB, true);

            secDB.Close();
            btreeDB.Close();
        }

        [Test]
        public void TestCompare()
        {
            testName = "TestCompare";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";

            Configuration.ClearDir(testHome);

            // Open a primary btree database.
            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.ALWAYS;
            BTreeDatabase btreeDB = BTreeDatabase.Open(
                dbFileName, btreeDBConfig);

            // Open a secondary btree database. 
            SecondaryBTreeDatabaseConfig secBtreeDBConfig =
                new SecondaryBTreeDatabaseConfig(null, null);
            secBtreeDBConfig.Primary = btreeDB;
            secBtreeDBConfig.Compare =
                new EntryComparisonDelegate(
                SecondaryEntryComparison);
            secBtreeDBConfig.KeyGen =
                new SecondaryKeyGenDelegate(SecondaryKeyGen);
            SecondaryBTreeDatabase secDB =
                SecondaryBTreeDatabase.Open(
                dbFileName, secBtreeDBConfig);

            /*
             * Get the compare function set in the configuration 
             * and run it in a comparison to see if it is alright.
             */
            EntryComparisonDelegate cmp =
                secDB.Compare;
            DatabaseEntry dbt1, dbt2;
            dbt1 = new DatabaseEntry(
                BitConverter.GetBytes((int)257));
            dbt2 = new DatabaseEntry(
                BitConverter.GetBytes((int)255));
            Assert.Less(0, cmp(dbt1, dbt2));

            secDB.Close();
            btreeDB.Close();
        }

        [Test]
        public void TestDuplicates()
        {
            testName = "TestDuplicates";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            string dbSecFileName = testHome + "/" + testName
                + "_sec.db";

            Configuration.ClearDir(testHome);

            // Open a primary btree database.
            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.IF_NEEDED;
            BTreeDatabase btreeDB = BTreeDatabase.Open(
                dbFileName, btreeDBConfig);

            // Open a secondary btree database. 
            SecondaryBTreeDatabaseConfig secBtreeDBConfig =
                new SecondaryBTreeDatabaseConfig(btreeDB, null);
            secBtreeDBConfig.Creation = CreatePolicy.IF_NEEDED;
            secBtreeDBConfig.Duplicates = DuplicatesPolicy.SORTED;
            SecondaryBTreeDatabase secDB =
                SecondaryBTreeDatabase.Open(dbSecFileName,
                secBtreeDBConfig);

            secDB.Close();
            btreeDB.Close();
        }

        [Test]
        public void TestPrefixCompare()
        {
            testName = "TestPrefixCompare";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";

            Configuration.ClearDir(testHome);

            // Open a primary btree database.
            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.ALWAYS;
            BTreeDatabase btreeDB = BTreeDatabase.Open(
                dbFileName, btreeDBConfig);

            // Open a secondary btree database. 
            SecondaryBTreeDatabaseConfig secBtreeDBConfig =
                new SecondaryBTreeDatabaseConfig(btreeDB, null);
            secBtreeDBConfig.Primary = btreeDB;
            secBtreeDBConfig.Compare =
                new EntryComparisonDelegate(
                SecondaryEntryComparison);
            secBtreeDBConfig.PrefixCompare =
                new EntryComparisonDelegate(
                SecondaryEntryComparison);
            secBtreeDBConfig.KeyGen =
                new SecondaryKeyGenDelegate(
                SecondaryKeyGen);
            SecondaryBTreeDatabase secDB =
                SecondaryBTreeDatabase.Open(
                dbFileName, secBtreeDBConfig);

            /*
             * Get the prefix compare function set in the 
             * configuration and run it in a comparison to 
             * see if it is alright.
             */
            EntryComparisonDelegate cmp =
                secDB.PrefixCompare;
            DatabaseEntry dbt1, dbt2;
            dbt1 = new DatabaseEntry(
                BitConverter.GetBytes((int)1));
            dbt2 = new DatabaseEntry(
                BitConverter.GetBytes((int)129));
            Assert.Greater(0, cmp(dbt1, dbt2));

            secDB.Close();
            btreeDB.Close();
        }

        public int SecondaryEntryComparison(
            DatabaseEntry dbt1, DatabaseEntry dbt2)
        {
            int a, b;
            a = BitConverter.ToInt32(dbt1.Data, 0);
            b = BitConverter.ToInt32(dbt2.Data, 0);
            return a - b;
        }

        public DatabaseEntry SecondaryKeyGen(
            DatabaseEntry key, DatabaseEntry data)
        {
            DatabaseEntry dbtGen;
            dbtGen = new DatabaseEntry(data.Data);
            return dbtGen;
        }

        public static void Confirm(XmlElement xmlElem,
            SecondaryBTreeDatabase secDB, bool compulsory)
        {
            Configuration.ConfirmDuplicatesPolicy(xmlElem,
                "Duplicates", secDB.Duplicates, compulsory);
            Configuration.ConfirmUint(xmlElem, "MinKeysPerPage",
                secDB.MinKeysPerPage, compulsory);
            Configuration.ConfirmBool(xmlElem, "NoReverseSplitting",
                secDB.ReverseSplit, compulsory);
            Configuration.ConfirmBool(xmlElem, "UseRecordNumbers",
                secDB.RecordNumbers, compulsory);
        }
    }
}
