/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package collections.ship.index;

import java.io.File;
import java.io.FileNotFoundException;

import com.sleepycat.bind.serial.ClassCatalog;
import com.sleepycat.bind.serial.SerialSerialKeyCreator;
import com.sleepycat.bind.serial.StoredClassCatalog;
import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseConfig;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.DatabaseType;
import com.sleepycat.db.Environment;
import com.sleepycat.db.EnvironmentConfig;
import com.sleepycat.db.ForeignKeyDeleteAction;
import com.sleepycat.db.SecondaryConfig;
import com.sleepycat.db.SecondaryDatabase;

/**
 * SampleDatabase defines the storage containers, indices and foreign keys
 * for the sample database.
 *
 * @author Mark Hayes
 */
public class SampleDatabase {

    private static final String CLASS_CATALOG = "java_class_catalog";
    private static final String SUPPLIER_STORE = "supplier_store";
    private static final String PART_STORE = "part_store";
    private static final String SHIPMENT_STORE = "shipment_store";
    private static final String SHIPMENT_PART_INDEX = "shipment_part_index";
    private static final String SHIPMENT_SUPPLIER_INDEX =
	"shipment_supplier_index";
    private static final String SUPPLIER_CITY_INDEX = "supplier_city_index";

    private Environment env;
    private Database partDb;
    private Database supplierDb;
    private Database shipmentDb;
    private SecondaryDatabase supplierByCityDb;
    private SecondaryDatabase shipmentByPartDb;
    private SecondaryDatabase shipmentBySupplierDb;
    private StoredClassCatalog javaCatalog;

    /**
     * Open all storage containers, indices, and catalogs.
     */
    public SampleDatabase(String homeDirectory)
        throws DatabaseException, FileNotFoundException {

        // Open the Berkeley DB environment in transactional mode.
        //
        System.out.println("Opening environment in: " + homeDirectory);
        EnvironmentConfig envConfig = new EnvironmentConfig();
        envConfig.setTransactional(true);
        envConfig.setAllowCreate(true);
        envConfig.setInitializeCache(true);
        envConfig.setInitializeLocking(true);
        env = new Environment(new File(homeDirectory), envConfig);

        // Set the Berkeley DB config for opening all stores.
        //
        DatabaseConfig dbConfig = new DatabaseConfig();
        dbConfig.setTransactional(true);
        dbConfig.setAllowCreate(true);
        dbConfig.setType(DatabaseType.BTREE);

        // Create the Serial class catalog.  This holds the serialized class
        // format for all database records of serial format.
        //
        Database catalogDb = env.openDatabase(null, CLASS_CATALOG, null,
                                              dbConfig);
        javaCatalog = new StoredClassCatalog(catalogDb);

        // Open the Berkeley DB database for the part, supplier and shipment
        // stores.  The stores are opened with no duplicate keys allowed.
        //
        partDb = env.openDatabase(null, PART_STORE, null, dbConfig);

        supplierDb = env.openDatabase(null, SUPPLIER_STORE, null, dbConfig);

        shipmentDb = env.openDatabase(null, SHIPMENT_STORE, null, dbConfig);

        // Open the SecondaryDatabase for the city index of the supplier store,
        // and for the part and supplier indices of the shipment store.
        // Duplicate keys are allowed since more than one supplier may be in
        // the same city, and more than one shipment may exist for the same
        // supplier or part.  A foreign key constraint is defined for the
        // supplier and part indices to ensure that a shipment only refers to
        // existing part and supplier keys.  The CASCADE delete action means
        // that shipments will be deleted if their associated part or supplier
        // is deleted.
        //
        SecondaryConfig secConfig = new SecondaryConfig();
        secConfig.setTransactional(true);
        secConfig.setAllowCreate(true);
        secConfig.setType(DatabaseType.BTREE);
        secConfig.setSortedDuplicates(true);

        secConfig.setKeyCreator(
            new SupplierByCityKeyCreator(javaCatalog,
                                         SupplierKey.class,
                                         SupplierData.class,
                                         String.class));
        supplierByCityDb = env.openSecondaryDatabase(null, SUPPLIER_CITY_INDEX,
                                                     null, supplierDb,
                                                     secConfig);

        secConfig.setForeignKeyDatabase(partDb);
        secConfig.setForeignKeyDeleteAction(ForeignKeyDeleteAction.CASCADE);
        secConfig.setKeyCreator(
            new ShipmentByPartKeyCreator(javaCatalog,
                                         ShipmentKey.class,
                                         ShipmentData.class,
                                         PartKey.class));
        shipmentByPartDb = env.openSecondaryDatabase(null, SHIPMENT_PART_INDEX,
                                                     null, shipmentDb,
                                                     secConfig);

        secConfig.setForeignKeyDatabase(supplierDb);
        secConfig.setForeignKeyDeleteAction(ForeignKeyDeleteAction.CASCADE);
        secConfig.setKeyCreator(
            new ShipmentBySupplierKeyCreator(javaCatalog,
                                             ShipmentKey.class,
                                             ShipmentData.class,
                                             SupplierKey.class));
        shipmentBySupplierDb = env.openSecondaryDatabase(null,
                                                     SHIPMENT_SUPPLIER_INDEX,
                                                     null, shipmentDb,
                                                     secConfig);
    }

    /**
     * Return the storage environment for the database.
     */
    public final Environment getEnvironment() {

        return env;
    }

    /**
     * Return the class catalog.
     */
    public final StoredClassCatalog getClassCatalog() {

        return javaCatalog;
    }

    /**
     * Return the part storage container.
     */
    public final Database getPartDatabase() {

        return partDb;
    }

    /**
     * Return the supplier storage container.
     */
    public final Database getSupplierDatabase() {

        return supplierDb;
    }

    /**
     * Return the shipment storage container.
     */
    public final Database getShipmentDatabase() {

        return shipmentDb;
    }

    /**
     * Return the shipment-by-part index.
     */
    public final SecondaryDatabase getShipmentByPartDatabase() {

        return shipmentByPartDb;
    }

    /**
     * Return the shipment-by-supplier index.
     */
    public final SecondaryDatabase getShipmentBySupplierDatabase() {

        return shipmentBySupplierDb;
    }

    /**
     * Return the supplier-by-city index.
     */
    public final SecondaryDatabase getSupplierByCityDatabase() {

        return supplierByCityDb;
    }

    /**
     * Close all stores (closing a store automatically closes its indices).
     */
    public void close()
        throws DatabaseException {

        // Close secondary databases, then primary databases.
        supplierByCityDb.close();
        shipmentByPartDb.close();
        shipmentBySupplierDb.close();
        partDb.close();
        supplierDb.close();
        shipmentDb.close();
        // And don't forget to close the catalog and the environment.
        javaCatalog.close();
        env.close();
    }

    /**
     * The SecondaryKeyCreator for the SupplierByCity index.  This is an
     * extension of the abstract class SerialSerialKeyCreator, which implements
     * SecondaryKeyCreator for the case where the data keys and value are all
     * of the serial format.
     */
    private static class SupplierByCityKeyCreator
        extends SerialSerialKeyCreator {

        /**
         * Construct the city key extractor.
         * @param catalog is the class catalog.
         * @param primaryKeyClass is the supplier key class.
         * @param valueClass is the supplier value class.
         * @param indexKeyClass is the city key class.
         */
        private SupplierByCityKeyCreator(ClassCatalog catalog,
                                         Class primaryKeyClass,
                                         Class valueClass,
                                         Class indexKeyClass) {

            super(catalog, primaryKeyClass, valueClass, indexKeyClass);
        }

        /**
         * Extract the city key from a supplier key/value pair.  The city key
         * is stored in the supplier value, so the supplier key is not used.
         */
        public Object createSecondaryKey(Object primaryKeyInput,
                                         Object valueInput) {

            SupplierData supplierData = (SupplierData) valueInput;
            return supplierData.getCity();
        }
    }

    /**
     * The SecondaryKeyCreator for the ShipmentByPart index.  This is an
     * extension of the abstract class SerialSerialKeyCreator, which implements
     * SecondaryKeyCreator for the case where the data keys and value are all
     * of the serial format.
     */
    private static class ShipmentByPartKeyCreator
        extends SerialSerialKeyCreator {

        /**
         * Construct the part key extractor.
         * @param catalog is the class catalog.
         * @param primaryKeyClass is the shipment key class.
         * @param valueClass is the shipment value class.
         * @param indexKeyClass is the part key class.
         */
        private ShipmentByPartKeyCreator(ClassCatalog catalog,
                                         Class primaryKeyClass,
                                         Class valueClass,
                                         Class indexKeyClass) {

            super(catalog, primaryKeyClass, valueClass, indexKeyClass);
        }

        /**
         * Extract the part key from a shipment key/value pair.  The part key
         * is stored in the shipment key, so the shipment value is not used.
         */
        public Object createSecondaryKey(Object primaryKeyInput,
                                         Object valueInput) {

            ShipmentKey shipmentKey = (ShipmentKey) primaryKeyInput;
            return new PartKey(shipmentKey.getPartNumber());
        }
    }

    /**
     * The SecondaryKeyCreator for the ShipmentBySupplier index.  This is an
     * extension of the abstract class SerialSerialKeyCreator, which implements
     * SecondaryKeyCreator for the case where the data keys and value are all
     * of the serial format.
     */
    private static class ShipmentBySupplierKeyCreator
        extends SerialSerialKeyCreator {

        /**
         * Construct the supplier key extractor.
         * @param catalog is the class catalog.
         * @param primaryKeyClass is the shipment key class.
         * @param valueClass is the shipment value class.
         * @param indexKeyClass is the supplier key class.
         */
        private ShipmentBySupplierKeyCreator(ClassCatalog catalog,
                                             Class primaryKeyClass,
                                             Class valueClass,
                                             Class indexKeyClass) {

            super(catalog, primaryKeyClass, valueClass, indexKeyClass);
        }

        /**
         * Extract the supplier key from a shipment key/value pair.  The part
         * key is stored in the shipment key, so the shipment value is not
         * used.
         */
        public Object createSecondaryKey(Object primaryKeyInput,
                                         Object valueInput) {

            ShipmentKey shipmentKey = (ShipmentKey) primaryKeyInput;
            return new SupplierKey(shipmentKey.getSupplierNumber());
        }
    }
}
