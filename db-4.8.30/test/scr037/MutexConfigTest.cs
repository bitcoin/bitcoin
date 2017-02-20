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
    public class MutexConfigTest
    {
        private string testFixtureHome;
        private string testFixtureName;
        private string testName;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "MutexConfigTest";
            testFixtureHome = "./TestOut/" + testFixtureName;
            Configuration.ClearDir(testFixtureHome);
        }

        [Test]
        public void TestConfig()
        {
            testName = "TestConfig";

            /* 
             * Configure the fields/properties and see if 
             * they are updated successfully.
             */
            MutexConfig lockingConfig = new MutexConfig();
            XmlElement xmlElem = Configuration.TestSetUp(
                testFixtureName, testName);
            Config(xmlElem, ref lockingConfig, true);
            Confirm(xmlElem, lockingConfig, true);
        }


        public static void Confirm(XmlElement
            xmlElement, MutexConfig mutexConfig, bool compulsory)
        {
            Configuration.ConfirmUint(xmlElement, "Alignment",
                mutexConfig.Alignment, compulsory);
            Configuration.ConfirmUint(xmlElement, "Increment",
                mutexConfig.Increment, compulsory);
            Configuration.ConfirmUint(xmlElement, "MaxMutexes",
                mutexConfig.MaxMutexes, compulsory);
            Configuration.ConfirmUint(xmlElement,
                "NumTestAndSetSpins",
                mutexConfig.NumTestAndSetSpins, compulsory);
        }

        public static void Config(XmlElement
            xmlElement, ref MutexConfig mutexConfig, bool compulsory)
        {
            uint value = new uint();
            if (Configuration.ConfigUint(xmlElement, "Alignment",
                ref value, compulsory))
                mutexConfig.Alignment = value;
            if (Configuration.ConfigUint(xmlElement, "Increment",
                ref value, compulsory))
                mutexConfig.Increment = value;
            if (Configuration.ConfigUint(xmlElement, "MaxMutexes",
                ref value, compulsory))
                mutexConfig.MaxMutexes = value;
            if (Configuration.ConfigUint(xmlElement,
                "NumTestAndSetSpins", ref value, compulsory))
                mutexConfig.NumTestAndSetSpins = value;
        }
    }
}
