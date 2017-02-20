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
    public class TransactionTest
    {
        private string testFixtureHome;
        private string testFixtureName;
        private string testName;
        private string testHome;

        private DatabaseEnvironment deadLockEnv;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "TransactionTest";
            testFixtureHome = "./TestOut/" + testFixtureName;

            DatabaseEnvironment.Remove(testFixtureHome);
        }

        [Test, ExpectedException(typeof(ExpectedTestException))]
        public void TestAbort()
        {
            testName = "TestAbort";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            DatabaseEnvironment env;
            Transaction txn;
            BTreeDatabase db;

            /* 
             * Open an environment and begin a transaction. Open
             * a db and write a record the db within this transaction.
             */ 
            PutRecordWithTxn(out env, testHome, testName, out txn);

            // Abort the transaction.
            txn.Abort();

            /* 
             * Undo all operations in the transaction so the
             * database couldn't be reopened.
             */ 
            try
            {
                OpenBtreeDBInEnv(testName + ".db", env,
                    out db, false, null);
            }
            catch (DatabaseException)
            {
                throw new ExpectedTestException();
            }
            finally
            {
                env.Close();
            }
        }

        [Test]
        public void TestCommit()
        {
            testName = "TestCommit";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            DatabaseEnvironment env;
            Transaction txn;
            BTreeDatabase db;

            /* 
             * Open an environment and begin a transaction. Open
             * a db and write a record the db within this transaction.
             */ 
            PutRecordWithTxn(out env, testHome, testName, out txn);

            // Commit the transaction.
            txn.Commit();

            // Reopen the database.
            OpenBtreeDBInEnv(testName + ".db", env,
                out db, false, null);

            /*
             * Confirm that the record("key", "data") exists in the
             * database.
             */
            try
            {
                db.GetBoth(new DatabaseEntry(
                    ASCIIEncoding.ASCII.GetBytes("key")),
                    new DatabaseEntry(
                    ASCIIEncoding.ASCII.GetBytes("data")));
            }
            catch (DatabaseException)
            {
                throw new TestException();
            }
            finally
            {
                db.Close();
                env.Close();
            }
        }

        [Test, ExpectedException(typeof(ExpectedTestException))]
        public void TestDiscard()
        {
            DatabaseEnvironment env;
            byte[] gid;

            testName = "TestDiscard";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            /*
             * Open an environment and begin a transaction
             * called "transaction". Within the transacion, open a 
             * database, write a record and close it. Then prepare 
             * the transaction and panic the environment.
             */
            PanicPreparedTxn(testHome, testName, out env, out gid);

            /*
             * Recover the environment.     Log and db files are not 
             * destoyed so run normal recovery. Recovery should 
             * use DB_CREATE and DB_INIT_TXN flags when 
             * opening the environment.
             */
            DatabaseEnvironmentConfig envConfig
                = new DatabaseEnvironmentConfig();
            envConfig.RunRecovery = true;
            envConfig.Create = true;
            envConfig.UseTxns = true;
            envConfig.UseMPool = true;
            env = DatabaseEnvironment.Open(testHome, envConfig);

            PreparedTransaction[] preparedTxns
                = new PreparedTransaction[10];
            preparedTxns = env.Recover(10, true);

            Assert.AreEqual(gid, preparedTxns[0].GlobalID);
            preparedTxns[0].Txn.Discard();
            try
            {
                preparedTxns[0].Txn.Commit();
            }
            catch (AccessViolationException)
            {
                throw new ExpectedTestException();
            }
            finally
            {
                env.Close();
            }
        }

        [Test]
        public void TestPrepare()
        {
            testName = "TestPrepare";
            testHome = testFixtureHome + "/" + testName;

            DatabaseEnvironment env;
            byte[] gid;

            Configuration.ClearDir(testHome);

            /*
             * Open an environment and begin a transaction
             * called "transaction". Within the transacion, open a 
             * database, write a record and close it. Then prepare 
             * the transaction and panic the environment.
             */
            PanicPreparedTxn(testHome, testName, out env, out gid);

            /*
             * Recover the environment.     Log and db files are not 
             * destoyed so run normal recovery. Recovery should 
             * use DB_CREATE and DB_INIT_TXN flags when 
             * opening the environment.
             */
            DatabaseEnvironmentConfig envConfig
                = new DatabaseEnvironmentConfig();
            envConfig.RunRecovery = true;
            envConfig.Create = true;
            envConfig.UseTxns = true;
            envConfig.UseMPool = true;
            env = DatabaseEnvironment.Open(testHome, envConfig);

            // Reopen the database.
            BTreeDatabase db;
            OpenBtreeDBInEnv(testName + ".db", env, out db, 
                false, null);

            /*
             * Confirm that record("key", "data") exists in the 
             * database.
             */
            DatabaseEntry key, data;
            key = new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("key"));
            data = new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("data"));
            try
            {
                db.GetBoth(key, data);
            }
            catch (DatabaseException)
            {
                throw new TestException();
            }
            finally
            {
                db.Close();
                env.Close();
            }
            
        }

        public void PanicPreparedTxn(string home, string dbName,
            out DatabaseEnvironment env, out byte[] globalID)
        {
            Transaction txn;

            // Put record into database within transaction.
            PutRecordWithTxn(out env, home, dbName, out txn);

            /*
             * Generate global ID for the transaction. Copy 
             * transaction ID to the first 4 tyes in global ID.
             */
            globalID = new byte[Transaction.GlobalIdLength];
            byte[] txnID = new byte[4];
            txnID = BitConverter.GetBytes(txn.Id);
            for (int i = 0; i < txnID.Length; i++)
                globalID[i] = txnID[i];

            // Prepare the transaction.
            txn.Prepare(globalID);

            // Panic the environment.
            env.Panic();

        }

        [Test]
        public void TestTxnName()
        {
            DatabaseEnvironment env;
            Transaction txn;

            testName = "TestTxnName";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            SetUpTransactionalEnv(testHome, out env);
            txn = env.BeginTransaction();
            txn.Name = testName;
            Assert.AreEqual(testName, txn.Name);
            txn.Commit();
            env.Close();
        }

        [Test]
        public void TestSetLockTimeout()
        {
            testName = "TestSetLockTimeout";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            // Set lock timeout.
            TestTimeOut(true);

        }

        [Test]
        public void TestSetTxnTimeout()
        {
            testName = "TestSetTxnTimeout";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            // Set transaction time out.
            TestTimeOut(false);

        }

        /*
         * ifSetLock is used to indicate which timeout function 
         * is used, SetLockTimeout or SetTxnTimeout.
         */
        public void TestTimeOut(bool ifSetLock)
        {
            // Open environment and begin transaction.
            Transaction txn;
            deadLockEnv = null;
            SetUpEnvWithTxnAndLocking(testHome,
                out deadLockEnv, out txn, 0, 0, 0, 0);

            // Define deadlock detection and resolve policy.
            deadLockEnv.DeadlockResolution =
                DeadlockPolicy.YOUNGEST;
            if (ifSetLock == true)
                txn.SetLockTimeout(10);
            else
                txn.SetTxnTimeout(10);

            txn.Commit();
            deadLockEnv.Close();
        }

        public static void SetUpEnvWithTxnAndLocking(string envHome,
            out DatabaseEnvironment env, out Transaction txn,
            uint maxLock, uint maxLocker, uint maxObject, uint partition)
        {
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
            envConfig.UseTxns = true;
            envConfig.UseMPool = true;
            envConfig.LockSystemCfg = lkConfig;
            envConfig.UseLocking = true;
            envConfig.NoLocking = false;
            env = DatabaseEnvironment.Open(envHome, envConfig);
            txn = env.BeginTransaction();
        }

        public void PutRecordWithTxn(out DatabaseEnvironment env,
            string home, string dbName, out Transaction txn)
        {
            BTreeDatabase db;

            // Open a new environment and begin a transaction.
            SetUpTransactionalEnv(home, out env);
            TransactionConfig txnConfig = new TransactionConfig();
            txnConfig.Name = "Transaction";
            txn = env.BeginTransaction(txnConfig);
            Assert.AreEqual("Transaction", txn.Name);

            // Open a new database within the transaction.
            OpenBtreeDBInEnv(dbName + ".db", env, out db, true, txn);

            // Write to the database within the transaction.
            WriteOneIntoBtreeDBWithTxn(db, txn);

            // Close the database.
            db.Close();
        }

        public void SetUpTransactionalEnv(string home, 
            out DatabaseEnvironment env)
        {
            DatabaseEnvironmentConfig envConfig = 
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.UseLogging = true;
            envConfig.UseMPool = true;
            envConfig.UseTxns = true;
            env = DatabaseEnvironment.Open(
                home, envConfig);
        }

        public void OpenBtreeDBInEnv(string dbName,
            DatabaseEnvironment env, out BTreeDatabase db,
            bool create, Transaction txn)
        {
            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Env = env;
            if (create == true)
                btreeDBConfig.Creation = CreatePolicy.IF_NEEDED;
            else
                btreeDBConfig.Creation = CreatePolicy.NEVER;
            if (txn == null)
                db = BTreeDatabase.Open(dbName,
                    btreeDBConfig);
            else
                db = BTreeDatabase.Open(dbName,
                    btreeDBConfig, txn);
        }

        public void WriteOneIntoBtreeDBWithTxn(BTreeDatabase db,
            Transaction txn)
        {
            DatabaseEntry key, data;

            key = new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("key"));
            data = new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("data"));
            db.Put(key, data, txn);
        }
    }
}
