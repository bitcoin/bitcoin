/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Xml;
using NUnit.Framework;
using BerkeleyDB;

namespace CsharpAPITest
{
    [TestFixture]
    public class BTreeDatabaseConfigTest : DatabaseConfigTest
    {
        private string testFixtureHome;
        private string testFixtureName;
        private string testName;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "BTreeDatabaseConfigTest";
            testFixtureHome = "./TestOut/" + testFixtureName;

            Configuration.ClearDir(testFixtureHome);
        }

        [Test]
        override public void TestConfigWithoutEnv()
        {
            testName = "TestConfigWithoutEnv";

            XmlElement xmlElem = Configuration.TestSetUp(
                testFixtureName, testName);
            BTreeDatabaseConfig btreeConfig =
                new BTreeDatabaseConfig();
            Config(xmlElem, ref btreeConfig, true);
            Confirm(xmlElem, btreeConfig, true);
        }

        public static void Confirm(XmlElement
            xmlElement, BTreeDatabaseConfig btreeDBConfig,
            bool compulsory)
        {
            DatabaseConfig dbConfig = btreeDBConfig;
            Confirm(xmlElement, dbConfig, compulsory);

            // Confirm Btree database specific configuration
            Configuration.ConfirmDuplicatesPolicy(xmlElement,
                "Duplicates", btreeDBConfig.Duplicates, compulsory);
            Configuration.ConfirmBool(xmlElement,
                "NoReverseSplitting",
                btreeDBConfig.NoReverseSplitting, compulsory);
            Configuration.ConfirmBool(xmlElement,
                "UseRecordNumbers", btreeDBConfig.UseRecordNumbers,
                compulsory);
            Configuration.ConfirmCreatePolicy(xmlElement,
                "Creation", btreeDBConfig.Creation, compulsory);
            Configuration.ConfirmUint(xmlElement, "MinKeysPerPage",
                btreeDBConfig.MinKeysPerPage, compulsory);
        }

        public static void Config(XmlElement xmlElement,
            ref BTreeDatabaseConfig btreeDBConfig, bool compulsory)
        {
            uint minKeysPerPage = new uint();
            DatabaseConfig dbConfig = btreeDBConfig;
            Config(xmlElement, ref dbConfig, compulsory);

            // Configure specific fields/properties of Btree db
            Configuration.ConfigDuplicatesPolicy(xmlElement,
                "Duplicates", ref btreeDBConfig.Duplicates,
                compulsory);
            Configuration.ConfigBool(xmlElement,
                "NoReverseSplitting",
                ref btreeDBConfig.NoReverseSplitting, compulsory);
            Configuration.ConfigBool(xmlElement,
                "UseRecordNumbers",
                ref btreeDBConfig.UseRecordNumbers, compulsory);
            Configuration.ConfigCreatePolicy(xmlElement,
                "Creation", ref btreeDBConfig.Creation, compulsory);
            if (Configuration.ConfigUint(xmlElement,
                "MinKeysPerPage", ref minKeysPerPage, compulsory))
                btreeDBConfig.MinKeysPerPage = minKeysPerPage;
        }
    }
}
