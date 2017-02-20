/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;

/**
Specifies the attributes of a verification operation.
*/
public class VerifyConfig {
    /**
    Default configuration used if null is passed to
    {@link com.sleepycat.db.Database#verify Database.verify}.
    */
    public static final VerifyConfig DEFAULT = new VerifyConfig();

    /* package */
    static VerifyConfig checkNull(VerifyConfig config) {
        return (config == null) ? DEFAULT : config;
    }

    private boolean aggressive = false;
    private boolean noOrderCheck = false;
    private boolean orderCheckOnly = false;
    private boolean salvage = false;
    private boolean printable = false;

    /**
    An instance created using the default constructor is initialized
    with the system's default settings.
    */
    public VerifyConfig() {
    }

    /**
    Configure {@link com.sleepycat.db.Database#verify Database.verify} to output <b>all</b> the
    key/data pairs in the file that can be found.
    <p>
    By default, {@link com.sleepycat.db.Database#verify Database.verify} does not assume corruption.
    For example, if a key/data pair on a page is marked as deleted, it
    is not then written to the output file.  When {@link com.sleepycat.db.Database#verify Database.verify} is configured with this method, corruption is assumed, and
    any key/data pair that can be found is written.  In this case,
    key/data pairs that are corrupted or have been deleted may appear
    in the output (even if the file being salvaged is in no way
    corrupt), and the output will almost certainly require editing
    before being loaded into a database.
    <p>
    @param aggressive
    If true, configure {@link com.sleepycat.db.Database#verify Database.verify} to output <b>all</b>
    the key/data pairs in the file that can be found.
    */
    public void setAggressive(final boolean aggressive) {
        this.aggressive = aggressive;
    }

    /**
Return true if the     {@link com.sleepycat.db.Database#verify Database.verify} is configured to output
    <b>all</b> the key/data pairs in the file that can be found.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the     {@link com.sleepycat.db.Database#verify Database.verify} is configured to output
    <b>all</b> the key/data pairs in the file that can be found.
    */
    public boolean getAggressive() {
        return aggressive;
    }

    /**
    Configure {@link com.sleepycat.db.Database#verify Database.verify} to skip the database checks for
    btree and duplicate sort order and for hashing.
    <p>
    {@link com.sleepycat.db.Database#verify Database.verify} normally verifies that btree keys and
    duplicate items are correctly sorted, and hash keys are correctly
    hashed.  If the file being verified contains multiple databases
    using differing sorting or hashing algorithms, some of them must
    necessarily fail database verification because only one sort order
    or hash function can be specified before {@link com.sleepycat.db.Database#verify Database.verify}
    is called.  To verify files with multiple databases having differing
    sorting orders or hashing functions, first perform verification of
    the file as a whole using {@link com.sleepycat.db.Database#verify Database.verify} configured with
    {@link com.sleepycat.db.VerifyConfig#setNoOrderCheck VerifyConfig.setNoOrderCheck}, and then individually verify
    the sort order and hashing function for each database in the file
    using 4_link(Database, verify) configured with {@link com.sleepycat.db.VerifyConfig#setOrderCheckOnly VerifyConfig.setOrderCheckOnly}.
    <p>
    @param noOrderCheck
    If true, configure {@link com.sleepycat.db.Database#verify Database.verify} to skip the database
    checks for btree and duplicate sort order and for hashing.
    */
    public void setNoOrderCheck(final boolean noOrderCheck) {
        this.noOrderCheck = noOrderCheck;
    }

    /**
Return true if the     {@link com.sleepycat.db.Database#verify Database.verify} is configured to skip the
    database checks for btree and duplicate sort order and for hashing.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the     {@link com.sleepycat.db.Database#verify Database.verify} is configured to skip the
    database checks for btree and duplicate sort order and for hashing.
    */
    public boolean getNoOrderCheck() {
        return printable;
    }

    /**
    Configure {@link com.sleepycat.db.Database#verify Database.verify} to do database checks for btree
    and duplicate sort order and for hashing, skipped by verification
    operations configured by {@link com.sleepycat.db.VerifyConfig#setNoOrderCheck VerifyConfig.setNoOrderCheck}.
    <p>
    When this flag is specified, a database name must be specified to
    {@link com.sleepycat.db.Database#verify Database.verify}, indicating the database in the physical
    file which is to be checked.
    <p>
    This configuration is only safe to use on databases that have
    already successfully been verified with {@link com.sleepycat.db.VerifyConfig#setNoOrderCheck VerifyConfig.setNoOrderCheck} configured.
    <p>
    @param orderCheckOnly
    If true, configure {@link com.sleepycat.db.Database#verify Database.verify} to do database checks
    for btree and duplicate sort order and for hashing, skipped by
    verification operations configured by {@link com.sleepycat.db.VerifyConfig#setNoOrderCheck VerifyConfig.setNoOrderCheck}.
    */
    public void setOrderCheckOnly(final boolean orderCheckOnly) {
        this.orderCheckOnly = orderCheckOnly;
    }

    /**
Return true if the     {@link com.sleepycat.db.Database#verify Database.verify} is configured to do database
    checks for btree and duplicate sort order and for hashing, skipped
    by verification operations configured by {@link com.sleepycat.db.VerifyConfig#setNoOrderCheck VerifyConfig.setNoOrderCheck}.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the     {@link com.sleepycat.db.Database#verify Database.verify} is configured to do database
    checks for btree and duplicate sort order and for hashing, skipped
    by verification operations configured by {@link com.sleepycat.db.VerifyConfig#setNoOrderCheck VerifyConfig.setNoOrderCheck}.
    */
    public boolean getOrderCheckOnly() {
        return orderCheckOnly;
    }

    /**
    Configure {@link com.sleepycat.db.Database#verify Database.verify} to use printing characters to
    where possible.
    <p>
    This method is only meaningful when combined with
    {@link com.sleepycat.db.VerifyConfig#setSalvage VerifyConfig.setSalvage}.
    <p>
    This configuration permits users to use standard text editors and
    tools to modify the contents of databases or selectively remove data
    from salvager output.
    <p>
    Note: different systems may have different notions about what characters
    are considered <em>printing characters</em>, and databases dumped in
    this manner may be less portable to external systems.
    <p>
    @param printable
    If true, configure {@link com.sleepycat.db.Database#verify Database.verify} to use printing
    characters to where possible.
    */
    public void setPrintable(final boolean printable) {
        this.printable = printable;
    }

    /**
Return true if the     {@link com.sleepycat.db.Database#verify Database.verify} is configured to use printing
    characters to where possible.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the     {@link com.sleepycat.db.Database#verify Database.verify} is configured to use printing
    characters to where possible.
    */
    public boolean getPrintable() {
        return printable;
    }

    /**
    Configure {@link com.sleepycat.db.Database#verify Database.verify} to write the key/data pairs from
    all databases in the file to the file stream named by the outfile
    parameter.
    <p>
    The output format is the same as that specified for the db_dump
    utility, and can be used as input for the db_load utility.
    <p>
    Because the key/data pairs are output in page order as opposed to the
    sort order used by db_dump, using {@link com.sleepycat.db.Database#verify Database.verify} to dump
    key/data pairs normally produces less than optimal loads for Btree
    databases.
    <p>
    @param salvage
    If true, configure {@link com.sleepycat.db.Database#verify Database.verify} to write the key/data
    pairs from all databases in the file to the file stream named by the
    outfile parameter.
    */
    public void setSalvage(final boolean salvage) {
        this.salvage = salvage;
    }

    /**
Return true if the     {@link com.sleepycat.db.Database#verify Database.verify} is configured to write the
    key/data pairs from all databases in the file to the file stream
    named by the outfile parameter..
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the     {@link com.sleepycat.db.Database#verify Database.verify} is configured to write the
    key/data pairs from all databases in the file to the file stream
    named by the outfile parameter..
    */
    public boolean getSalvage() {
        return salvage;
    }

    int getFlags() {
        int flags = 0;
        flags |= aggressive ? DbConstants.DB_AGGRESSIVE : 0;
        flags |= noOrderCheck ? DbConstants.DB_NOORDERCHK : 0;
        flags |= orderCheckOnly ? DbConstants.DB_ORDERCHKONLY : 0;
        flags |= salvage ? DbConstants.DB_SALVAGE : 0;
        flags |= printable ? DbConstants.DB_PRINTABLE : 0;

        return flags;
    }
}
