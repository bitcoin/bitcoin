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
    public class MutexTest
    {
        private string testFixtureHome;
        private string testFixtureName;
        private string testName;
        private string testHome;

        private BTreeDatabase TestDB;
        private BerkeleyDB.Mutex TestMutex;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "MutexTest";
            testFixtureHome = "./TestOut/" + testFixtureName;

            Configuration.ClearDir(testFixtureHome);
        }

        [Test]
        public void TestGetAndFreeMutex()
        {
            testName = "TestGetAndFreeMutex";
            testHome = testFixtureHome + "/" + testName;
            Configuration.ClearDir(testHome);
            
            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.UseMPool = true;
            DatabaseEnvironment env = DatabaseEnvironment.Open(
                testHome, envConfig);
            BerkeleyDB.Mutex mutex = env.GetMutex(true, true);
            mutex.Dispose();
            env.Close();
        }

        [Test]
        public void TestLockAndUnlockMutex()
        {
            testName = "TestLockAndUnlockMutex";
            testHome = testFixtureHome + "/" + testName;
            Configuration.ClearDir(testHome);

            /*
             * Open an environment without locking and 
             * deadlock detection.
             */ 
            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            envConfig.FreeThreaded = true;
            envConfig.UseLogging = true;
            envConfig.Create = true;
            envConfig.UseMPool = true;
            DatabaseEnvironment env = DatabaseEnvironment.Open(
                testHome, envConfig);

            // Open a database.
            BTreeDatabaseConfig dbConfig =
                new BTreeDatabaseConfig();
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            dbConfig.Env = env;
            TestDB = BTreeDatabase.Open(
                testName + ".db", dbConfig);

            // Get a mutex which will be used in two threads.
            TestMutex = env.GetMutex(true, false);

            // Begin two threads to write records into database.
            Thread mutexThread1 = new Thread(
                new ThreadStart(MutexThread1));
            Thread mutexThread2 = new Thread(
                new ThreadStart(MutexThread2));
            mutexThread1.Start();
            mutexThread2.Start();
            mutexThread1.Join();
            mutexThread2.Join();

            // Free the mutex.
            TestMutex.Dispose();

            // Close all.
            TestDB.Close();
            env.Close();
        }

        public void MutexThread1()
        {
            TestMutex.Lock();
            for (int i = 0; i < 100; i++)
                TestDB.Put(new DatabaseEntry(
                    BitConverter.GetBytes(i)),
                    new DatabaseEntry(new byte[102400]));
            TestMutex.Unlock();
        }

        public void MutexThread2()
        {
            TestMutex.Lock();
            for (int i = 0; i < 100; i++)
                TestDB.Put(new DatabaseEntry(
                    BitConverter.GetBytes(i)), 
                    new DatabaseEntry(new byte[102400]));
            TestMutex.Unlock();
        }
    }
}
