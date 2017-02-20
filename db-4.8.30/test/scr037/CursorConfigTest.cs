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
    public class CursorConfigTest
    {
        private string testFixtureName;
        private string testFixtureHome;
        private string testName;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "CursorConfigTest";
            testFixtureHome = "./TestOut/" + testFixtureName;
        }

        [Test]
        public void TestConfig()
        {
            testName = "TestConfig";
            /* 
             * Configure the fields/properties and see if 
             * they are updated successfully.
             */ 
            CursorConfig cursorConfig = new CursorConfig();
            XmlElement xmlElem = Configuration.TestSetUp(
                testFixtureName, testName);
            Config(xmlElem, ref cursorConfig, true);
            Confirm(xmlElem, cursorConfig, true);
        }

        public static void Confirm(XmlElement xmlElement, 
            CursorConfig cursorConfig, bool compulsory)
        {
            Configuration.ConfirmIsolation(xmlElement, 
                "IsolationDegree", cursorConfig.IsolationDegree,
                compulsory);
            Configuration.ConfirmCachePriority(xmlElement,
                "Priority", cursorConfig.Priority,
                compulsory);
            Configuration.ConfirmBool(xmlElement,
                "SnapshotIsolation", cursorConfig.SnapshotIsolation,
                compulsory);
            Configuration.ConfirmBool(xmlElement,
                "WriteCursor", cursorConfig.WriteCursor,
                compulsory);
        }

        public static void Config(XmlElement xmlElement, 
            ref CursorConfig cursorConfig, bool compulsory)
        {
            Configuration.ConfigIsolation(xmlElement, 
                "IsolationDegree", ref cursorConfig.IsolationDegree, 
                compulsory);
            Configuration.ConfigCachePriority(xmlElement,
                "Priority", ref cursorConfig.Priority, compulsory);
            Configuration.ConfigBool(xmlElement,
                "SnapshotIsolation", ref cursorConfig.SnapshotIsolation,
                compulsory);
            Configuration.ConfigBool(xmlElement,
                "WriteCursor", ref cursorConfig.WriteCursor,
                compulsory);
        }
    }
}
