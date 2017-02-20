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
    public class SecondaryRecnoDatabaseConfigTest
    {
        private string testFixtureHome;
        private string testFixtureName;
        private string testName;
        private string testHome;
        
        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "SecondaryRecnoDatabaseConfigTest";
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
            RecnoDatabaseConfig recDBConfig =
                new RecnoDatabaseConfig();
            recDBConfig.Creation = CreatePolicy.IF_NEEDED;
            RecnoDatabase recDB = RecnoDatabase.Open(
                dbFileName, recDBConfig);

            SecondaryRecnoDatabaseConfig secDBConfig =
                new SecondaryRecnoDatabaseConfig(recDB, null);

            Config(xmlElem, ref secDBConfig, true);
            Confirm(xmlElem, secDBConfig, true);

            // Close the primary btree database.
            recDB.Close();
        }

        public static void Confirm(XmlElement xmlElement,
            SecondaryRecnoDatabaseConfig secRecDBConfig,
            bool compulsory)
        {
            SecondaryDatabaseConfig secDBConfig =
                secRecDBConfig;
            SecondaryDatabaseConfigTest.Confirm(xmlElement,
                secDBConfig, compulsory);

            // Confirm secondary hash database specific configuration.
            Configuration.ConfirmString(xmlElement, "BackingFile", 
                secRecDBConfig.BackingFile, compulsory);
            Configuration.ConfirmCreatePolicy(xmlElement, "Creation", 
                secRecDBConfig.Creation, compulsory);
            Configuration.ConfirmInt(xmlElement, "Delimiter", 
                secRecDBConfig.Delimiter, compulsory);
            Configuration.ConfirmUint(xmlElement, "Length", 
                secRecDBConfig.Length, compulsory);
            Configuration.ConfirmInt(xmlElement, "PadByte", 
                secRecDBConfig.PadByte, compulsory);
            Configuration.ConfirmBool(xmlElement, "Renumber", 
                secRecDBConfig.Renumber, compulsory);
            Configuration.ConfirmBool(xmlElement, "Snapshot",
                secRecDBConfig.Snapshot, compulsory);
        }

        public static void Config(XmlElement xmlElement,
            ref SecondaryRecnoDatabaseConfig secRecDBConfig,
            bool compulsory)
        {
            int intValue = new int();
            uint uintValue = new uint();
            SecondaryDatabaseConfig secDBConfig = secRecDBConfig;
            SecondaryDatabaseConfigTest.Config(xmlElement, 
                ref secDBConfig, compulsory);

            // Configure specific fields/properties of Recno database
            Configuration.ConfigCreatePolicy(xmlElement, "Creation",
                ref secRecDBConfig.Creation, compulsory);
            if (Configuration.ConfigInt(xmlElement, "Delimiter", 
                ref intValue, compulsory))
                secRecDBConfig.Delimiter = intValue;
            if (Configuration.ConfigUint(xmlElement, "Length", 
                 ref  uintValue, compulsory))
                secRecDBConfig.Length = uintValue;
            if (Configuration.ConfigInt(xmlElement, "PadByte", 
                ref intValue, compulsory))
                secRecDBConfig.PadByte = intValue;
            Configuration.ConfigBool(xmlElement, "Renumber", 
                ref secRecDBConfig.Renumber, compulsory);
            Configuration.ConfigBool(xmlElement, "Snapshot", 
                ref secRecDBConfig.Snapshot, compulsory);
        }
    }
}

