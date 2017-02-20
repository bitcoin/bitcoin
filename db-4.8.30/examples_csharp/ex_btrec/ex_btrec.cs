/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 */
using System;
using System.IO;
using System.Collections.Generic;
using System.Collections;
using System.Diagnostics;
using System.Text;
using System.Runtime.Serialization.Formatters.Binary;
using BerkeleyDB;

namespace ex_btrec {
    class Program {
        static void Main(string[] args) {
            string dbFileName;
            string dbName;
            string wordFile;

            /*
             * ex_btrec is meant to be run from build_windows\AnyCPU, in either
             * the Debug or Release directory. The required core libraries,
             * however, are in either build_windows\Win32 or build_windows\x64,
             * depending upon the platform.  That location needs to be added to
             * the PATH environment variable for the P/Invoke calls to work.
             */
            try {
                String pwd = Environment.CurrentDirectory;
                pwd = Path.Combine(pwd, "..");
                pwd = Path.Combine(pwd, "..");
                if (IntPtr.Size == 4)
                    pwd = Path.Combine(pwd, "Win32");
                else
                    pwd = Path.Combine(pwd, "x64");
#if DEBUG
                pwd = Path.Combine(pwd, "Debug");
#else
                pwd = Path.Combine(pwd, "Release");
#endif
                pwd += ";" + Environment.GetEnvironmentVariable("PATH");
                Environment.SetEnvironmentVariable("PATH", pwd);
            } catch (Exception e) {
                Console.WriteLine(
                    "Unable to set the PATH environment variable.");
                Console.WriteLine(e.Message);
                return;
            }
            
            try {
                dbFileName = args[0];
                dbName = args[1];
                wordFile = args[2];
            } catch {
                usage();
                return;
            }

            ex_csharp_btrec(dbFileName, dbName, wordFile);
        }

        public static int ex_csharp_btrec(string dbFileName,
            string dbName, string wordFile) {
            BTreeCursor dbc;
            BTreeDatabase btreeDB;
            BTreeDatabaseConfig btreeDBconfig;
            DatabaseEntry key, data;
            KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
            StreamReader wordList;
            string buff;
            char[] rbuff;
            int i, j;
            uint cnt;

            string progName = "ex_csharp_btrec";

            /* Check if word list exists. */
            if (!File.Exists(wordFile)) {
                Console.WriteLine("Cannot find {0}/{1}.",
                    Environment.CurrentDirectory, wordFile);
                return (1);
            }

            /* Optionally remove the previous database. */
            if (File.Exists(dbFileName)) {
                while (true) {
                    Console.Write
                        ("Database already exists, delete or not (y/n)?");
                    buff = Console.ReadLine();
                    if (buff == "y" || buff == "n")
                        break;
                }

                if (buff == "y") {
                    File.Delete(dbFileName);
                    Console.WriteLine("The existing {0} is deleted",
                        dbName);
                }
            }

            /* Create and initialize database object, open the database. */
            btreeDBconfig = new BTreeDatabaseConfig();
            btreeDBconfig.Creation = CreatePolicy.IF_NEEDED;
            btreeDBconfig.ErrorPrefix = progName;
            btreeDBconfig.UseRecordNumbers = true;
            try {
                btreeDB = BTreeDatabase.Open(dbFileName, dbName,
                    btreeDBconfig);
            } catch {
                Console.WriteLine("Database can not be opened");
                return (1);
            }

            /*
             * Insert records into the database, where the key is the word
             * preceded by its record number, and the data is the same,
             * but in reverse order.
             */
            key = new DatabaseEntry();
            data = new DatabaseEntry();
            wordList = new StreamReader(wordFile);
            for (cnt = 1; cnt <= 1000; cnt++) {
                /* Read one word from word list. */
                try {
                    buff = wordList.ReadLine();
                } catch {
                    Console.WriteLine("Error when reading word list");
                    btreeDB.Close();
                    wordList.Close();
                    return (1);
                }

                /* 
                 * Prefix the word with record number and store
                 * them in key.
                 */
                try {
                    buff = cnt.ToString("0000") + buff;
                } catch {
                    Console.WriteLine(
                        "Wrong format for record number.");
                    btreeDB.Close();
                    wordList.Close();
                    return (1);
                }
                dbtFromString(key, buff);

                /* Reverse buff to rbuff and store them in data. */
                rbuff = new char[buff.Length];
                for (i = 0, j = buff.Length - 1; i < buff.Length; i++, j--)
                    rbuff[i] = buff[j];
                dbtFromString(data, new string(rbuff));

                /* Put the key/data pair into database. */
                try {
                    btreeDB.PutNoOverwrite(key, data);
                } catch {
                    Console.WriteLine("Put error with key::{0} and"
                        + "data::{1}", buff, new string(rbuff));
                    btreeDB.Close();
                    wordList.Close();
                    return (1);
                }
            }

            /* Close the word list. */
            wordList.Close();

            /* Acquire a cursor for the database. */
            dbc = btreeDB.Cursor();

            /* 
             * Input an available record number and get its 
             * corresponding key/data pair in the database. 
             */
            while (true) {
                Console.WriteLine("Please input record number"
                    + ", input 0 to stop");

                while (true) {
                    Console.WriteLine("recno #:\t");
                    buff = Console.ReadLine();

                    try {
                        cnt = uint.Parse(buff);
                        break;
                    } catch {
                        Console.WriteLine("Invalid Record Number. "
                            + "Try Again.");
                    }
                }

                if (cnt == 0)
                    break;

                /*
                 * Position the cursor at a key/data pair in the database 
                 * where the record number points to.
                 */
                if (!dbc.Move(cnt)) {
                    Console.WriteLine("No record is found");
                    break;
                }

                /* Get the current key/data pair and display them.*/
                pair = dbc.Current;
                Console.WriteLine("key::{0, 18:G}    data::{1, 18:G}",
                    strFromDBT(pair.Key), strFromDBT(pair.Value));

                /* Check if it's the right record number. */
                if (dbc.Recno() != cnt) {
                    Console.WriteLine("The current record is not"
                        + "the right one of the given record number.");
                    btreeDB.Close();
                    dbc.Close();
                    return (1);
                }

                /* Get the next record. */
                if (!dbc.MoveNext()) {
                    Console.WriteLine(
                        "Next record is not found");
                    break;
                }

                /* Get the current key/data pair and display them.*/
                pair = dbc.Current;
                Console.WriteLine("key::{0, 18:G}    data::{1, 18:G}\n",
                    strFromDBT(pair.Key), strFromDBT(pair.Value));

            }

            /* Close cursor and database. */
            dbc.Close();
            btreeDB.Close();

            return (0);
        }

        #region Utilities
        /* Show the usage. */
        public static void usage() {
            Console.WriteLine(
                "Usage: [db file name]  [db name] [word list file]");
        }

        /* Get dbt from string. */
        static void dbtFromString(DatabaseEntry dbt, string s) {
            dbt.Data = System.Text.Encoding.ASCII.GetBytes(s);
        }

        /* Get string from dbt. */
        public static string strFromDBT(DatabaseEntry dbt) {

            System.Text.ASCIIEncoding decode =
                new ASCIIEncoding();
            return decode.GetString(dbt.Data);
        }
        #endregion Utilities
    }
}