/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 * $Id$
 */
using System;
using System.Collections.Generic;
using System.Text;
using BerkeleyDB;

namespace ex_lock {
    class Program {
        const int EXIT_FAILURE = 1;
        const int EXIT_SUCCESS = 0;

        static void Main(string[] args) {
            DatabaseEntry data;
            DatabaseEnvironment env;
            List<Lock> lockList;
            Lock lk;
            LockMode mode;
            string buff;
            string home;
            string progName;
            int lockNo;
            uint doUnLink;
            uint locker;
            uint maxLock;

            try {
                home = args[0];
                maxLock = uint.Parse(args[1]);
                doUnLink = uint.Parse(args[2]);
            } catch {
                Usage();
                return;
            }

            data = new DatabaseEntry();
            lockList = new List<Lock>();
            progName = "ex_csharp_lock";

            /* Initialize the database environment. */
            if (DBInit(out env, home, progName, maxLock, doUnLink) == EXIT_FAILURE)
                return;

            /*
             * Accept lock requests. 
             */
            try {
                locker = env.CreateLockerID();
            } catch (Exception e) {
                Console.WriteLine("{0}:{1}\n{2}",
                    e.Source, e.Message, e.StackTrace);
                env.Close();
                return;
            }

            while (true) {
                /* Choose getting or releasing lock. */
                Console.WriteLine("Operation get/release/quit: ");
                buff = Console.ReadLine();
                if (buff == "get") {
                    /* Input the content to be locked. */
                    Console.WriteLine("Input text string to lock");
                    try {
                        buff = Console.ReadLine();
                        DbtFromString(data, buff);
                    } catch {
                        Console.WriteLine("Input fails");
                        continue;
                    }

                    /* 
                     * Choose the locker's mode. More lock modes
                     * could be provided. Only support read/write 
                     * mode here.
                     */
                    while (true) {
                        Console.WriteLine("read or write");
                        buff = Console.ReadLine();
                        if (buff == "read" || buff == "write")
                            break;
                    }
                    if (buff == "write")
                        mode = LockMode.WRITE;
                    else
                        mode = LockMode.READ;

                    /* Get lock and add it to the list of locks. */
                    try {
                        lk = env.GetLock(locker, false, data, mode);
                    } catch (Exception e) {
                        Console.WriteLine("{0}:{1}\n{2}",
                            e.Source, e.Message, e.StackTrace);
                        env.Close();
                        return;
                    }

                    Console.WriteLine("Lock #{0} granted",
                        lockList.Count);
                    lockList.Add(lk);
                } else if (buff == "release") {
                    /* 
                     * Release a lock. 
                     */
                    while (true) {
                        /* Input lock number to release. */
                        Console.WriteLine(
                            "Input lock number to release");
                        buff = Console.ReadLine();
                        try {
                            lockNo = int.Parse(buff);
                            if (lockNo > 0 && lockNo <
                                lockList.Count)
                                break;
                            else
                                Console.WriteLine(
                                    "Lock number is out of range");
                        } catch {
                            Console.WriteLine(
                                "Not a valid lock number");
                        }

                    }

                    /* Release a lock and remove it from the list of locks. */
                    try {
                        env.PutLock(lockList[lockNo]);
                    } catch (Exception e) {
                        Console.WriteLine("{0}:{1}\n{2}",
                            e.Source, e.Message, e.StackTrace);
                        env.Close();
                        return;
                    }

                    lockList.Remove(lockList[lockNo]);
                } else if (buff == "quit") {
                    break;
                }
            }

            /*Free locker and close the environment. */
            env.FreeLockerID(locker);
            env.Close();
        }

        /*
         * Create and open environment and database.
         */
        public static int DBInit(out DatabaseEnvironment env,
            string home, string progName, uint maxLock,
            uint doUnLink) {
            DatabaseEnvironmentConfig envConfig;
            LockingConfig lkConfig;

            /* Configure locking subsystem. */
            lkConfig = new LockingConfig();
            lkConfig.MaxLocks = maxLock;

            /* Configure environment. */
            envConfig = new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.ErrorPrefix = progName;
            envConfig.LockSystemCfg = lkConfig;
            envConfig.UseLocking = true;

            /* 
             * Optionally remove the environment region and
             * open the environment. 
             */
            try {
                if (doUnLink == 1)
                    DatabaseEnvironment.Remove(home, true);
                env = DatabaseEnvironment.Open(home, envConfig);
            } catch (Exception e) {
                Console.WriteLine("{0}:{1}\n{2}",
                    e.Source, e.Message, e.StackTrace);
                env = null;
                return EXIT_FAILURE;
            }

            /*
            try
            {
                env = DatabaseEnvironment.Open(home, envConfig);
            }
            catch(Exception e)
            {
                Console.WriteLine("{0}:{1}\n{2}",
                    e.Source, e.Message, e.StackTrace);
                env = null;
                return ExConstants.EXIT_FAILURE;
            }
            */

            return EXIT_SUCCESS;
        }

        #region Utilities
        /* Show the usage of the example. */
        public static void Usage() {
            Console.WriteLine(
                "Usage: [home] [max lock] [1 doUnlink | 0]");
        }

        /* Get dbt from a string. */
        static void DbtFromString(DatabaseEntry dbt, string s) {
            dbt.Data = System.Text.Encoding.ASCII.GetBytes(s);
        }
        #endregion Utilities
    }
}
