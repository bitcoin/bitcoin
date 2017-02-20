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
using System.Xml;
using NUnit.Framework;
using BerkeleyDB;

namespace CsharpAPITest
{
    [TestFixture]
    public class SecondaryBTreeDatabaseConfigTest
    {
        private string testFixtureHome;
        private string testFixtureName;
        private string testName;
        private string testHome;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "SecondaryBTreeDatabaseConfigTest";
            testFixtureHome = "./TestOut/" + testFixtureName;

            Configuration.ClearDir(testFixtureHome);
        }

        [Test]
        public void TestConfig()
        {
            testName = "TestConfig";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";

            Configuration.ClearDir(testHome);
            XmlElement xmlElem = Configuration.TestSetUp(
                testFixtureName, testName);

            // Open a primary btree database.
            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.IF_NEEDED;
            BTreeDatabase btreeDB = BTreeDatabase.Open(
                dbFileName, btreeDBConfig);

            SecondaryBTreeDatabaseConfig secDBConfig =
                new SecondaryBTreeDatabaseConfig(btreeDB, null);

            Config(xmlElem, ref secDBConfig, true);
            Confirm(xmlElem, secDBConfig, true);

            // Close the primary btree database.
            btreeDB.Close();
        }

        public static void Confirm(XmlElement xmlElement,
            SecondaryBTreeDatabaseConfig secBtreeDBConfig,
            bool compulsory)
        {
            SecondaryDatabaseConfig secDBConfig =
                secBtreeDBConfig;
            SecondaryDatabaseConfigTest.Confirm(xmlElement,
                secDBConfig, compulsory);

            // Confirm secondary btree database specific configuration.
            Configuration.ConfirmCreatePolicy(xmlElement,
                "Creation", secBtreeDBConfig.Creation, compulsory);
            Configuration.ConfirmDuplicatesPolicy(xmlElement,
                "Duplicates", secBtreeDBConfig.Duplicates, compulsory);
            Configuration.ConfirmUint(xmlElement, "MinKeysPerPage",
                secBtreeDBConfig.MinKeysPerPage, compulsory);
            Configuration.ConfirmBool(xmlElement,
                "NoReverseSplitting",
                secBtreeDBConfig.NoReverseSplitting, compulsory);
            Configuration.ConfirmBool(xmlElement,
                "UseRecordNumbers",
                secBtreeDBConfig.UseRecordNumbers,
                compulsory);
        }

        public static void Config(XmlElement xmlElement,
            ref SecondaryBTreeDatabaseConfig secBtreeDBConfig,
            bool compulsory)
        {
            uint minKeysPerPage = new uint();

            SecondaryDatabaseConfig secDBConfig = secBtreeDBConfig;
            SecondaryDatabaseConfigTest.Config(xmlElement,
                ref secDBConfig, compulsory);

            // Configure specific fields/properties of Btree db
            Configuration.ConfigCreatePolicy(xmlElement,
                "Creation", ref secBtreeDBConfig.Creation, compulsory);
            Configuration.ConfigDuplicatesPolicy(xmlElement,
                "Duplicates", ref secBtreeDBConfig.Duplicates,
                compulsory);
            if (Configuration.ConfigUint(xmlElement,
                "MinKeysPerPage", ref minKeysPerPage, compulsory))
                secBtreeDBConfig.MinKeysPerPage = minKeysPerPage;
            Configuration.ConfigBool(xmlElement,
                "NoReverseSplitting",
                ref secBtreeDBConfig.NoReverseSplitting, compulsory);
            Configuration.ConfigBool(xmlElement,
                "UseRecordNumbers",
                ref secBtreeDBConfig.UseRecordNumbers, compulsory);
        }
    }
}
