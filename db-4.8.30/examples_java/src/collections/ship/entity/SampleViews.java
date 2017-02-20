/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package collections.ship.entity;

import com.sleepycat.bind.EntityBinding;
import com.sleepycat.bind.serial.ClassCatalog;
import com.sleepycat.bind.serial.SerialBinding;
import com.sleepycat.bind.serial.SerialSerialBinding;
import com.sleepycat.collections.StoredSortedMap;
import com.sleepycat.collections.StoredValueSet;

/**
 * SampleViews defines the data bindings and collection views for the sample
 * database.
 *
 * @author Mark Hayes
 */
public class SampleViews {

    private StoredSortedMap partMap;
    private StoredSortedMap supplierMap;
    private StoredSortedMap shipmentMap;
    private StoredSortedMap shipmentByPartMap;
    private StoredSortedMap shipmentBySupplierMap;
    private StoredSortedMap supplierByCityMap;

    /**
     * Create the data bindings and collection views.
     */
    public SampleViews(SampleDatabase db) {

        // Create the data bindings.
        // In this sample, EntityBinding classes are used to bind the stored
        // key/data entry pair to a combined data object.  For keys, however,
        // the stored entry is used directly via a SerialBinding and no
        // special binding class is needed.
        //
        ClassCatalog catalog = db.getClassCatalog();
        SerialBinding partKeyBinding =
            new SerialBinding(catalog, PartKey.class);
        EntityBinding partDataBinding =
            new PartBinding(catalog, PartKey.class, PartData.class);
        SerialBinding supplierKeyBinding =
            new SerialBinding(catalog, SupplierKey.class);
        EntityBinding supplierDataBinding =
            new SupplierBinding(catalog, SupplierKey.class,
                                SupplierData.class);
        SerialBinding shipmentKeyBinding =
            new SerialBinding(catalog, ShipmentKey.class);
        EntityBinding shipmentDataBinding =
            new ShipmentBinding(catalog, ShipmentKey.class,
                                ShipmentData.class);
        SerialBinding cityKeyBinding =
            new SerialBinding(catalog, String.class);

        // Create map views for all stores and indices.
        // StoredSortedMap is not used since the stores and indices are
        // ordered by serialized key objects, which do not provide a very
        // useful ordering.
        //
        partMap =
            new StoredSortedMap(db.getPartDatabase(),
                                partKeyBinding, partDataBinding, true);
        supplierMap =
            new StoredSortedMap(db.getSupplierDatabase(),
                                supplierKeyBinding, supplierDataBinding, true);
        shipmentMap =
            new StoredSortedMap(db.getShipmentDatabase(),
                                shipmentKeyBinding, shipmentDataBinding, true);
        shipmentByPartMap =
            new StoredSortedMap(db.getShipmentByPartDatabase(),
                                partKeyBinding, shipmentDataBinding, true);
        shipmentBySupplierMap =
            new StoredSortedMap(db.getShipmentBySupplierDatabase(),
                                supplierKeyBinding, shipmentDataBinding, true);
        supplierByCityMap =
            new StoredSortedMap(db.getSupplierByCityDatabase(),
                                cityKeyBinding, supplierDataBinding, true);
    }

    // The views returned below can be accessed using the java.util.Map or
    // java.util.Set interfaces, or using the StoredSortedMap and
    // StoredValueSet classes, which provide additional methods.  The entity
    // sets could be obtained directly from the Map.values() method but
    // convenience methods are provided here to return them in order to avoid
    // down-casting elsewhere.

    /**
     * Return a map view of the part storage container.
     */
    public StoredSortedMap getPartMap() {

        return partMap;
    }

    /**
     * Return a map view of the supplier storage container.
     */
    public StoredSortedMap getSupplierMap() {

        return supplierMap;
    }

    /**
     * Return a map view of the shipment storage container.
     */
    public StoredSortedMap getShipmentMap() {

        return shipmentMap;
    }

    /**
     * Return an entity set view of the part storage container.
     */
    public StoredValueSet getPartSet() {

        return (StoredValueSet) partMap.values();
    }

    /**
     * Return an entity set view of the supplier storage container.
     */
    public StoredValueSet getSupplierSet() {

        return (StoredValueSet) supplierMap.values();
    }

    /**
     * Return an entity set view of the shipment storage container.
     */
    public StoredValueSet getShipmentSet() {

        return (StoredValueSet) shipmentMap.values();
    }

    /**
     * Return a map view of the shipment-by-part index.
     */
    public StoredSortedMap getShipmentByPartMap() {

        return shipmentByPartMap;
    }

    /**
     * Return a map view of the shipment-by-supplier index.
     */
    public StoredSortedMap getShipmentBySupplierMap() {

        return shipmentBySupplierMap;
    }

    /**
     * Return a map view of the supplier-by-city index.
     */
    public final StoredSortedMap getSupplierByCityMap() {

        return supplierByCityMap;
    }

    /**
     * PartBinding is used to bind the stored key/data entry pair for a part
     * to a combined data object (entity).
     */
    private static class PartBinding extends SerialSerialBinding {

        /**
         * Construct the binding object.
         */
        private PartBinding(ClassCatalog classCatalog,
                            Class keyClass,
                            Class dataClass) {

            super(classCatalog, keyClass, dataClass);
        }

        /**
         * Create the entity by combining the stored key and data.
         */
        public Object entryToObject(Object keyInput, Object dataInput) {

            PartKey key = (PartKey) keyInput;
            PartData data = (PartData) dataInput;
            return new Part(key.getNumber(), data.getName(), data.getColor(),
                            data.getWeight(), data.getCity());
        }

        /**
         * Create the stored key from the entity.
         */
        public Object objectToKey(Object object) {

            Part part = (Part) object;
            return new PartKey(part.getNumber());
        }

        /**
         * Create the stored data from the entity.
         */
        public Object objectToData(Object object) {

            Part part = (Part) object;
            return new PartData(part.getName(), part.getColor(),
                                 part.getWeight(), part.getCity());
        }
    }

    /**
     * SupplierBinding is used to bind the stored key/data entry pair for a
     * supplier to a combined data object (entity).
     */
    private static class SupplierBinding extends SerialSerialBinding {

        /**
         * Construct the binding object.
         */
        private SupplierBinding(ClassCatalog classCatalog,
                                Class keyClass,
                                Class dataClass) {

            super(classCatalog, keyClass, dataClass);
        }

        /**
         * Create the entity by combining the stored key and data.
         */
        public Object entryToObject(Object keyInput, Object dataInput) {

            SupplierKey key = (SupplierKey) keyInput;
            SupplierData data = (SupplierData) dataInput;
            return new Supplier(key.getNumber(), data.getName(),
                                data.getStatus(), data.getCity());
        }

        /**
         * Create the stored key from the entity.
         */
        public Object objectToKey(Object object) {

            Supplier supplier = (Supplier) object;
            return new SupplierKey(supplier.getNumber());
        }

        /**
         * Create the stored data from the entity.
         */
        public Object objectToData(Object object) {

            Supplier supplier = (Supplier) object;
            return new SupplierData(supplier.getName(), supplier.getStatus(),
                                     supplier.getCity());
        }
    }

    /**
     * ShipmentBinding is used to bind the stored key/data entry pair for a
     * shipment to a combined data object (entity).
     */
    private static class ShipmentBinding extends SerialSerialBinding {

        /**
         * Construct the binding object.
         */
        private ShipmentBinding(ClassCatalog classCatalog,
                                Class keyClass,
                                Class dataClass) {

            super(classCatalog, keyClass, dataClass);
        }

        /**
         * Create the entity by combining the stored key and data.
         */
        public Object entryToObject(Object keyInput, Object dataInput) {

            ShipmentKey key = (ShipmentKey) keyInput;
            ShipmentData data = (ShipmentData) dataInput;
            return new Shipment(key.getPartNumber(), key.getSupplierNumber(),
                                data.getQuantity());
        }

        /**
         * Create the stored key from the entity.
         */
        public Object objectToKey(Object object) {

            Shipment shipment = (Shipment) object;
            return new ShipmentKey(shipment.getPartNumber(),
                                   shipment.getSupplierNumber());
        }

        /**
         * Create the stored data from the entity.
         */
        public Object objectToData(Object object) {

            Shipment shipment = (Shipment) object;
            return new ShipmentData(shipment.getQuantity());
        }
    }
}
