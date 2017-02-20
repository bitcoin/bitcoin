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

namespace ex_sequence {
    class Program {
        static void Main(string[] args) {
            BTreeDatabase btreeDB;
            BTreeDatabaseConfig btreeDBConfig;
            DatabaseEnvironment env;
            DatabaseEnvironmentConfig envConfig;
            Sequence seq;
            SequenceConfig seqConfig;
            string buff;
            string dbFileName;
            string home;
            string progName;

            /*
             * ex_sequence is meant to be run from build_windows\AnyCPU, in
             * either the Debug or Release directory. The required core
             * libraries, however, are in either build_windows\Win32 or
             * build_windows\x64, depending upon the platform.  That location
             * needs to be added to the PATH environment variable for the
             * P/Invoke calls to work.
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
            
            progName = "ex_csharp_sequence";
            try {
                home = args[0];
                dbFileName = args[1];
            } catch {
                Usage();
                return;
            };

            /* Optiionally remove the existing database file. */
            if (File.Exists(dbFileName)) {
                while (true) {
                    Console.Write
                        ("File already exists, delete or not (y/n)?");
                    buff = Console.ReadLine();
                    if (buff == "y" || buff == "n")
                        break;
                }

                if (buff == "y") {
                    File.Delete(dbFileName);
                    Console.WriteLine("The existing {0} is deleted",
                        dbFileName);
                }
            }

            /* Configure and open environment. */
            envConfig = new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.UseLogging = true;
            envConfig.UseMPool = true;
            envConfig.UseTxns = true;
            try {
                env = DatabaseEnvironment.Open(home, envConfig);
            } catch (Exception e) {
                Console.WriteLine("{0}:{1}\n{2}",
                    e.Source, e.Message, e.StackTrace);
                return;
            }

            /* Configure and open sequence's database. */
            btreeDBConfig = new BTreeDatabaseConfig();
            btreeDBConfig.AutoCommit = true;
            btreeDBConfig.Creation = CreatePolicy.IF_NEEDED;
            btreeDBConfig.ErrorPrefix = progName;
            btreeDBConfig.Env = env;
            try {
                btreeDB = BTreeDatabase.Open(dbFileName,
                    btreeDBConfig);
            } catch (Exception e) {
                Console.WriteLine("{0}:{1}\n{2}",
                    e.Source, e.Message, e.StackTrace);
                return;
            }

            /* Configure and initialize sequence. */
            seqConfig = new SequenceConfig();
            seqConfig.BackingDatabase = btreeDB;
            seqConfig.Creation = CreatePolicy.IF_NEEDED;
            seqConfig.Increment = true;
            seqConfig.InitialValue = Int64.MaxValue;
            seqConfig.key = new DatabaseEntry();
            seqConfig.SetRange(Int64.MinValue, Int64.MaxValue);
            seqConfig.Wrap = true;
            DbtFromString(seqConfig.key, "ex_csharp_sequence");
            seq = new Sequence(seqConfig);

            /* Get from sequence. */
            for (int i = 0; i < 10; i++) {
                Console.WriteLine("{0}", seq.Get(2));
                Console.ReadLine();
            }

            Console.WriteLine("Sequence Name: {0}",
                seq.BackingDatabase.FileName);
            Console.WriteLine("Sequence Key: {0}",
                StrFromDBT(seq.Key));
            Console.WriteLine("{0}->{1}", seq.Min, seq.Max);
            Console.WriteLine(seq.Wrap ? "wrap" : "not wrap");
            Console.WriteLine(seq.Increment ? "increase" :
                "decrease");

            /* Close sequence, database and environment. */
            seq.Close();
            btreeDB.Close();
            env.Close();
        }

        #region Utilities
        public static void DbtFromString(DatabaseEntry dbt, string s) {
            dbt.Data = System.Text.Encoding.ASCII.GetBytes(s);
        }

        public static string StrFromDBT(DatabaseEntry dbt) {

            System.Text.ASCIIEncoding decode =
                new ASCIIEncoding();
            return decode.GetString(dbt.Data);
        }

        public static void Usage() {
            Console.WriteLine("Usage: [home] [db file name]");
        }
        #endregion Utilities
    }

}