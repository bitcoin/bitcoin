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
    public class LockingConfigTest
    {
        private string testFixtureHome;
        private string testFixtureName;
        private string testName;
        private string testHome;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "LockingConfigTest";
            testFixtureHome = "./TestOut/" + testFixtureName;

            Configuration.ClearDir(testFixtureHome);
        }

        [Test]
        public void TestConfig()
        {
            testName = "TestConfig";

            // Configure the fields/properties.
            LockingConfig lockingConfig = new LockingConfig();
            XmlElement xmlElem = Configuration.TestSetUp(
                testFixtureName, testName);

            // Configure LockingConfig
            Config(xmlElem, ref lockingConfig, true);

            // Confirm LockingConfig
            Confirm(xmlElem, lockingConfig, true);
        }

        [Test]
        public void TestMaxLock() {
            testName = "TestMaxLock";
            testHome = testFixtureHome + "/" + testName;
            Configuration.ClearDir(testHome);

            DatabaseEnvironment env;
            uint maxLocks;
            DatabaseEntry obj;

            maxLocks = 1;
            obj = new DatabaseEntry();

            /*
             * Initialize environment using locking subsystem. The max number 
             * of locks should be larger than environment's partitions. So in 
             * order to make the MaxLock work, the environment paritition is 
             * set to be the same value as MaxLock.
             */
            LockTest.LockingEnvSetUp(testHome, testName, out env,
                maxLocks, 0, 0, maxLocks);
            Assert.AreEqual(maxLocks, env.MaxLocks);

            env.Close();
        }

        [Test]
        public void TestMaxLocker() {
            testName = "TestMaxLocker";
            testHome = testFixtureHome + "/" + testName;
            Configuration.ClearDir(testHome);

            DatabaseEnvironment env;
            uint maxLockers;

            maxLockers = 1;
            LockTest.LockingEnvSetUp(testHome, testName, out env,
                0, maxLockers, 0, 0);
            Assert.AreEqual(maxLockers, env.MaxLockers);
            env.Close();
        }

        [Test]
        public void TestMaxObjects() {
            testName = "TestMaxObjects";
            testHome = testFixtureHome + "/" + testName;
            Configuration.ClearDir(testHome);

            DatabaseEnvironment env;
            uint maxObjects;

            maxObjects = 1;

            /*
             * Initialize environment using locking subsystem. The max number 
             * of objects should be larger than environment's partitions. So 
             * in order to make the MaxObject work, the environment paritition 
             * is set to be the same value as MaxObject.
             */ 
            LockTest.LockingEnvSetUp(testHome, testName, out env, 0, 0, 
                maxObjects, maxObjects);
            Assert.AreEqual(maxObjects, env.MaxObjects);
            env.Close();
        }

        public static void Confirm(XmlElement xmlElement,
            LockingConfig lockingConfig, bool compulsory)
        {
            Configuration.ConfirmByteMatrix(xmlElement, "Conflicts",
                lockingConfig.Conflicts, compulsory);
            Configuration.ConfirmDeadlockPolicy(xmlElement,
                "DeadlockResolution",
                lockingConfig.DeadlockResolution, compulsory);
            Configuration.ConfirmUint(xmlElement, "MaxLockers",
                lockingConfig.MaxLockers, compulsory);
            Configuration.ConfirmUint(xmlElement, "MaxLocks",
                lockingConfig.MaxLocks, compulsory);
            Configuration.ConfirmUint(xmlElement, "MaxObjects",
                lockingConfig.MaxObjects, compulsory);
            Configuration.ConfirmUint(xmlElement, "Partitions",
                lockingConfig.Partitions, compulsory);
        }

        public static void Config(XmlElement xmlElement,
            ref LockingConfig lockingConfig, bool compulsory)
        {
            byte[,] matrix = new byte[6, 6];
            uint value = new uint();

            if (Configuration.ConfigByteMatrix(xmlElement, "Conflicts", 
                ref matrix, compulsory) == true)
                lockingConfig.Conflicts = matrix;

            Configuration.ConfigDeadlockPolicy(xmlElement, "DeadlockResolution",
                ref lockingConfig.DeadlockResolution, compulsory);
            if (Configuration.ConfigUint(xmlElement, "MaxLockers", 
                ref value, compulsory))
                lockingConfig.MaxLockers = value;
            if (Configuration.ConfigUint(xmlElement, "MaxLocks", 
                ref value, compulsory))
                lockingConfig.MaxLocks = value;
            if (Configuration.ConfigUint(xmlElement, "MaxObjects", 
                ref value, compulsory))
                lockingConfig.MaxObjects = value;
            if (Configuration.ConfigUint(xmlElement, "Partitions", 
                ref value, compulsory))
                lockingConfig.Partitions = value;
        }

    }
}
