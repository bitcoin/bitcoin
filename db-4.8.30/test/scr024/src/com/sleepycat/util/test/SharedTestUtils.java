/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.util.test;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import junit.framework.TestCase;

import com.sleepycat.db.DatabaseConfig;

/**
 * Test utility methods shared by JE and DB core tests.  Collections and
 * persist package test are used in both JE and DB core.
 */
public class SharedTestUtils {

    /* Common system properties for running tests */
    public static String DEST_DIR = "testdestdir";
    public static String NO_SYNC = "txnnosync";
    public static String LONG_TEST =  "longtest";

    public static final DatabaseConfig DBCONFIG_CREATE = new DatabaseConfig();
    static {
        DBCONFIG_CREATE.setAllowCreate(true);
    }

    private static File getTestDir() {
        String dir = System.getProperty(DEST_DIR);
        if (dir == null || dir.length() == 0) {
            throw new IllegalArgumentException
                ("System property must be set to test data directory: " +
                 DEST_DIR);
        }
        return new File(dir);
    }

    /**
     * @return true if long running tests are enabled via setting the system
     * property longtest=true.
     */
    public static boolean runLongTests() {
        String longTestProp =  System.getProperty(LONG_TEST);
        if ((longTestProp != null)  &&
            longTestProp.equalsIgnoreCase("true")) {
            return true;
        } else {
            return false;
        }
    }

    public static void printTestName(String name) {
        // don't want verbose printing for now
        // System.out.println(name);
    }

    public static File getExistingDir(String name) {
        File dir = new File(getTestDir(), name);
        if (!dir.exists() || !dir.isDirectory()) {
            throw new IllegalStateException(
                    "Not an existing directory: " + dir);
        }
        return dir;
    }

    public static File getNewDir() {
        return getNewDir("test-dir");
    }

    public static void emptyDir(File dir) {
        if (dir.isDirectory()) {
            String[] files = dir.list();
            if (files != null) {
                for (int i = 0; i < files.length; i += 1) {
                    new File(dir, files[i]).delete();
                }
            }
        } else {
            dir.delete();
            dir.mkdirs();
        }
    }

    public static File getNewDir(String name) {
        File dir = new File(getTestDir(), name);
        emptyDir(dir);
        return dir;
    }

    public static File getNewFile() {
        return getNewFile("test-file");
    }

    public static File getNewFile(String name) {
        return getNewFile(getTestDir(), name);
    }

    public static File getNewFile(File dir, String name) {
        File file = new File(dir, name);
        file.delete();
        return file;
    }

    public static boolean copyResource(Class cls, String fileName, File toDir)
        throws IOException {

        InputStream in = cls.getResourceAsStream("testdata/" + fileName);
        if (in == null) {
            return false;
        }
        in = new BufferedInputStream(in);
        File file = new File(toDir, fileName);
        OutputStream out = new FileOutputStream(file);
        out = new BufferedOutputStream(out);
        int c;
        while ((c = in.read()) >= 0) out.write(c);
        in.close();
        out.close();
        return true;
    }

    public static String qualifiedTestName(TestCase test) {

        String s = test.getClass().getName();
        int i = s.lastIndexOf('.');
        if (i >= 0) {
            s = s.substring(i + 1);
        }
        return s + '.' + test.getName();
    }

    /**
     * Copies all files in fromDir to toDir.  Does not copy subdirectories.
     */
    public static void copyFiles(File fromDir, File toDir)
        throws IOException {

        String[] names = fromDir.list();
        if (names != null) {
            for (int i = 0; i < names.length; i += 1) {
                File fromFile = new File(fromDir, names[i]);
                if (fromFile.isDirectory()) {
                    continue;
                }
                File toFile = new File(toDir, names[i]);
                int len = (int) fromFile.length();
                byte[] data = new byte[len];
                FileInputStream fis = null;
                FileOutputStream fos = null;
                try {
                    fis = new FileInputStream(fromFile);
                    fos = new FileOutputStream(toFile);
                    fis.read(data);
                    fos.write(data);
                } finally {
                    if (fis != null) {
                        fis.close();
                    }
                    if (fos != null) {
                        fos.close();
                    }
                }
            }
        }
    }
}
