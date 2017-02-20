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
    public class SecondaryRecnoDatabaseTest
    {
        private string testFixtureHome;
        private string testFixtureName;
        private string testName;
        private string testHome;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "SecondaryRecnoDatabaseTest";
            testFixtureHome = "./TestOut/" + testFixtureName;

            Configuration.ClearDir(testFixtureHome);
        }

        /*
         * Tests to all Open() share the same configuration in 
         * AllTestData.xml.
         */
        [Test]
        public void TestOpen()
        {
            testName = "TestOpen";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            string dbSecFileName = testHome + "/" +
                testName + "_sec.db";

            Configuration.ClearDir(testHome);

            OpenSecRecnoDB(testFixtureName, "TestOpen",
                dbFileName, dbSecFileName, false);
        }

        [Test]
        public void TestOpenWithDBName()
        {
            testName = "TestOpenWithDBName";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            string dbSecFileName = testHome + "/" +
                testName + "_sec.db";

            Configuration.ClearDir(testHome);

            OpenSecRecnoDB(testFixtureName, "TestOpen",
                dbFileName, dbSecFileName, true);
        }

        public void OpenSecRecnoDB(string className,
            string funName, string dbFileName, string dbSecFileName,
            bool ifDBName)
        {
            XmlElement xmlElem = Configuration.TestSetUp(
                className, funName);

            // Open a primary recno database.
            RecnoDatabaseConfig primaryDBConfig =
                new RecnoDatabaseConfig();
            primaryDBConfig.Creation = CreatePolicy.IF_NEEDED;
            RecnoDatabase primaryDB;

            /*
             * If secondary database name is given, the primary 
             * database is also opened with database name.
             */
            if (ifDBName == false)
                primaryDB = RecnoDatabase.Open(dbFileName,
                    primaryDBConfig);
            else
                primaryDB = RecnoDatabase.Open(dbFileName,
                    "primary", primaryDBConfig);

            try
            {
                // Open a new secondary database.
                SecondaryRecnoDatabaseConfig secRecnoDBConfig =
                    new SecondaryRecnoDatabaseConfig(
                    primaryDB, null);
                SecondaryRecnoDatabaseConfigTest.Config(
                    xmlElem, ref secRecnoDBConfig, false);
                secRecnoDBConfig.Creation =
                    CreatePolicy.IF_NEEDED;

                SecondaryRecnoDatabase secRecnoDB;
                if (ifDBName == false)
                    secRecnoDB = SecondaryRecnoDatabase.Open(
                        dbSecFileName, secRecnoDBConfig);
                else
                    secRecnoDB = SecondaryRecnoDatabase.Open(
                        dbSecFileName, "secondary",
                        secRecnoDBConfig);

                // Close the secondary database.
                secRecnoDB.Close();

                // Open the existing secondary database.
                SecondaryDatabaseConfig secDBConfig =
                    new SecondaryDatabaseConfig(
                    primaryDB, null);

                SecondaryDatabase secDB;
                if (ifDBName == false)
                    secDB = SecondaryRecnoDatabase.Open(
                        dbSecFileName, secDBConfig);
                else
                    secDB = SecondaryRecnoDatabase.Open(
                        dbSecFileName, "secondary", secDBConfig);

                // Close secondary database.
                secDB.Close();
            }
            catch (DatabaseException)
            {
                throw new TestException();
            }
            finally
            {
                // Close primary database.
                primaryDB.Close();
            }
        }

        [Test]
        public void TestOpenWithinTxn()
        {
            testName = "TestOpenWithinTxn";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testName + ".db";
            string dbSecFileName = testName + "_sec.db";

            Configuration.ClearDir(testHome);

            OpenSecRecnoDBWithinTxn(testFixtureName,
                "TestOpen", testHome, dbFileName,
                dbSecFileName, false);
        }

        [Test]
        public void TestOpenDBNameWithinTxn()
        {
            testName = "TestOpenDBNameWithinTxn";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testName + ".db";
            string dbSecFileName = testName + "_sec.db";

            Configuration.ClearDir(testHome);

            OpenSecRecnoDBWithinTxn(testFixtureName,
                "TestOpen", testHome, dbFileName,
                dbSecFileName, true);
        }

        public void OpenSecRecnoDBWithinTxn(string className,
            string funName, string home, string dbFileName,
            string dbSecFileName, bool ifDbName)
        {
            XmlElement xmlElem = Configuration.TestSetUp(
                className, funName);

            // Open an environment.
            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.UseTxns = true;
            envConfig.UseMPool = true;
            envConfig.UseLogging = true;
            DatabaseEnvironment env = DatabaseEnvironment.Open(
                home, envConfig);

            // Open a primary recno database.
            Transaction openDBTxn = env.BeginTransaction();
            RecnoDatabaseConfig dbConfig =
                new RecnoDatabaseConfig();
            dbConfig.Creation = CreatePolicy.IF_NEEDED;

            dbConfig.Env = env;
            RecnoDatabase db = RecnoDatabase.Open(
                dbFileName, dbConfig, openDBTxn);
            openDBTxn.Commit();

            // Open a secondary recno database.
            Transaction openSecTxn = env.BeginTransaction();
            SecondaryRecnoDatabaseConfig secDBConfig =
                new SecondaryRecnoDatabaseConfig(db,
                new SecondaryKeyGenDelegate(SecondaryKeyGen));
            SecondaryRecnoDatabaseConfigTest.Config(xmlElem,
                ref secDBConfig, false);
            secDBConfig.Env = env;
            SecondaryRecnoDatabase secDB;
            if (ifDbName == false)
                secDB = SecondaryRecnoDatabase.Open(
                    dbSecFileName, secDBConfig, openSecTxn);
            else
                secDB = SecondaryRecnoDatabase.Open(
                    dbSecFileName, "secondary", secDBConfig,
                    openSecTxn);
            openSecTxn.Commit();

            // Confirm its flags configured in secDBConfig.
            Confirm(xmlElem, secDB, true);
            secDB.Close();

            // Open the existing secondary database.
            Transaction secTxn = env.BeginTransaction();
            SecondaryDatabaseConfig secConfig =
                new SecondaryDatabaseConfig(db,
                new SecondaryKeyGenDelegate(SecondaryKeyGen));
            secConfig.Env = env;

            SecondaryDatabase secExDB;
            if (ifDbName == false)
                secExDB = SecondaryRecnoDatabase.Open(
                    dbSecFileName, secConfig, secTxn);
            else
                secExDB = SecondaryRecnoDatabase.Open(
                    dbSecFileName, "secondary", secConfig,
                    secTxn);
            secExDB.Close();
            secTxn.Commit();
            
            db.Close();
            env.Close();
        }

        public DatabaseEntry SecondaryKeyGen(
            DatabaseEntry key, DatabaseEntry data)
        {
            DatabaseEntry dbtGen;
            dbtGen = new DatabaseEntry(data.Data);
            return dbtGen;
        }

        public static void Confirm(XmlElement xmlElem,
            SecondaryRecnoDatabase secDB, bool compulsory)
        {
            Configuration.ConfirmInt(xmlElem, "Delimiter",
                secDB.Delimiter, compulsory);
            Configuration.ConfirmUint(xmlElem, "Length",
                secDB.Length, compulsory);
            Configuration.ConfirmInt(xmlElem, "PadByte",
                secDB.PadByte, compulsory);
            Configuration.ConfirmBool(xmlElem,
                "Renumber", secDB.Renumber, compulsory);
            Configuration.ConfirmBool(xmlElem, "Snapshot",
                secDB.Snapshot, compulsory);
        }
    }
}
