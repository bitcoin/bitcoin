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
    public class DatabaseTest
    {
        public static uint getDefaultCacheSizeBytes()
        {
            uint defaultCacheSizeBytes;

            string fixtureHome = "./TestOut/DatabaseTest";
            string dbName = fixtureHome + "/" + "getDefaultCacheSizeBytes" + ".db";

            Configuration.ClearDir(fixtureHome);

            BTreeDatabaseConfig cfg = new BTreeDatabaseConfig();
            cfg.Creation = CreatePolicy.ALWAYS;
            using (BTreeDatabase db = BTreeDatabase.Open(dbName, cfg))
            {
                defaultCacheSizeBytes = db.CacheSize.Bytes;
            }

            return defaultCacheSizeBytes;
        }

        public static ByteOrder getMachineByteOrder()
        {
            string fixtureHome = "./TestOut/DatabaseTest";
            string dbName = fixtureHome + "/" + "getMachineByteOrder" + ".db";

            Configuration.ClearDir(fixtureHome);

            ByteOrder byteOrder;

            BTreeDatabaseConfig dbConfig = new BTreeDatabaseConfig();
            dbConfig.ByteOrder = ByteOrder.MACHINE;
            dbConfig.Creation = CreatePolicy.ALWAYS;
            using (BTreeDatabase db = BTreeDatabase.Open(dbName, dbConfig))
            {
                byteOrder = db.Endianness;
            }
            return byteOrder;
        }

        public static void Confirm(XmlElement xmlElement, Database db, bool compulsory)
        {
            uint defaultBytes;
            defaultBytes = getDefaultCacheSizeBytes();

            Configuration.ConfirmBool(xmlElement, "AutoCommit",
                 db.AutoCommit, compulsory);
            Configuration.ConfirmCacheSize(xmlElement, "CacheSize",
                db.CacheSize, defaultBytes, compulsory);
            Configuration.ConfirmCreatePolicy(xmlElement, "Creation",
                db.Creation, compulsory);
            Configuration.ConfirmString(xmlElement, "DatabaseName",
                db.DatabaseName, compulsory);
            Configuration.ConfirmBool(xmlElement, "DoChecksum",
                db.DoChecksum, compulsory);
            // Encrypted and EncryptWithAES?
            Configuration.ConfirmByteOrder(xmlElement, "ByteOrder",
                db.Endianness, compulsory);
            Configuration.ConfirmString(xmlElement, "ErrorPrefix",
                db.ErrorPrefix, compulsory);
            // File name is confirmed in functiion, not here.
            Configuration.ConfirmBool(xmlElement, "FreeThreaded",
                db.FreeThreaded, compulsory);
            Configuration.ConfirmBool(xmlElement, "HasMultiple",
                db.HasMultiple, compulsory);
            if (db.Endianness == getMachineByteOrder())
                Assert.IsTrue(db.InHostOrder);
            else
                Assert.IsFalse(db.InHostOrder);
            Configuration.ConfirmBool(xmlElement, "NoMMap",
                db.NoMMap, compulsory);
            Configuration.ConfirmBool(xmlElement, "NonDurableTxns",
                db.NonDurableTxns, compulsory);
            Configuration.ConfirmUint(xmlElement, "PageSize",
                db.Pagesize, compulsory);
            Configuration.ConfirmCachePriority(xmlElement,
                "Priority", db.Priority, compulsory);
            Configuration.ConfirmBool(xmlElement, "ReadOnly",
                db.ReadOnly, compulsory);
            Configuration.ConfirmBool(xmlElement, "ReadUncommitted",
                db.ReadUncommitted, compulsory);
            /*
             * Database.Truncated is the value set in
             * DatabaseConfig.Truncate.
             */
            Configuration.ConfirmBool(xmlElement, "Truncate",
                db.Truncated, compulsory);
            Configuration.ConfirmBool(xmlElement, "UseMVCC",
                db.UseMVCC, compulsory);
        }
    }

}
