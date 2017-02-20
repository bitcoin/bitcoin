/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.collections;

import com.sleepycat.compat.DbCompat;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.DeadlockException;
import com.sleepycat.db.Environment;
import com.sleepycat.db.Transaction;
import com.sleepycat.db.TransactionConfig;
import com.sleepycat.util.ExceptionUnwrapper;

/**
 * Starts a transaction, calls {@link TransactionWorker#doWork}, and handles
 * transaction retry and exceptions.  To perform a transaction, the user
 * implements the {@link TransactionWorker} interface and passes an instance of
 * that class to the {@link #run run} method.
 *
 * <p>A single TransactionRunner instance may be used by any number of threads
 * for any number of transactions.</p>
 *
 * <p>The behavior of the run() method depends on whether the environment is
 * transactional, whether nested transactions are enabled, and whether a
 * transaction is already active.</p>
 *
 * <ul>
 * <li>When the run() method is called in a transactional environment and no
 * transaction is active for the current thread, a new transaction is started
 * before calling doWork().  If DeadlockException is thrown by doWork(),
 * the transaction will be aborted and the process will be repeated up to the
 * maximum number of retries.  If another exception is thrown by doWork() or
 * the maximum number of retries has occurred, the transaction will be aborted
 * and the exception will be rethrown by the run() method.  If no exception is
 * thrown by doWork(), the transaction will be committed.  The run() method
 * will not attempt to commit or abort a transaction if it has already been
 * committed or aborted by doWork().</li>
 *
 * <li>When the run() method is called and a transaction is active for the
 * current thread, and nested transactions are enabled, a nested transaction is
 * started before calling doWork().  The transaction that is active when
 * calling the run() method will become the parent of the nested transaction.
 * The nested transaction will be committed or aborted by the run() method
 * following the same rules described above.  Note that nested transactions may
 * not be enabled for the JE product, since JE does not support nested
 * transactions.</li>
 *
 * <li>When the run() method is called in a non-transactional environment, the
 * doWork() method is called without starting a transaction.  The run() method
 * will return without committing or aborting a transaction, and any exceptions
 * thrown by the doWork() method will be thrown by the run() method.</li>
 *
 * <li>When the run() method is called and a transaction is active for the
 * current thread and nested transactions are not enabled (the default) the
 * same rules as above apply. All the operations performed by the doWork()
 * method will be part of the currently active transaction.</li>
 * </ul>
 *
 * <p>In a transactional environment, the rules described above support nested
 * calls to the run() method and guarantee that the outermost call will cause
 * the transaction to be committed or aborted.  This is true whether or not
 * nested transactions are supported or enabled.  Note that nested transactions
 * are provided as an optimization for improving concurrency but do not change
 * the meaning of the outermost transaction.  Nested transactions are not
 * currently supported by the JE product.</p>
 *
 * @author Mark Hayes
 */
public class TransactionRunner {

    /** The default maximum number of retries. */
    public static final int DEFAULT_MAX_RETRIES = 10;

    private CurrentTransaction currentTxn;
    private int maxRetries;
    private TransactionConfig config;
    private boolean allowNestedTxn;

    /**
     * Creates a transaction runner for a given Berkeley DB environment.
     * The default maximum number of retries ({@link #DEFAULT_MAX_RETRIES}) and
     * a null (default) {@link TransactionConfig} will be used.
     *
     * @param env is the environment for running transactions.
     */
    public TransactionRunner(Environment env) {

        this(env, DEFAULT_MAX_RETRIES, null);
    }

    /**
     * Creates a transaction runner for a given Berkeley DB environment and
     * with a given number of maximum retries.
     *
     * @param env is the environment for running transactions.
     *
     * @param maxRetries is the maximum number of retries that will be
     * performed when deadlocks are detected.
     *
     * @param config the transaction configuration used for calling
     * {@link Environment#beginTransaction}, or null to use the default
     * configuration.  The configuration object is not cloned, and
     * any modifications to it will impact subsequent transactions.
     */
    public TransactionRunner(Environment env,
			     int maxRetries,
                             TransactionConfig config) {

        this.currentTxn = CurrentTransaction.getInstance(env);
        this.maxRetries = maxRetries;
        this.config = config;
    }

    /**
     * Returns the maximum number of retries that will be performed when
     * deadlocks are detected.
     */
    public int getMaxRetries() {

        return maxRetries;
    }

    /**
     * Changes the maximum number of retries that will be performed when
     * deadlocks are detected.
     * Calling this method does not impact transactions already running.
     */
    public void setMaxRetries(int maxRetries) {

        this.maxRetries = maxRetries;
    }

    /**
     * Returns whether nested transactions will be created if
     * <code>run()</code> is called when a transaction is already active for
     * the current thread.
     * By default this property is false.
     *
     * <p>Note that this method always returns false in the JE product, since
     * nested transactions are not supported by JE.</p>
     */
    public boolean getAllowNestedTransactions() {

        return allowNestedTxn;
    }

    /**
     * Changes whether nested transactions will be created if
     * <code>run()</code> is called when a transaction is already active for
     * the current thread.
     * Calling this method does not impact transactions already running.
     *
     * <p>Note that true may not be passed to this method in the JE product,
     * since nested transactions are not supported by JE.</p>
     */
    public void setAllowNestedTransactions(boolean allowNestedTxn) {

        if (allowNestedTxn && !DbCompat.NESTED_TRANSACTIONS) {
            throw new UnsupportedOperationException(
                    "Nested transactions are not supported.");
        }
        this.allowNestedTxn = allowNestedTxn;
    }

    /**
     * Returns the transaction configuration used for calling
     * {@link Environment#beginTransaction}.
     *
     * <p>If this property is null, the default configuration is used.  The
     * configuration object is not cloned, and any modifications to it will
     * impact subsequent transactions.</p>
     *
     * @return the transaction configuration.
     */
    public TransactionConfig getTransactionConfig() {

        return config;
    }

    /**
     * Changes the transaction configuration used for calling
     * {@link Environment#beginTransaction}.
     *
     * <p>If this property is null, the default configuration is used.  The
     * configuration object is not cloned, and any modifications to it will
     * impact subsequent transactions.</p>
     *
     * @param config the transaction configuration.
     */
    public void setTransactionConfig(TransactionConfig config) {

        this.config = config;
    }

    /**
     * Calls the {@link TransactionWorker#doWork} method and, for transactional
     * environments, may begin and end a transaction.  If the environment given
     * is non-transactional, a transaction will not be used but the doWork()
     * method will still be called.  See the class description for more
     * information.
     *
     * @throws DeadlockException when it is thrown by doWork() and the
     * maximum number of retries has occurred.  The transaction will have been
     * aborted by this method.
     *
     * @throws Exception when any other exception is thrown by doWork().  The
     * exception will first be unwrapped by calling {@link
     * ExceptionUnwrapper#unwrap}.  The transaction will have been aborted by
     * this method.
     */
    public void run(TransactionWorker worker)
        throws DatabaseException, Exception {

        if (currentTxn != null &&
            (allowNestedTxn || currentTxn.getTransaction() == null)) {
            /* Transactional and (not nested or nested txns allowed). */
            int useMaxRetries = maxRetries;
            for (int retries = 0;; retries += 1) {
                Transaction txn = null;
                try {
                    txn = currentTxn.beginTransaction(config);
                    worker.doWork();
                    if (txn != null && txn == currentTxn.getTransaction()) {
                        currentTxn.commitTransaction();
                    }
                    return;
                } catch (Throwable e) {
                    e = ExceptionUnwrapper.unwrapAny(e);
                    if (txn != null && txn == currentTxn.getTransaction()) {
                        try {
                            currentTxn.abortTransaction();
                        } catch (Throwable e2) {

                            /*
                             * We print this stack trace so that the
                             * information is not lost when we throw the
                             * original exception.
                             */
			    if (DbCompat.
                                TRANSACTION_RUNNER_PRINT_STACK_TRACES) {
				e2.printStackTrace();
			    }
                            /* Force the original exception to be thrown. */
                            retries = useMaxRetries;
                        }
                    }
                    /* An Error should not require special handling. */
                    if (e instanceof Error) {
                        throw (Error) e;
                    }
                    /* Allow a subclass to determine retry policy. */
                    Exception ex = (Exception) e;
                    useMaxRetries =
                        handleException(ex, retries, useMaxRetries);
                    if (retries >= useMaxRetries) {
                        throw ex;
                    }
                }
            }
        } else {
            /* Non-transactional or (nested and no nested txns allowed). */
            try {
                worker.doWork();
            } catch (Exception e) {
                throw ExceptionUnwrapper.unwrap(e);
            }
        }
    }

    /**
     * Handles exceptions that occur during a transaction, and may implement
     * transaction retry policy.  The transaction is aborted by the {@link
     * #run run} method before calling this method.
     *
     * <p>The default implementation of this method throws the {@code
     * exception} parameter if it is not an instance of {@link
     * DeadlockException} and otherwise returns the {@code maxRetries}
     * parameter value.  This method can be overridden to throw a different
     * exception or return a different number of retries.  For example:</p>
     * <ul>
     * <li>This method could call {@code Thread.sleep} for a short interval to
     * allow other transactions to finish.</li>
     *
     * <li>This method could return a different {@code maxRetries} value
     * depending on the {@code exception} that occurred.</li>
     *
     * <li>This method could throw an application-defined exception when the
     * {@code retries} value is greater or equal to the {@code maxRetries} and
     * a {@link DeadlockException} occurs, to override the default behavior
     * which is to throw the {@link DeadlockException}.</li>
     * </ul>
     *
     * @param exception an exception that was thrown by the {@link
     * TransactionWorker#doWork} method or thrown when beginning or committing
     * the transaction.  If the {@code retries} value is greater or equal to
     * {@code maxRetries} when this method returns normally, this exception
     * will be thrown by the {@link #run run} method.
     *
     * @param retries the current value of a counter that starts out at zero
     * and is incremented when each retry is performed.
     *
     * @param maxRetries the maximum retries to be performed.  By default,
     * this value is set to {@link #getMaxRetries}.  This method may return a
     * different maximum retries value to override that default.
     *
     * @return the maximum number of retries to perform.  The
     * default policy is to return the {@code maxRetries} parameter value
     * if the {@code exception} parameter value is an instance of {@link
     * DeadlockException}.
     *
     * @throws Exception to cause the exception to be thrown by the {@link
     * #run run} method.  The default policy is to throw the {@code exception}
     * parameter value if it is not an instance of {@link
     * DeadlockException}.
     *
     * @since 3.4
     */
    public int handleException(Exception exception,
                               int retries,
                               int maxRetries)
        throws Exception {

        if (exception instanceof DeadlockException) {
            return maxRetries;
        } else {
            throw exception;
        }
    }
}
