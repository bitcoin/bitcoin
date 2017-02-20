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

namespace ex_env {
    class Program {
        const int EXIT_FAILURE = 1;
        const int EXIT_SUCCESS = 0;
        const string progName = "ex_csharp_env";
        static void Main(string[] args) {
            string data_dir, home;

            /*
             * ex_env is meant to be run from build_windows\AnyCPU, in either
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
            
            data_dir = home = null;
            switch (args.Length) {
                case 0:
                    data_dir = "envData";
                    home = "envHome";
                    break;
                case 2:
                    home = args[0];
                    data_dir = args[1];
                    break;
                case 1:
                    home = args[0];
                    break;
                default:
                    usage();
                    return;
            }

            if (!Directory.Exists(home)) {
                Console.WriteLine("Creating home directory: {0}", home);
                try {
                    Directory.CreateDirectory(home);
                } catch {
                    Console.WriteLine("Unable to create {0}", home);
                    return;
                }
            }
            if (data_dir != null && !Directory.Exists(home + "/" + data_dir)) {
                Console.WriteLine(
                    "Creating home directory: {0}/{1}", home, data_dir);
                try {
                    Directory.CreateDirectory(home + "/" + data_dir);
                } catch {
                    Console.WriteLine(
                        "Unable to create {0}/{1}", home, data_dir);
                    return;
                }
            }

            /* Set up environment. */
            if (SetUpEnv(home, data_dir) == EXIT_FAILURE) {
                Console.WriteLine("Fail to set up the environment.");
                return;
            }
            Console.WriteLine("Set up the environment.");

            /* Tear down the environment and remove its files. */
            if (TearDownEnv(home) == EXIT_FAILURE) {
                Console.WriteLine(
                    "Fail to tear down the environment.");
                return;
            }
            Console.WriteLine("Tear down the environment.");
        }

        /*
         * Set up environment.
         */
        public static int SetUpEnv(string home, string data_dir) {
            DatabaseEnvironment env;
            DatabaseEnvironmentConfig envConfig;

            /* Configure an environment. */
            envConfig = new DatabaseEnvironmentConfig();
            envConfig.MPoolSystemCfg = new MPoolConfig();
            envConfig.MPoolSystemCfg.CacheSize = new CacheInfo(
                0, 64 * 1024, 1);
            envConfig.Create = true;
            envConfig.DataDirs.Add(data_dir);
            envConfig.CreationDir = data_dir;
            envConfig.ErrorPrefix = progName;
            envConfig.UseLogging = true;
            envConfig.UseLocking = true;
            envConfig.UseMPool = true;
            envConfig.UseTxns = true;

            /* Create and open the environment. */
            try {
                env = DatabaseEnvironment.Open(home, envConfig);
            } catch (Exception e) {
                Console.WriteLine("{0}", e.Message);
                return EXIT_FAILURE;
            }

            Console.ReadLine();
            env.Close();
            return EXIT_SUCCESS;
        }

        /*
         * Tear down environment and remove its files.
         * Any log or database files and the environment 
         * directory are not removed.
         */
        public static int TearDownEnv(string home) {
            /* Remove environment regions. */
            try {
                DatabaseEnvironment.Remove(home);
            } catch (Exception e) {
                Console.WriteLine("{0}: {1}\n{2}",
                    e.Source, e.Message, e.StackTrace);
                return EXIT_FAILURE;
            }

            return EXIT_SUCCESS;
        }

        public static void usage() {
            Console.WriteLine("Usage: ex_env [home] [data dir]");
        }
    }
}