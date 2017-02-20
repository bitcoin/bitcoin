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
    public class HashDatabaseConfigTest : DatabaseConfigTest
    {
        private string testFixtureHome;
        private string testFixtureName;
        private string testName;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "HashDatabaseConfigTest";
            testFixtureHome = "./TestOut/" + testFixtureName;

            Configuration.ClearDir(testFixtureHome);
        }

        [Test]
        override public void TestConfigWithoutEnv()
        {
            testName = "TestConfigWithoutEnv";
            XmlElement xmlElem = Configuration.TestSetUp(
                testFixtureName, testName);
            HashDatabaseConfig hashConfig = new HashDatabaseConfig();
            Config(xmlElem, ref hashConfig, true);
            Confirm(xmlElem, hashConfig, true);
        }


        public static void Confirm(XmlElement xmlElement,
            HashDatabaseConfig hashDBConfig, bool compulsory)
        {
            DatabaseConfig dbConfig = hashDBConfig;
            Confirm(xmlElement, dbConfig, compulsory);

            // Confirm Hash database specific configuration.
            Configuration.ConfirmCreatePolicy(xmlElement,
                "Creation", hashDBConfig.Creation, compulsory);
            Configuration.ConfirmDuplicatesPolicy(xmlElement,
                "Duplicates", hashDBConfig.Duplicates, compulsory);
            Configuration.ConfirmUint(xmlElement, "FillFactor", 
                hashDBConfig.FillFactor, compulsory);
            Configuration.ConfirmUint(xmlElement, "NumElements",
                hashDBConfig.TableSize, compulsory);
        }

        public static void Config(XmlElement xmlElement,
            ref HashDatabaseConfig hashDBConfig, bool compulsory)
        {
            uint fillFactor = new uint();
            uint numElements = new uint();
            DatabaseConfig dbConfig = hashDBConfig;
            Config(xmlElement, ref dbConfig, compulsory);

            // Configure specific fields/properties of Hash db
            Configuration.ConfigCreatePolicy(xmlElement,
                "Creation", ref hashDBConfig.Creation,
                compulsory);
            Configuration.ConfigDuplicatesPolicy(xmlElement,
                "Duplicates", ref hashDBConfig.Duplicates,
                compulsory);
            if (Configuration.ConfigUint(xmlElement, "FillFactor",
                ref fillFactor, compulsory))
                hashDBConfig.FillFactor = fillFactor;
            if (Configuration.ConfigUint(xmlElement, "NumElements",
                ref numElements, compulsory))
                hashDBConfig.TableSize = numElements;
        }
    }
}
