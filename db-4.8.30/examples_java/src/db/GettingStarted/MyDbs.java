/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2004-2009 Oracle.  All rights reserved.
 *
 * $Id$ 
 */

// File: MyDbs.java

package db.GettingStarted;

import java.io.FileNotFoundException;

import com.sleepycat.bind.serial.StoredClassCatalog;
import com.sleepycat.bind.tuple.TupleBinding;
import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseConfig;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.DatabaseType;
import com.sleepycat.db.SecondaryConfig;
import com.sleepycat.db.SecondaryDatabase;


public class MyDbs {

    // The databases that our application uses
    private Database vendorDb = null;
    private Database inventoryDb = null;
    private Database classCatalogDb = null;
    private SecondaryDatabase itemNameIndexDb = null;

    private String vendordb = "VendorDB.db";
    private String inventorydb = "InventoryDB.db";
    private String classcatalogdb = "ClassCatalogDB.db";
    private String itemnameindexdb = "ItemNameIndexDB.db";

    // Needed for object serialization
    private StoredClassCatalog classCatalog;

    // Our constructor does nothing
    public MyDbs() {}

    // The setup() method opens all our databases
    // for us.
    public void setup(String databasesHome)
        throws DatabaseException {

        DatabaseConfig myDbConfig = new DatabaseConfig();
        SecondaryConfig mySecConfig = new SecondaryConfig();

        myDbConfig.setErrorStream(System.err);
        mySecConfig.setErrorStream(System.err);
        myDbConfig.setErrorPrefix("MyDbs");
        mySecConfig.setErrorPrefix("MyDbs");
        myDbConfig.setType(DatabaseType.BTREE);
        mySecConfig.setType(DatabaseType.BTREE);
        myDbConfig.setAllowCreate(true);
        mySecConfig.setAllowCreate(true);

        // Now open, or create and open, our databases
        // Open the vendors and inventory databases
        try {
            vendordb = databasesHome + "/" + vendordb;
            vendorDb = new Database(vendordb,
                                    null,
                                    myDbConfig);

            inventorydb = databasesHome + "/" + inventorydb;
            inventoryDb = new Database(inventorydb,
                                        null,
                                        myDbConfig);

            // Open the class catalog db. This is used to
            // optimize class serialization.
            classcatalogdb = databasesHome + "/" + classcatalogdb;
            classCatalogDb = new Database(classcatalogdb,
                                          null,
                                          myDbConfig);
        } catch(FileNotFoundException fnfe) {
            System.err.println("MyDbs: " + fnfe.toString());
            System.exit(-1);
        }

        // Create our class catalog
        classCatalog = new StoredClassCatalog(classCatalogDb);

        // Need a tuple binding for the Inventory class.
        // We use the InventoryBinding class
        // that we implemented for this purpose.
        TupleBinding inventoryBinding = new InventoryBinding();

        // Open the secondary database. We use this to create a
        // secondary index for the inventory database

        // We want to maintain an index for the inventory entries based
        // on the item name. So, instantiate the appropriate key creator
        // and open a secondary database.
        ItemNameKeyCreator keyCreator =
            new ItemNameKeyCreator(new InventoryBinding());


        // Set up additional secondary properties
        // Need to allow duplicates for our secondary database
        mySecConfig.setSortedDuplicates(true);
        mySecConfig.setAllowPopulate(true); // Allow autopopulate
        mySecConfig.setKeyCreator(keyCreator);

        // Now open it
        try {
            itemnameindexdb = databasesHome + "/" + itemnameindexdb;
            itemNameIndexDb = new SecondaryDatabase(itemnameindexdb,
                                                    null,
                                                    inventoryDb,
                                                    mySecConfig);
        } catch(FileNotFoundException fnfe) {
            System.err.println("MyDbs: " + fnfe.toString());
            System.exit(-1);
        }
    }

   // getter methods
    public Database getVendorDB() {
        return vendorDb;
    }

    public Database getInventoryDB() {
        return inventoryDb;
    }

    public SecondaryDatabase getNameIndexDB() {
        return itemNameIndexDb;
    }

    public StoredClassCatalog getClassCatalog() {
        return classCatalog;
    }

    // Close the databases
    public void close() {
        try {
            if (itemNameIndexDb != null) {
                itemNameIndexDb.close();
            }

            if (vendorDb != null) {
                vendorDb.close();
            }

            if (inventoryDb != null) {
                inventoryDb.close();
            }

            if (classCatalogDb != null) {
                classCatalogDb.close();
            }

        } catch(DatabaseException dbe) {
            System.err.println("Error closing MyDbs: " +
                                dbe.toString());
            System.exit(-1);
        }
    }
}

