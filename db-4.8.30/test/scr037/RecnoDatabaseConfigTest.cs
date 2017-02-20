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
    public class RecnoDatabaseConfigTest : DatabaseConfigTest
    {
        private string testFixtureHome;
        private string testFixtureName;
        private string testName;
        private string testHome;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "RecnoDatabaseConfigTest";
            testFixtureHome = "./TestOut/" + testFixtureName;

            Configuration.ClearDir(testFixtureHome);
        }

        [Test]
        override public void TestConfigWithoutEnv()
        {
            testName = "TestConfigWithoutEnv";
            XmlElement xmlElem = Configuration.TestSetUp(
                testFixtureName, testName);

            RecnoDatabaseConfig recnoDBConfig =
                new RecnoDatabaseConfig();
            Config(xmlElem, ref recnoDBConfig, true);
            Confirm(xmlElem, recnoDBConfig, true);
        }

        [Test]
        public void TestAppend()
        {
            uint recno;

            testName = "TestAppend";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";

            Configuration.ClearDir(testHome);

            RecnoDatabaseConfig recnoConfig =
                new RecnoDatabaseConfig();
            recnoConfig.Creation = CreatePolicy.IF_NEEDED;
            recnoConfig.Append = new AppendRecordDelegate(
                AppendRecord);
            RecnoDatabase recnoDB = RecnoDatabase.Open(
                dbFileName, recnoConfig);
            recno = recnoDB.Append(new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("data")));

            KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
            pair = recnoDB.Get(
                new DatabaseEntry(BitConverter.GetBytes(recno)));
            Assert.AreEqual(ASCIIEncoding.ASCII.GetBytes("data"),
                pair.Value.Data);

            recnoDB.Close();
        }

        public void AppendRecord(DatabaseEntry data, uint recno)
        {
            data.Data = BitConverter.GetBytes(recno);
        }

        public static void Confirm(XmlElement xmlElement,
            RecnoDatabaseConfig recnoDBConfig, bool compulsory)
        {
            DatabaseConfig dbConfig = recnoDBConfig;
            Confirm(xmlElement, dbConfig, compulsory);

            // Confirm Recno database specific configuration
            Configuration.ConfirmString(xmlElement, "BackingFile",
                recnoDBConfig.BackingFile, compulsory);
            Configuration.ConfirmCreatePolicy(xmlElement,
                "Creation", recnoDBConfig.Creation, compulsory);
            Configuration.ConfirmInt(xmlElement, "Delimiter",
                recnoDBConfig.Delimiter, compulsory);
            Configuration.ConfirmUint(xmlElement, "Length",
                recnoDBConfig.Length, compulsory);
            Configuration.ConfirmInt(xmlElement, "PadByte",
                recnoDBConfig.PadByte, compulsory);
            Configuration.ConfirmBool(xmlElement, "Renumber",
                recnoDBConfig.Renumber, compulsory);
            Configuration.ConfirmBool(xmlElement, "Snapshot",
                recnoDBConfig.Snapshot, compulsory);
        }

        public static void Config(XmlElement xmlElement,
            ref RecnoDatabaseConfig recnoDBConfig, bool compulsory)
        {
            int intValue = new int();
            uint uintValue = new uint();
            DatabaseConfig dbConfig = recnoDBConfig;
            Config(xmlElement, ref dbConfig, compulsory);

            // Configure specific fields/properties of Recno database
            Configuration.ConfigCreatePolicy(xmlElement, "Creation",
                ref recnoDBConfig.Creation, compulsory);
            if (Configuration.ConfigInt(xmlElement, "Delimiter",
                ref intValue, compulsory))
                recnoDBConfig.Delimiter = intValue;
            if (Configuration.ConfigUint(xmlElement, "Length",
                ref uintValue, compulsory))
                recnoDBConfig.Length = uintValue;
            if (Configuration.ConfigInt(xmlElement, "PadByte",
                ref intValue, compulsory))
                recnoDBConfig.PadByte = intValue;
            Configuration.ConfigBool(xmlElement, "Renumber",
                ref recnoDBConfig.Renumber, compulsory);
            Configuration.ConfigBool(xmlElement, "Snapshot",
                ref recnoDBConfig.Snapshot, compulsory);
        }
    }
}
