/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2004-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package persist;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.Serializable;
import java.util.Calendar;
import java.util.Date;
import java.util.HashSet;
import java.util.Random;
import java.util.Set;

import com.sleepycat.bind.EntryBinding;
import com.sleepycat.bind.serial.SerialBinding;
import com.sleepycat.bind.serial.StoredClassCatalog;
import com.sleepycat.bind.tuple.IntegerBinding;
import com.sleepycat.bind.tuple.LongBinding;
import com.sleepycat.db.Cursor;
import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseConfig;
import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.DatabaseType;
import com.sleepycat.db.Environment;
import com.sleepycat.db.EnvironmentConfig;
import com.sleepycat.db.OperationStatus;
import com.sleepycat.db.SecondaryConfig;
import com.sleepycat.db.SecondaryCursor;
import com.sleepycat.db.SecondaryDatabase;
import com.sleepycat.db.SecondaryKeyCreator;
import com.sleepycat.db.Transaction;

/**
 * EventExample is a trivial example which stores Java objects that represent
 * an event. Events are primarily indexed by a timestamp, but have other
 * attributes, such as price, account reps, customer name and quantity.
 * Some of those other attributes are indexed.
 * <p>
 * The example simply shows the creation of a BDB environment and database,
 * inserting some events, and retrieving the events.
 * <p>
 * This example is meant to be paired with its twin, EventExampleDPL.java.
 * EventExample.java and EventExampleDPL.java perform the same functionality,
 * but use the Base API and the Direct Persistence Layer API, respectively.
 * This may be a useful way to compare the two APIs.
 * <p>
 * To run the example:
 * <pre>
 * cd jehome/examples
 * javac je/EventExample.java
 * java je.EventExample -h <environmentDirectory>
 * </pre>
 */
public class EventExample {

    /*
     * The Event class embodies our example event and is the application
     * data. BDB data records are represented at key/data tuples. In this
     * example, the key portion of the record is the event time, and the data
     * portion is the Event instance.
     */
    static class Event implements Serializable {

        /* This example will add secondary indices on price and accountReps. */
        private int price;
        private Set<String> accountReps;

        private String customerName;
        private int quantity;

        Event(int price,
              String customerName) {

            this.price = price;
            this.customerName = customerName;
            this.accountReps = new HashSet<String>();
        }

        void addRep(String rep) {
            accountReps.add(rep);
        }

        @Override
        public String toString() {
            StringBuilder sb = new StringBuilder();
            sb.append(" price=").append(price);
            sb.append(" customerName=").append(customerName);
            sb.append(" reps=");
            if (accountReps.size() == 0) {
                sb.append("none");
            } else {
                for (String rep: accountReps) {
                    sb.append(rep).append(" ");
                }
            }
            return sb.toString();
        }

        int getPrice() {
            return price;
        }
    }

    /* A BDB environment is roughly equivalent to a relational database. */
    private Environment env;

    /*
     * A BDB table is roughly equivalent to a relational table with a
     * primary index.
     */
    private Database eventDb;

    /* A secondary database indexes an additional field of the data record */
    private SecondaryDatabase eventByPriceDb;

    /*
     * The catalogs and bindings are used to convert Java objects to the byte
     * array format used by BDB key/data in the base API. The Direct
     * Persistence Layer API supports Java objects as arguments directly.
     */
    private Database catalogDb;
    private EntryBinding eventBinding;

    /* Used for generating example data. */
    private Calendar cal;


    /*
     * First manually make a directory to house the BDB environment.
     * Usage: java EventExample -h <envHome>
     * All BDB on-disk storage is held within envHome.
     */
    public static void main(String[] args)
        throws DatabaseException, FileNotFoundException {

        if (args.length != 2 || !"-h".equals(args[0])) {
            System.err.println
                ("Usage: java " + EventExample.class.getName() +
                 " -h <envHome>");
            System.exit(2);
        }
        EventExample example = new EventExample(new File(args[1]));
        example.run();
        example.close();
    }

    private EventExample(File envHome)
        throws DatabaseException, FileNotFoundException {

        /* Open a transactional Berkeley DB engine environment. */
        System.out.println("-> Creating a BDB environment");
        EnvironmentConfig envConfig = new EnvironmentConfig();
        envConfig.setAllowCreate(true);
        envConfig.setTransactional(true);
        envConfig.setInitializeCache(true);
        envConfig.setInitializeLocking(true);
        env = new Environment(envHome, envConfig);

        init();
        cal = Calendar.getInstance();
    }

    /**
     * Create all primary and secondary indices.
     */
    private void init()
        throws DatabaseException, FileNotFoundException {

        System.out.println("-> Creating a BDB database");
        DatabaseConfig dbConfig = new DatabaseConfig();
        dbConfig.setTransactional(true);
        dbConfig.setAllowCreate(true);
        dbConfig.setType(DatabaseType.BTREE);
        eventDb = env.openDatabase(null,      // use auto-commit txn
                                   "eventDb", // file name
                                   null,      // database name
                                   dbConfig);


        /*
         * In our example, the database record is composed of a key portion
         * which represents the event timestamp, and a data portion holds an
         * instance of the Event class.
         *
         * BDB's base API accepts and returns key and data as byte arrays, so
         * we need some support for marshaling between objects and byte arrays.
         * We call this binding, and supply a package of helper classes to
         * support this. It's entirely possible to do all binding on your own.
         *
         * A class catalog database is needed for storing class descriptions
         * for the serial binding used below. This avoids storing class
         * descriptions redundantly in each record.
         */
        DatabaseConfig catalogConfig = new DatabaseConfig();
        catalogConfig.setTransactional(true);
        catalogConfig.setAllowCreate(true);
        catalogConfig.setType(DatabaseType.BTREE);
        catalogDb = env.openDatabase(null, "catalogDb", null, catalogConfig);
        StoredClassCatalog catalog = new StoredClassCatalog(catalogDb);

        /*
         * Create a serial binding for Event data objects.  Serial
         * bindings can be used to store any Serializable object.
         * We can use some pre-defined binding classes to convert
         * primitives like the long key value to the a byte array.
         */
        eventBinding = new SerialBinding(catalog, Event.class);

        /*
         * Open a secondary database to allow accessing the primary
         * database a secondary key value. In this case, access events
         * by price.
         */
        SecondaryConfig secConfig = new SecondaryConfig();
        secConfig.setTransactional(true);
        secConfig.setAllowCreate(true);
        secConfig.setType(DatabaseType.BTREE);
        secConfig.setSortedDuplicates(true);
        secConfig.setKeyCreator(new PriceKeyCreator(eventBinding));
        eventByPriceDb = env.openSecondaryDatabase(null,
                                                   "priceDb",
                                                   null,
                                                   eventDb,
                                                   secConfig);

    }

    private void run()
        throws DatabaseException {

        Random rand = new Random();

        /* DatabaseEntry represents the key and data of each record */
        DatabaseEntry key = new DatabaseEntry();
        DatabaseEntry data = new DatabaseEntry();

        /*
         * Create a set of events. Each insertion is a separate, auto-commit
         * transaction.
         */
        System.out.println("-> Inserting 4 events");
        LongBinding.longToEntry(makeDate(1), key);
        eventBinding.objectToEntry(new Event(100, "Company_A"),
                                   data);
        eventDb.put(null, key, data);

        LongBinding.longToEntry(makeDate(2), key);
        eventBinding.objectToEntry(new Event(2, "Company_B"),
                                   data);
        eventDb.put(null, key, data);

        LongBinding.longToEntry(makeDate(3), key);
        eventBinding.objectToEntry(new Event(20, "Company_C"),
                                   data);
        eventDb.put(null, key, data);

        LongBinding.longToEntry(makeDate(4), key);
        eventBinding.objectToEntry(new Event(40, "CompanyD"),
                                   data);
        eventDb.put(null, key, data);

        /* Load a whole set of events transactionally. */
        Transaction txn = env.beginTransaction(null, null);
        int maxPrice = 50;
        System.out.println("-> Inserting some randomly generated events");
        for (int i = 0; i < 25; i++) {
            long time = makeDate(rand.nextInt(365));
            Event e = new Event(rand.nextInt(maxPrice),"Company_X");
            if ((i%2) ==0) {
                e.addRep("Jane");
                e.addRep("Nikunj");
            } else {
                e.addRep("Yongmin");
            }
            LongBinding.longToEntry(time, key);
            eventBinding.objectToEntry(e, data);
            eventDb.put(txn, key, data);
        }
        txn.commitWriteNoSync();

        /*
         * Windows of events - display the events between June 1 and Aug 31
         */
        System.out.println("\n-> Display the events between June 1 and Aug 31");
        long endDate = makeDate(Calendar.AUGUST, 31);

        /* Position the cursor and print the first event. */
        Cursor eventWindow = eventDb.openCursor(null, null);
        LongBinding.longToEntry(makeDate(Calendar.JUNE, 1), key);

        if ((eventWindow.getSearchKeyRange(key,  data, null)) !=
            OperationStatus.SUCCESS) {
            System.out.println("No events found!");
            eventWindow.close();
            return;
        }
        try {
            printEvents(key, data, eventWindow, endDate);
        } finally {
            eventWindow.close();
        }

        /*
         * Display all events, ordered by a secondary index on price.
         */
        System.out.println("\n-> Display all events, ordered by price");
        SecondaryCursor priceCursor =
            eventByPriceDb.openSecondaryCursor(null, null);
        try {
            printEvents(priceCursor);
        } finally {
            priceCursor.close();
        }
    }

    private void close()
        throws DatabaseException {

        eventByPriceDb.close();
        eventDb.close();
        catalogDb.close();
        env.close();
    }

    /**
     * Print all events covered by this cursor up to the end date.  We know
     * that the cursor operates on long keys and Event data items, but there's
     * no type-safe way of expressing that within the BDB base API.
     */
    private void printEvents(DatabaseEntry firstKey,
                             DatabaseEntry firstData,
                             Cursor cursor,
                             long endDate)
        throws DatabaseException {

        System.out.println("time=" +
                           new Date(LongBinding.entryToLong(firstKey)) +
                           eventBinding.entryToObject(firstData));
        DatabaseEntry key = new DatabaseEntry();
        DatabaseEntry data = new DatabaseEntry();

        while (cursor.getNext(key, data, null) ==
               OperationStatus.SUCCESS) {
            if (LongBinding.entryToLong(key) > endDate) {
                break;
            }
            System.out.println("time=" +
                               new Date(LongBinding.entryToLong(key)) +
                               eventBinding.entryToObject(data));
        }
    }

    private void printEvents(SecondaryCursor cursor)
        throws DatabaseException {
        DatabaseEntry timeKey = new DatabaseEntry();
        DatabaseEntry priceKey = new DatabaseEntry();
        DatabaseEntry eventData = new DatabaseEntry();

        while (cursor.getNext(priceKey, timeKey, eventData, null) ==
               OperationStatus.SUCCESS) {
            System.out.println("time=" +
                               new Date(LongBinding.entryToLong(timeKey)) +
                               eventBinding.entryToObject(eventData));
        }
    }

    /**
     * Little utility for making up java.util.Dates for different days, just
     * to generate test data.
     */
    private long makeDate(int day) {

        cal.set((Calendar.DAY_OF_YEAR), day);
        return cal.getTime().getTime();
    }
    /**
     * Little utility for making up java.util.Dates for different days, just
     * to make the test data easier to read.
     */
    private long makeDate(int month, int day) {

        cal.set((Calendar.MONTH), month);
        cal.set((Calendar.DAY_OF_MONTH), day);
        return cal.getTime().getTime();
    }

    /**
     * A key creator that knows how to extract the secondary key from the data
     * entry of the primary database.  To do so, it uses both the dataBinding
     * of the primary database and the secKeyBinding.
     */
    private static class PriceKeyCreator implements SecondaryKeyCreator {

        private EntryBinding dataBinding;

        PriceKeyCreator(EntryBinding eventBinding) {
            this.dataBinding = eventBinding;
        }

        public boolean createSecondaryKey(SecondaryDatabase secondaryDb,
                                          DatabaseEntry keyEntry,
                                          DatabaseEntry dataEntry,
                                          DatabaseEntry resultEntry)
            throws DatabaseException {

            /*
             * Convert the data entry to an Event object, extract the secondary
             * key value from it, and then convert it to the resulting
             * secondary key entry.
             */
            Event e  = (Event) dataBinding.entryToObject(dataEntry);
            int price = e.getPrice();
            IntegerBinding.intToEntry(price, resultEntry);
            return true;
        }
    }
}
