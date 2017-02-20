/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package collections.ship.index;

import java.io.FileNotFoundException;
import java.util.Iterator;
import java.util.Map;

import com.sleepycat.collections.TransactionRunner;
import com.sleepycat.collections.TransactionWorker;
import com.sleepycat.db.DatabaseException;

/**
 * Sample is the main entry point for the sample program and may be run as
 * follows:
 *
 * <pre>
 * java collections.ship.index.Sample
 *      [-h <home-directory> ]
 * </pre>
 *
 * <p> The default for the home directory is ./tmp -- the tmp subdirectory of
 * the current directory where the sample is run. The home directory must exist
 * before running the sample.  To recreate the sample database from scratch,
 * delete all files in the home directory before running the sample.  </p>
 *
 * @author Mark Hayes
 */
public class Sample {

    private SampleDatabase db;
    private SampleViews views;

    /**
     * Run the sample program.
     */
    public static void main(String[] args) {

        System.out.println("\nRunning sample: " + Sample.class);

        // Parse the command line arguments.
        //
        String homeDir = "./tmp";
        for (int i = 0; i < args.length; i += 1) {
            if (args[i].equals("-h") && i < args.length - 1) {
                i += 1;
                homeDir = args[i];
            } else {
                System.err.println("Usage:\n java " + Sample.class.getName() +
				   "\n  [-h <home-directory>]");
                System.exit(2);
            }
        }

        // Run the sample.
        //
        Sample sample = null;
        try {
            sample = new Sample(homeDir);
            sample.run();
        } catch (Exception e) {
            // If an exception reaches this point, the last transaction did not
            // complete.  If the exception is RunRecoveryException, follow
            // the Berkeley DB recovery procedures before running again.
            e.printStackTrace();
        } finally {
            if (sample != null) {
                try {
                    // Always attempt to close the database cleanly.
                    sample.close();
                } catch (Exception e) {
                    System.err.println("Exception during database close:");
                    e.printStackTrace();
                }
            }
        }
    }

    /**
     * Open the database and views.
     */
    private Sample(String homeDir)
        throws DatabaseException, FileNotFoundException {

        db = new SampleDatabase(homeDir);
        views = new SampleViews(db);
    }

    /**
     * Close the database cleanly.
     */
    private void close()
        throws DatabaseException {

        db.close();
    }

    /**
     * Run two transactions to populate and print the database.  A
     * TransactionRunner is used to ensure consistent handling of transactions,
     * including deadlock retries.  But the best transaction handling mechanism
     * to use depends on the application.
     */
    private void run()
        throws Exception {

        TransactionRunner runner = new TransactionRunner(db.getEnvironment());
        runner.run(new PopulateDatabase());
        runner.run(new PrintDatabase());
    }

    /**
     * Populate the database in a single transaction.
     */
    private class PopulateDatabase implements TransactionWorker {

        public void doWork()
            throws Exception {
            addSuppliers();
            addParts();
            addShipments();
        }
    }

    /**
     * Print the database in a single transaction.  All entities are printed
     * and the indices are used to print the entities for certain keys.
     *
     * <p> Note the use of special iterator() methods.  These are used here
     * with indices to find the shipments for certain keys.</p>
     */
    private class PrintDatabase implements TransactionWorker {

        public void doWork()
            throws Exception {
            printEntries("Parts",
			 views.getPartEntrySet().iterator());
            printEntries("Suppliers",
			 views.getSupplierEntrySet().iterator());
            printValues("Suppliers for City Paris",
			views.getSupplierByCityMap().duplicates(
							"Paris").iterator());
            printEntries("Shipments",
			 views.getShipmentEntrySet().iterator());
            printValues("Shipments for Part P1",
			views.getShipmentByPartMap().duplicates(
						new PartKey("P1")).iterator());
            printValues("Shipments for Supplier S1",
			views.getShipmentBySupplierMap().duplicates(
					    new SupplierKey("S1")).iterator());
        }
    }

    /**
     * Populate the part entities in the database.  If the part map is not
     * empty, assume that this has already been done.
     */
    private void addParts() {

        Map parts = views.getPartMap();
        if (parts.isEmpty()) {
            System.out.println("Adding Parts");
            parts.put(new PartKey("P1"),
                      new PartData("Nut", "Red",
                                    new Weight(12.0, Weight.GRAMS),
                                    "London"));
            parts.put(new PartKey("P2"),
                      new PartData("Bolt", "Green",
                                    new Weight(17.0, Weight.GRAMS),
                                    "Paris"));
            parts.put(new PartKey("P3"),
                      new PartData("Screw", "Blue",
                                    new Weight(17.0, Weight.GRAMS),
                                    "Rome"));
            parts.put(new PartKey("P4"),
                      new PartData("Screw", "Red",
                                    new Weight(14.0, Weight.GRAMS),
                                    "London"));
            parts.put(new PartKey("P5"),
                      new PartData("Cam", "Blue",
                                    new Weight(12.0, Weight.GRAMS),
                                    "Paris"));
            parts.put(new PartKey("P6"),
                      new PartData("Cog", "Red",
                                    new Weight(19.0, Weight.GRAMS),
                                    "London"));
        }
    }

    /**
     * Populate the supplier entities in the database.  If the supplier map is
     * not empty, assume that this has already been done.
     */
    private void addSuppliers() {

        Map suppliers = views.getSupplierMap();
        if (suppliers.isEmpty()) {
            System.out.println("Adding Suppliers");
            suppliers.put(new SupplierKey("S1"),
                          new SupplierData("Smith", 20, "London"));
            suppliers.put(new SupplierKey("S2"),
                          new SupplierData("Jones", 10, "Paris"));
            suppliers.put(new SupplierKey("S3"),
                          new SupplierData("Blake", 30, "Paris"));
            suppliers.put(new SupplierKey("S4"),
                          new SupplierData("Clark", 20, "London"));
            suppliers.put(new SupplierKey("S5"),
                          new SupplierData("Adams", 30, "Athens"));
        }
    }

    /**
     * Populate the shipment entities in the database.  If the shipment map
     * is not empty, assume that this has already been done.
     */
    private void addShipments() {

        Map shipments = views.getShipmentMap();
        if (shipments.isEmpty()) {
            System.out.println("Adding Shipments");
            shipments.put(new ShipmentKey("P1", "S1"),
                          new ShipmentData(300));
            shipments.put(new ShipmentKey("P2", "S1"),
                          new ShipmentData(200));
            shipments.put(new ShipmentKey("P3", "S1"),
                          new ShipmentData(400));
            shipments.put(new ShipmentKey("P4", "S1"),
                          new ShipmentData(200));
            shipments.put(new ShipmentKey("P5", "S1"),
                          new ShipmentData(100));
            shipments.put(new ShipmentKey("P6", "S1"),
                          new ShipmentData(100));
            shipments.put(new ShipmentKey("P1", "S2"),
                          new ShipmentData(300));
            shipments.put(new ShipmentKey("P2", "S2"),
                          new ShipmentData(400));
            shipments.put(new ShipmentKey("P2", "S3"),
                          new ShipmentData(200));
            shipments.put(new ShipmentKey("P2", "S4"),
                          new ShipmentData(200));
            shipments.put(new ShipmentKey("P4", "S4"),
                          new ShipmentData(300));
            shipments.put(new ShipmentKey("P5", "S4"),
                          new ShipmentData(400));
        }
    }

    /**
     * Print the key/value objects returned by an iterator of Map.Entry
     * objects.
     */
    private void printEntries(String label, Iterator iterator) {

        System.out.println("\n--- " + label + " ---");
        while (iterator.hasNext()) {
            Map.Entry entry = (Map.Entry) iterator.next();
            System.out.println(entry.getKey().toString());
            System.out.println(entry.getValue().toString());
        }
    }

    /**
     * Print the objects returned by an iterator of value objects.
     */
    private void printValues(String label, Iterator iterator) {

        System.out.println("\n--- " + label + " ---");
        while (iterator.hasNext()) {
            System.out.println(iterator.next().toString());
        }
    }
}
