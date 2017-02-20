/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.model;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.DirectoryScanner;
import org.apache.tools.ant.Task;
import org.apache.tools.ant.types.FileSet;

/**
 * An {@code ant} task for running the {@link ClassEnhancer}.
 *
 * <p>{@code ClassEnhancerTask} objects are thread-safe.  Multiple threads may
 * safely call the methods of a shared {@code ClassEnhancerTask} object.</p>
 *
 * <p>Note that in the BDB Java Edition product, the {@code ClassEnhancerTask}
 * class is included in {@code je-<version>.jar}.  However, in the BDB
 * (C-based) product, it is not included in {@code db.jar} because the build is
 * not dependent on the Ant libraries.  Therefore, in the BDB product, the
 * application must compile the {@code
 * java/src/com/sleepycat/persist/model/ClassEnhancerTask.java} source file and
 * ensure that the compiled class is available to the Ant task.  For example
 * the following Ant task definitions could be used.</p>
 *
 * <p>For BDB Java Edition product:</p>
 * <pre class="code">
 * {@literal <taskdef name="enhance-persistent-classes"}
 *          {@literal classname="com.sleepycat.persist.model.ClassEnhancerTask"}
 *          {@literal classpath="${je.home}/lib/je-<version>.jar"/>}</pre>
 * 
 * <p>For BDB (C-based Edition) product:</p>
 * <pre class="code">
 * {@literal <taskdef name="enhance-persistent-classes"}
 *          {@literal classname="com.sleepycat.persist.model.ClassEnhancerTask"}
 *          {@literal classpath="/path-to-jar/db.jar:/path-to-ClassEnhancerTask-class"/>}</pre>
 *
 * <p>The class enhancer task element has no attributes.  It may contain one or
 * more nested {@code fileset} elements specifying the classes to be enhanced.
 * The class files are replaced when they are enhanced, without changing the
 * file modification date.  For example:</p>
 *
 * <pre class="code">
 * {@literal <target name="main">}
 *     {@literal <enhance-persistent-classes verbose="no">}
 *         {@literal <fileset dir="classes"/>}
 *     {@literal </enhance-persistent-classes>}
 * {@literal </target>}</pre>
 *
 * <p>The verbose attribute may be specified as "true", "yes" or "on" (like
 * other Ant boolean attributes) to print the name of each class file that is
 * enhanced.  The total number of class files enhanced will always be
 * printed.</p>
 *
 * @author Mark Hayes
 */
public class ClassEnhancerTask extends Task {

    private List<FileSet> fileSets = new ArrayList<FileSet>();
    private boolean verbose;

    public void execute() throws BuildException {
        if (fileSets.size() == 0) {
            throw new BuildException("At least one fileset must be specified");
        }
        try {
            int nFiles = 0;
            ClassEnhancer enhancer = new ClassEnhancer();
            enhancer.setVerbose(verbose);
            for (FileSet fileSet : fileSets) {
                DirectoryScanner scanner =
                    fileSet.getDirectoryScanner(getProject());
                String[] fileNames = scanner.getIncludedFiles();
                for (String fileName : fileNames) {
                    File file = new File(scanner.getBasedir(), fileName);
                    try {
                        nFiles += enhancer.enhanceFile(file);
                    } catch (IOException e) {
                        throw new BuildException(e);
                    }
                }
            }
            if (nFiles > 0) {
                System.out.println("Enhanced: " + nFiles + " files");
            }
        } catch (RuntimeException e) {
            e.printStackTrace();
            throw e;
        }
    }

    public void addConfiguredFileset(FileSet files) {
        fileSets.add(files);
    }

    public void setVerbose(boolean verbose) {
        this.verbose = verbose;
    }
}
