/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.Data;
using System.IO;
using System.Text;
using System.Xml;
using NUnit.Framework;
using BerkeleyDB;

namespace CsharpAPITest
{
    [TestFixture]
    public class DatabaseConfigTest
    {
        [Test]
        virtual public void TestConfigWithoutEnv()
        {
            string testName = "TestConfigWithoutEnv";
            string testFixtureName = "DatabaseConfigTest";

            XmlElement xmlElem = Configuration.TestSetUp(
                testFixtureName, testName);
            DatabaseConfig dbConfig = new DatabaseConfig();
            Config(xmlElem, ref dbConfig, true);
            Confirm(xmlElem, dbConfig, true);
        }

        public static void Config(XmlElement xmlElement,
            ref DatabaseConfig dbConfig, bool compulsory)
        {
            uint pageSize = new uint();

            Configuration.ConfigBool(xmlElement, "AutoCommit",
                ref dbConfig.AutoCommit, compulsory);
            Configuration.ConfigByteOrder(xmlElement, "ByteOrder",
                ref dbConfig.ByteOrder, compulsory);
            Configuration.ConfigCacheInfo(xmlElement, "CacheSize",
                ref dbConfig.CacheSize, compulsory);
            Configuration.ConfigBool(xmlElement, "DoChecksum",
                ref dbConfig.DoChecksum, compulsory);
            Configuration.ConfigString(xmlElement, "ErrorPrefix",
                ref dbConfig.ErrorPrefix, compulsory);
            Configuration.ConfigBool(xmlElement, "FreeThreaded",
                ref dbConfig.FreeThreaded, compulsory);
            Configuration.ConfigBool(xmlElement, "NoMMap",
                ref dbConfig.NoMMap, compulsory);
            Configuration.ConfigBool(xmlElement, "NonDurableTxns",
                ref dbConfig.NonDurableTxns, compulsory);
            if (Configuration.ConfigUint(xmlElement, "PageSize",
                ref pageSize, compulsory))
                dbConfig.PageSize = pageSize;
            Configuration.ConfigCachePriority(xmlElement,
                "Priority", ref dbConfig.Priority, compulsory);
            Configuration.ConfigBool(xmlElement, "ReadOnly",
                ref dbConfig.ReadOnly, compulsory);
            Configuration.ConfigBool(xmlElement, "ReadUncommitted",
                ref dbConfig.ReadUncommitted, compulsory);
            Configuration.ConfigEncryption(xmlElement,
                "Encryption", dbConfig, compulsory);
            Configuration.ConfigBool(xmlElement, "Truncate",
                ref dbConfig.Truncate, compulsory);
            Configuration.ConfigBool(xmlElement, "UseMVCC",
                ref dbConfig.UseMVCC, compulsory);
        }

        public static void Confirm(XmlElement xmlElement,
            DatabaseConfig dbConfig, bool compulsory)
        {
            Configuration.ConfirmBool(xmlElement, "AutoCommit",
                dbConfig.AutoCommit, compulsory);
            Configuration.ConfirmByteOrder(xmlElement, "ByteOrder",
                dbConfig.ByteOrder, compulsory);
            Configuration.ConfirmCacheSize(xmlElement, "CacheSize",
                dbConfig.CacheSize, compulsory);
            Configuration.ConfirmBool(xmlElement, "DoChecksum",
                dbConfig.DoChecksum, compulsory);
            Configuration.ConfirmEncryption(xmlElement, "Encryption",
                dbConfig.EncryptionPassword,
                dbConfig.EncryptAlgorithm, compulsory);
            Configuration.ConfirmString(xmlElement, "ErrorPrefix",
                dbConfig.ErrorPrefix, compulsory);
            Configuration.ConfirmBool(xmlElement, "FreeThreaded",
                dbConfig.FreeThreaded, compulsory);
            Configuration.ConfirmBool(xmlElement, "NoMMap",
                dbConfig.NoMMap, compulsory);
            Configuration.ConfirmBool(xmlElement, "NonDurableTxns",
                dbConfig.NonDurableTxns, compulsory);
            Configuration.ConfirmUint(xmlElement, "PageSize",
                dbConfig.PageSize, compulsory);
            Configuration.ConfirmCachePriority(xmlElement,
                "Priority", dbConfig.Priority, compulsory);
            Configuration.ConfirmBool(xmlElement, "ReadOnly",
                dbConfig.ReadOnly, compulsory);
            Configuration.ConfirmBool(xmlElement, "ReadUncommitted",
                dbConfig.ReadUncommitted, compulsory);
            Configuration.ConfirmBool(xmlElement, "Truncate",
                dbConfig.Truncate, compulsory);
            Configuration.ConfirmBool(xmlElement, "UseMVCC",
                dbConfig.UseMVCC, compulsory);
        }
    }
}

