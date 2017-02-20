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
    public class MPoolConfigTest
    {
        private string testFixtureName;
        private string testName;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "MPoolConfigTest";
        }

        [Test]
        public void TestConfig()
        {
            testName = "TestConfig";

            // Config and confirm mpool subsystem configuration.
            MPoolConfig mpoolConfig = new MPoolConfig();
            XmlElement xmlElem = Configuration.TestSetUp(
                testFixtureName, testName);
            Config(xmlElem, ref mpoolConfig, true);
            Confirm(xmlElem, mpoolConfig, true);
        }


        public static void Confirm(XmlElement
            xmlElement, MPoolConfig mpoolConfig, bool compulsory)
        {
            Configuration.ConfirmCacheSize(xmlElement, 
                "CacheSize", mpoolConfig.CacheSize, compulsory);
            Configuration.ConfirmCacheSize(xmlElement,
                "MaxCacheSize", mpoolConfig.MaxCacheSize, 
                compulsory);
            Configuration.ConfirmInt(xmlElement, "MaxOpenFiles",
                mpoolConfig.MaxOpenFiles, compulsory);
            Configuration.ConfirmUint(xmlElement,
                "MMapSize",
                mpoolConfig.MMapSize, compulsory);
            Configuration.ConfirmMaxSequentialWrites(xmlElement,
                "MaxSequentialWrites",
                mpoolConfig.SequentialWritePause, 
                mpoolConfig.MaxSequentialWrites, compulsory);
        }

        public static void Config(XmlElement
            xmlElement, ref MPoolConfig mpoolConfig, bool compulsory)
        {
            uint uintValue = new uint();
            int intValue = new int();

            Configuration.ConfigCacheInfo(xmlElement,
                "CacheSize", ref mpoolConfig.CacheSize, compulsory);
            Configuration.ConfigCacheInfo(xmlElement,
                "MaxCacheSize", ref mpoolConfig.MaxCacheSize, 
                compulsory);
            if (Configuration.ConfigInt(xmlElement, "MaxOpenFiles",
                ref intValue, compulsory))
                mpoolConfig.MaxOpenFiles = intValue;
            Configuration.ConfigMaxSequentialWrites(
                xmlElement, "MaxSequentialWrites", mpoolConfig,
                compulsory);
            if (Configuration.ConfigUint(xmlElement,
                "MMapSize", ref uintValue, compulsory))
                mpoolConfig.MMapSize = uintValue;
        }
    }
}
