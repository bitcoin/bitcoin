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
    public class SecondaryHashDatabaseConfigTest
    {
        private string testFixtureHome;
        private string testFixtureName;
        private string testName;
        private string testHome;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "SecondaryHashDatabaseConfigTest";
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
            HashDatabaseConfig hashDBConfig =
                new HashDatabaseConfig();
            hashDBConfig.Creation = CreatePolicy.IF_NEEDED;
            HashDatabase hashDB = HashDatabase.Open(
                dbFileName, hashDBConfig);

            SecondaryHashDatabaseConfig secDBConfig =
                new SecondaryHashDatabaseConfig(hashDB, null);

            Config(xmlElem, ref secDBConfig, true);
            Confirm(xmlElem, secDBConfig, true);

            // Close the primary btree database.
            hashDB.Close();
        }

        public static void Confirm(XmlElement xmlElement,
            SecondaryHashDatabaseConfig secHashDBConfig,
            bool compulsory)
        {
            SecondaryDatabaseConfig secDBConfig =
                secHashDBConfig;
            SecondaryDatabaseConfigTest.Confirm(xmlElement,
                secDBConfig, compulsory);

            // Confirm secondary hash database specific configuration.
            Configuration.ConfirmCreatePolicy(xmlElement,
                "Creation", secHashDBConfig.Creation, compulsory);
            Configuration.ConfirmDuplicatesPolicy(xmlElement,
                "Duplicates", secHashDBConfig.Duplicates, compulsory);
            Configuration.ConfirmUint(xmlElement, "FillFactor",
                secHashDBConfig.FillFactor, compulsory);
            Configuration.ConfirmUint(xmlElement,
                "NumElements",
                secHashDBConfig.TableSize, compulsory);
        }

        public static void Config(XmlElement xmlElement,
            ref SecondaryHashDatabaseConfig secHashDBConfig,
            bool compulsory)
        {
            uint fillFactor = new uint();
            uint numElements = new uint();

            SecondaryDatabaseConfig secDBConfig = secHashDBConfig;
            SecondaryDatabaseConfigTest.Config(xmlElement,
                ref secDBConfig, compulsory);

            // Configure specific fields/properties of hash db
            Configuration.ConfigCreatePolicy(xmlElement,
                "Creation", ref secHashDBConfig.Creation, compulsory);
            Configuration.ConfigDuplicatesPolicy(xmlElement,
                "Duplicates", ref secHashDBConfig.Duplicates, compulsory);
            if (Configuration.ConfigUint(xmlElement, "FillFactor",
                 ref fillFactor, compulsory))
                secHashDBConfig.FillFactor = fillFactor;
            if (Configuration.ConfigUint(xmlElement, "NumElements",
                 ref numElements, compulsory))
                secHashDBConfig.TableSize = numElements;
        }
    }
}

