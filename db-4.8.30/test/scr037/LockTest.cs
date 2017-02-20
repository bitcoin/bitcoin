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

namespace CsharpAPITest {
    [TestFixture]
    public class LockTest {
        private string testFixtureHome;
        private string testFixtureName;
        private string testName;
        private string testHome;

        [TestFixtureSetUp]
        public void RunBeforeTests() {
            testFixtureName = "LockTest";
            testFixtureHome = "./TestOut/" + testFixtureName;

            /*
             * Delete existing test ouput directory and files specified 
             * for the current test fixture and then create a new one.
             */
            Configuration.ClearDir(testFixtureHome);
        }

        [Test]
        public void TestLockStats() {
            testName = "TestLockManyAndStats";
            testHome = testFixtureHome + "/" + testName;
            Configuration.ClearDir(testHome);

            // Configure locking subsystem.
            LockingConfig lkConfig = new LockingConfig();
            lkConfig.MaxLockers = 60;
            lkConfig.MaxLocks = 50;
            lkConfig.MaxObjects = 70;
            lkConfig.Partitions = 20;

            // Configure and open environment.
            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            envConfig.MPoolSystemCfg = new MPoolConfig();
            envConfig.Create = true;
            envConfig.LockSystemCfg = lkConfig;
            envConfig.UseLocking = true;
            envConfig.UseMPool = true;
            envConfig.UseTxns = true;
            envConfig.ErrorPrefix = testName;
            envConfig.NoLocking = false;
            envConfig.LockTimeout = 1000;
            envConfig.TxnTimeout = 2000;
            envConfig.MPoolSystemCfg.CacheSize = new CacheInfo(0, 10485760, 1);
            DatabaseEnvironment env =
                DatabaseEnvironment.Open(testHome, envConfig);

            // Get and confirm locking subsystem statistics.
            LockStats stats = env.LockingSystemStats();
            env.PrintLockingSystemStats(true, true);
            Assert.AreEqual(1000, stats.LockTimeoutLength);
            Assert.AreEqual(60, stats.MaxLockersInTable);
            Assert.AreEqual(50, stats.MaxLocksInTable);
            Assert.AreEqual(70, stats.MaxObjectsInTable);
            Assert.AreNotEqual(0, stats.MaxUnusedID);
            Assert.AreEqual(20, stats.nPartitions);
            Assert.AreNotEqual(0, stats.RegionNoWait);
            Assert.AreNotEqual(0, stats.RegionSize);
            Assert.AreEqual(0, stats.RegionWait);
            Assert.AreEqual(2000, stats.TxnTimeoutLength);

            env.PrintLockingSystemStats();
            
            env.Close();
        }

        public static void LockingEnvSetUp(string testHome,
            string testName, out DatabaseEnvironment env,
            uint maxLock, uint maxLocker, uint maxObject,
            uint partition) {
            // Configure env and locking subsystem.
            LockingConfig lkConfig = new LockingConfig();
            /*
             * If the maximum number of locks/lockers/objects
             * is given, then the LockingConfig is set. Unless, 
             * it is not set to any value.  
             */
            if (maxLock != 0)
                lkConfig.MaxLocks = maxLock;
            if (maxLocker != 0)
                lkConfig.MaxLockers = maxLocker;
            if (maxObject != 0)
                lkConfig.MaxObjects = maxObject;
            if (partition != 0)
                lkConfig.Partitions = partition;

            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.LockSystemCfg = lkConfig;
            envConfig.UseLocking = true;
            envConfig.ErrorPrefix = testName;

            env = DatabaseEnvironment.Open(testHome, envConfig);
        }
    }
}
