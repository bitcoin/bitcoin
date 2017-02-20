/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2004-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package persist;

import static com.sleepycat.persist.model.Relationship.MANY_TO_ONE;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.Calendar;
import java.util.Date;
import java.util.HashSet;
import java.util.Random;
import java.util.Set;

import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.Environment;
import com.sleepycat.db.EnvironmentConfig;
import com.sleepycat.db.Transaction;
import com.sleepycat.persist.EntityCursor;
import com.sleepycat.persist.EntityStore;
import com.sleepycat.persist.PrimaryIndex;
import com.sleepycat.persist.SecondaryIndex;
import com.sleepycat.persist.StoreConfig;
import com.sleepycat.persist.model.Entity;
import com.sleepycat.persist.model.PrimaryKey;
import com.sleepycat.persist.model.SecondaryKey;

/**
 * EventExampleDPL is a trivial example which stores Java objects that
 * represent an event. Events are primarily indexed by a timestamp, but have
 * other attributes, such as price, account reps, customer name and
 * quantity.  Some of those other attributes are indexed.
 * <p>
 * The example simply shows the creation of a BDB environment and database,
 * inserting some events, and retrieving the events using the Direct
 * Persistence layer.
 * <p>
 * This example is meant to be paired with its twin, EventExample.java.
 * EventExample.java and EventExampleDPL.java perform the same functionality,
 * but use the Base API and the Direct Persistence Layer API, respectively.
 * This may be a useful way to compare the two APIs.
 * <p>
 * To run the example:
 * <pre>
 * javac EventExampleDPL.java
 * java EventExampleDPL -h <environmentDirectory>
 * </pre>
 */
public class EventExampleDPL {

    /*
     * The Event class embodies our example event and is the application
     * data. The @Entity annotation indicates that this class defines the
     * objects stored in a BDB database.
     */
    @Entity
    static class Event {

        @PrimaryKey
        private Date time;

        @SecondaryKey(relate=MANY_TO_ONE)
        private int price;

        private Set<String> accountReps;

        private String customerName;
        private int quantity;

        Event(Date time,
              int price,
              String customerName) {

            this.time = time;
            this.price = price;
            this.customerName = customerName;
            this.accountReps = new HashSet<String>();
        }

        private Event() {} // For deserialization

        void addRep(String rep) {
            accountReps.add(rep);
        }

        @Override
        public String toString() {
            StringBuilder sb = new StringBuilder();
            sb.append("time=").append(time);
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
    }

    /* A BDB environment is roughly equivalent to a relational database. */
    private Environment env;
    private EntityStore store;

    /*
     * Event accessors let us access events by the primary index (time)
     * as well as by the rep and price fields
     */
    PrimaryIndex<Date,Event> eventByTime;
    SecondaryIndex<Integer,Date,Event> eventByPrice;

    /* Used for generating example data. */
    private Calendar cal;

    /*
     * First manually make a directory to house the BDB environment.
     * Usage: java EventExampleDPL -h <envHome>
     * All BDB on-disk storage is held within envHome.
     */
    public static void main(String[] args)
        throws DatabaseException, FileNotFoundException {

        if (args.length != 2 || !"-h".equals(args[0])) {
            System.err.println
                ("Usage: java " + EventExampleDPL.class.getName() +
                 " -h <envHome>");
            System.exit(2);
        }
        EventExampleDPL example = new EventExampleDPL(new File(args[1]));
        example.run();
        example.close();
    }

    private EventExampleDPL(File envHome)
        throws DatabaseException, FileNotFoundException {

        /* Open a transactional Berkeley DB engine environment. */
        System.out.println("-> Creating a BDB environment");
        EnvironmentConfig envConfig = new EnvironmentConfig();
        envConfig.setAllowCreate(true);
        envConfig.setTransactional(true);
        envConfig.setInitializeCache(true);
        envConfig.setInitializeLocking(true);
        env = new Environment(envHome, envConfig);

        /* Initialize the data access object. */
        init();
        cal = Calendar.getInstance();
    }

    /**
     * Create all primary and secondary indices.
     */
    private void init()
        throws DatabaseException {

        /* Open a transactional entity store. */
        System.out.println("-> Creating a BDB database");
        StoreConfig storeConfig = new StoreConfig();
        storeConfig.setAllowCreate(true);
        storeConfig.setTransactional(true);
        store = new EntityStore(env, "ExampleStore", storeConfig);

        eventByTime = store.getPrimaryIndex(Date.class, Event.class);
        eventByPrice = store.getSecondaryIndex(eventByTime,
                                               Integer.class,
                                               "price");
    }

    private void run()
        throws DatabaseException {

        Random rand = new Random();

        /*
         * Create a set of events. Each insertion is a separate, auto-commit
         * transaction.
         */
        System.out.println("-> Inserting 4 events");
        eventByTime.put(new Event(makeDate(1), 100, "Company_A"));
        eventByTime.put(new Event(makeDate(2), 2, "Company_B"));
        eventByTime.put(new Event(makeDate(3), 20, "Company_C"));
        eventByTime.put(new Event(makeDate(4), 40, "CompanyD"));

        /* Load a whole set of events transactionally. */
        Transaction txn = env.beginTransaction(null, null);
        int maxPrice = 50;
        System.out.println("-> Inserting some randomly generated events");
        for (int i = 0; i < 25; i++) {
            Event e = new Event(makeDate(rand.nextInt(365)),
                                rand.nextInt(maxPrice),
                                "Company_X");
            if ((i%2) ==0) {
                e.addRep("Bob");
                e.addRep("Nikunj");
            } else {
                e.addRep("Yongmin");
            }
            eventByTime.put(e);
        }
        txn.commitWriteNoSync();

        /*
         * Windows of events - display the events between June 1 and Aug 31
         */
        System.out.println("\n-> Display the events between June 1 and Aug 31");
        Date startDate = makeDate(Calendar.JUNE, 1);
        Date endDate = makeDate(Calendar.AUGUST, 31);

        EntityCursor<Event> eventWindow =
            eventByTime.entities(startDate, true, endDate, true);
        printEvents(eventWindow);

        /*
         * Display all events, ordered by a secondary index on price.
         */
        System.out.println("\n-> Display all events, ordered by price");
        EntityCursor<Event> byPriceEvents = eventByPrice.entities();
        printEvents(byPriceEvents);
    }

    private void close()
        throws DatabaseException {

        store.close();
        env.close();
    }

    /**
     * Print all events covered by this cursor.
     */
    private void printEvents(EntityCursor<Event> eCursor)
        throws DatabaseException {
        try {
            for (Event e: eCursor) {
                System.out.println(e);
            }
        } finally {
            /* Be sure to close the cursor. */
            eCursor.close();
        }
    }

    /**
     * Little utility for making up java.util.Dates for different days, just
     * to generate test data.
     */
    private Date makeDate(int day) {

        cal.set((Calendar.DAY_OF_YEAR), day);
        return cal.getTime();
    }

    /**
     * Little utility for making up java.util.Dates for different days, just
     * to make the test data easier to read.
     */
    private Date makeDate(int month, int day) {

        cal.set((Calendar.MONTH), month);
        cal.set((Calendar.DAY_OF_MONTH), day);
        return cal.getTime();
    }
}
