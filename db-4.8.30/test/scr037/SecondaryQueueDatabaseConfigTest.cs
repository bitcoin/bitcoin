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
    public class SecondaryQueueDatabaseConfigTest
    {
        private string testFixtureHome;
        private string testFixtureName;
        private string testName;
        private string testHome;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "SecondaryQueueDatabaseConfigTest";
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
            QueueDatabaseConfig queueDBConfig =
                new QueueDatabaseConfig();
            queueDBConfig.Creation = CreatePolicy.IF_NEEDED;
            QueueDatabase queueDB = QueueDatabase.Open(
                dbFileName, queueDBConfig);

            SecondaryQueueDatabaseConfig secDBConfig =
                new SecondaryQueueDatabaseConfig(queueDB, null);

            Config(xmlElem, ref secDBConfig, true);
            Confirm(xmlElem, secDBConfig, true);

            // Close the primary btree database.
            queueDB.Close();
        }

        public static void Confirm(XmlElement xmlElement,
            SecondaryQueueDatabaseConfig secQueueDBConfig,
            bool compulsory)
        {
            SecondaryDatabaseConfig secDBConfig =
                secQueueDBConfig;
            SecondaryDatabaseConfigTest.Confirm(xmlElement,
                secDBConfig, compulsory);

            // Confirm secondary hash database specific configuration.
            Configuration.ConfirmCreatePolicy(xmlElement,
                "Creation", secQueueDBConfig.Creation, compulsory);
            Configuration.ConfirmUint(xmlElement,
                "ExtentSize", secQueueDBConfig.ExtentSize, compulsory);
            Configuration.ConfirmUint(xmlElement, "Length",
                secQueueDBConfig.Length, compulsory);
            Configuration.ConfirmInt(xmlElement, "PadByte",
                secQueueDBConfig.PadByte, compulsory);
        }

        public static void Config(XmlElement xmlElement,
            ref SecondaryQueueDatabaseConfig secQueueDBConfig,
            bool compulsory)
        {
            uint uintValue = new uint();
            int intValue = new int();
            SecondaryDatabaseConfig secConfig = secQueueDBConfig;
            SecondaryDatabaseConfigTest.Config(xmlElement, 
                ref secConfig, compulsory);

            // Configure specific fields/properties of Queue database
            Configuration.ConfigCreatePolicy(xmlElement, "Creation",
                ref secQueueDBConfig.Creation, compulsory);
            if (Configuration.ConfigUint(xmlElement, "Length", 
                ref uintValue, compulsory))
                secQueueDBConfig.Length = uintValue;
            if (Configuration.ConfigInt(xmlElement, "PadByte", 
                ref intValue, compulsory))
                secQueueDBConfig.PadByte = intValue;
            if (Configuration.ConfigUint(xmlElement, "ExtentSize", 
                ref uintValue, compulsory))
                secQueueDBConfig.ExtentSize = uintValue;
        }
    }
}

