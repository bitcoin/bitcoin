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
    public class SecondaryDatabaseConfigTest
    {
        private string testFixtureHome;
        private string testFixtureName;
        private string testName;
        private string testHome;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "SecondaryDatabaseConfigTest";
            testFixtureHome = "./TestOut/" + testFixtureName;

            Configuration.ClearDir(testFixtureHome);
        }

        [Test]
        virtual public void TestConfig()
        {
            testName = "TestConfig";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName;

            Configuration.ClearDir(testHome);

            XmlElement xmlElem = Configuration.TestSetUp(
                testFixtureName, testName);
            

            // Open a primary btree database.
            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.IF_NEEDED;
            BTreeDatabase btreeDB = BTreeDatabase.Open(
                dbFileName, btreeDBConfig);

            SecondaryDatabaseConfig secDBConfig = 
                new SecondaryDatabaseConfig(btreeDB, null);
            Config(xmlElem, ref secDBConfig, true);
            Confirm(xmlElem, secDBConfig, true);
            btreeDB.Close();
        }

        public static void Config(XmlElement xmlElement, 
            ref SecondaryDatabaseConfig secDBConfig, 
            bool compulsory)
        {
            Configuration.ConfigBool(xmlElement, "ImmutableKey", 
                ref secDBConfig.ImmutableKey, compulsory);
            Configuration.ConfigBool(xmlElement, "Populate", 
                ref secDBConfig.Populate, compulsory);
        }

        public static void Confirm(XmlElement xmlElement, 
            SecondaryDatabaseConfig secDBConfig, 
            bool compulsory)
        {
            Configuration.ConfirmBool(xmlElement, "ImmutableKey", 
                secDBConfig.ImmutableKey, compulsory);
            Configuration.ConfirmBool(xmlElement, "Populate",  
                secDBConfig.Populate, compulsory);
        }
    }
}
