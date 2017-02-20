/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.model;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.instrument.ClassFileTransformer;
import java.lang.instrument.Instrumentation;
import java.security.ProtectionDomain;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.StringTokenizer;

import com.sleepycat.asm.ClassReader;
import com.sleepycat.asm.ClassVisitor;
import com.sleepycat.asm.ClassWriter;

/**
 * Enhances the bytecode of persistent classes to provide efficient access to
 * fields and constructors, and to avoid special security policy settings for
 * accessing non-public members.  Classes are enhanced if they are annotated
 * with {@link Entity} or {@link Persistent}.
 *
 * <p>{@code ClassEnhancer} objects are thread-safe.  Multiple threads may
 * safely call the methods of a shared {@code ClassEnhancer} object.</p>
 *
 * <p>As described in the {@link <a
 * href="../package-summary.html#bytecode">package summary</a>}, bytecode
 * enhancement may be used either at runtime or offline (at build time).</p>
 *
 * <p>To use enhancement offline, this class may be used as a {@link #main main
 * program}.  It may also be used via a ClassEnhancerTask ant task: the source
 * is in the <pre>java/ext</pre> directory in Berkeley DB.</p>
 *
 * <p>For enhancement at runtime, this class provides the low level support
 * needed to transform class bytes during class loading.  To configure runtime
 * enhancement you may use one of the following approaches:</p>
 * <ol>
 * <li>The BDB {@code je-<version>.jar} or {@code db.jar} file may be used as
 * an instrumentation agent as follows:
 * <pre class="code">{@literal java -javaagent:<BDB-JAR-FILE>=enhance:packageNames ...}</pre>
 * {@code packageNames} is a comma separated list of packages containing
 * persistent classes.  Sub-packages of these packages are also searched.  If
 * {@code packageNames} is omitted then all packages known to the current
 * classloader are searched.
 * <p>The "-v" option may be included in the comma separated list to print the
 * name of each class that is enhanced.</p></li>
 * <br>
 * <li>The {@link #enhance} method may be called to implement a class loader
 * that performs enhancement.  Using this approach, it is the developer's
 * responsibility to implement and configure the class loader.</li>
 * </ol>
 *
 * @author Mark Hayes
 */
public class ClassEnhancer implements ClassFileTransformer {

    private static final String AGENT_PREFIX = "enhance:";

    private Set<String> packagePrefixes;
    private boolean verbose;

    /**
     * Enhances classes in the directories specified.  The class files are
     * replaced when they are enhanced, without changing the file modification
     * date.  For example:
     *
     * <pre class="code">java -cp je-&lt;version&gt;.jar com.sleepycat.persist.model.ClassEnhancer ./classes</pre>
     *
     * <p>The "-v" argument may be specified to print the name of each class
     * file that is enhanced.  The total number of class files enhanced will
     * always be printed.</p>
     *
     * @param args one or more directories containing classes to be enhanced.
     * Subdirectories of these directories will also be searched.  Optionally,
     * -v may be included to print the name of every class file enhanced.
     */
    public static void main(String[] args) throws Exception {
        try {
            boolean verbose = false;
            List<File> fileList = new ArrayList<File>();
            for (int i = 0; i < args.length; i += 1) {
                String arg = args[i];
                if (arg.startsWith("-")) {
                    if ("-v".equals(args[i])) {
                        verbose = true;
                    } else {
                        throw new IllegalArgumentException
                            ("Unknown arg: " + arg);
                    }
                } else {
                    fileList.add(new File(arg));
                }
            }
            ClassEnhancer enhancer = new ClassEnhancer();
            enhancer.setVerbose(verbose);
            int nFiles = 0;
            for (File file : fileList) {
                nFiles += enhancer.enhanceFile(file);
            }
            if (nFiles > 0) {
                System.out.println("Enhanced: " + nFiles + " files");
            }
        } catch (Exception e) {
            e.printStackTrace();
            throw e;
        }
    }

    /**
     * Enhances classes as specified by a JVM -javaagent argument.
     *
     * @see java.lang.instrument.Instrumentation
     */
    public static void premain(String args, Instrumentation inst) {
        if (!args.startsWith(AGENT_PREFIX)) {
            throw new IllegalArgumentException
                ("Unknown javaagent args: " + args +
                 " Args must start with: \"" + AGENT_PREFIX + '"');
        }
        args = args.substring(AGENT_PREFIX.length());
        Set<String> packageNames = null;
        boolean verbose = false;
        if (args.length() > 0) {
            packageNames = new HashSet<String>();
            StringTokenizer tokens = new StringTokenizer(args, ",");
            while (tokens.hasMoreTokens()) {
                String token = tokens.nextToken();
                if (token.startsWith("-")) {
                    if (token.equals("-v")) {
                        verbose = true;
                    } else {
                        throw new IllegalArgumentException
                            ("Unknown javaagent arg: " + token);
                    }
                } else {
                    packageNames.add(token);
                }
            }
        }
        ClassEnhancer enhancer = new ClassEnhancer(packageNames);
        enhancer.setVerbose(verbose);
        inst.addTransformer(enhancer);
    }

    /**
     * Creates a class enhancer that searches all packages.
     */
    public ClassEnhancer() {
    }

    /**
     * Sets verbose mode.
     *
     * <p>True may be specified to print the name of each class file that is
     * enhanced.  This property is false by default.</p>
     */
    public void setVerbose(boolean verbose) {
        this.verbose = verbose;
    }

    /**
     * Gets verbose mode.
     *
     * @see #setVerbose
     */
    public boolean getVerbose() {
        return verbose;
    }

    /**
     * Creates a class enhancer that searches a given set of packages.
     *
     * @param packageNames a set of packages to search for persistent
     * classes.  Sub-packages of these packages are also searched.  If empty or
     * null, all packages known to the current classloader are searched.
     */
    public ClassEnhancer(Set<String> packageNames) {
        if (packageNames != null) {
            packagePrefixes = new HashSet<String>();
            for (String name : packageNames) {
                packagePrefixes.add(name + '.');
            }
        }
    }

    public byte[] transform(ClassLoader loader,
                            String className,
                            Class<?> classBeingRedefined,
                            ProtectionDomain protectionDomain,
                            byte[] classfileBuffer) {
        className = className.replace('/', '.');
        byte[] bytes = enhance(className, classfileBuffer);
        if (verbose && bytes != null) {
            System.out.println("Enhanced: " + className);
        }
        return bytes;
    }

    /**
     * Enhances the given class bytes if the class is annotated with {@link
     * Entity} or {@link Persistent}.
     *
     * @param className the class name in binary format; for example,
     * "my.package.MyClass$Name", or null if no filtering by class name
     * should be performed.
     *
     * @param classBytes are the class file bytes to be enhanced.
     *
     * @return the enhanced bytes, or null if no enhancement was performed.
     */
    public byte[] enhance(String className, byte[] classBytes) {
        if (className != null && packagePrefixes != null) {
            for (String prefix : packagePrefixes) {
                if (className.startsWith(prefix)) {
                    return enhanceBytes(classBytes);
                }
            }
            return null;
        } else {
            return enhanceBytes(classBytes);
        }
    }

    int enhanceFile(File file)
        throws IOException {

        int nFiles = 0;
        if (file.isDirectory()) {
            String[] names = file.list();
            if (names != null) {
                for (int i = 0; i < names.length; i += 1) {
                    nFiles += enhanceFile(new File(file, names[i]));
                }
            }
        } else if (file.getName().endsWith(".class")) {
            byte[] newBytes = enhanceBytes(readFile(file));
            if (newBytes != null) {
                long modified = file.lastModified();
                writeFile(file, newBytes);
                file.setLastModified(modified);
                nFiles += 1;
                if (verbose) {
                    System.out.println("Enhanced: " + file);
                }
            }
        }
        return nFiles;
    }

    private byte[] readFile(File file)
        throws IOException {

        byte[] bytes = new byte[(int) file.length()];
        FileInputStream in = new FileInputStream(file);
        try {
            in.read(bytes);
        } finally {
            in.close();
        }
        return bytes;
    }

    private void writeFile(File file, byte[] bytes)
        throws IOException {

        FileOutputStream out = new FileOutputStream(file);
        try {
            out.write(bytes);
        } finally {
            out.close();
        }
    }

    private byte[] enhanceBytes(byte[] bytes) {

        /*
         * The writer is at the end of the visitor chain.  Pass true to
         * calculate stack size, for safety.
         */
        ClassWriter writer = new ClassWriter(true);
        ClassVisitor visitor = writer;

        /* The enhancer is at the beginning of the visitor chain. */
        visitor = new BytecodeEnhancer(visitor);

        /* The reader processes the class and invokes the visitors. */
        ClassReader reader = new ClassReader(bytes);
        try {

            /*
             * Pass false for skipDebug since we are rewriting the class and
             * should include all information.
             */
            reader.accept(visitor, false);
            return writer.toByteArray();
        } catch (BytecodeEnhancer.NotPersistentException e) {
            /* The class is not persistent and should not be enhanced. */
            return null;
        }
    }
}
