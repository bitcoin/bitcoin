/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package collections.ship.sentity;

import com.sleepycat.bind.EntityBinding;
import com.sleepycat.bind.EntryBinding;
import com.sleepycat.bind.serial.ClassCatalog;
import com.sleepycat.bind.serial.TupleSerialBinding;
import com.sleepycat.bind.tuple.TupleBinding;
import com.sleepycat.bind.tuple.TupleInput;
import com.sleepycat.bind.tuple.TupleOutput;
import com.sleepycat.collections.StoredSortedMap;
import com.sleepycat.collections.StoredSortedValueSet;

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
        // key/data entry pair to a combined data object; a "tricky" binding
        // that uses transient fields is used--see PartBinding, etc, for
        // details.  For keys, a one-to-one binding is implemented with
        // EntryBinding classes to bind the stored tuple entry to a key Object.
        //
        ClassCatalog catalog = db.getClassCatalog();
        EntryBinding partKeyBinding =
            new PartKeyBinding();
        EntityBinding partDataBinding =
            new PartBinding(catalog, Part.class);
        EntryBinding supplierKeyBinding =
            new SupplierKeyBinding();
        EntityBinding supplierDataBinding =
            new SupplierBinding(catalog, Supplier.class);
        EntryBinding shipmentKeyBinding =
            new ShipmentKeyBinding();
        EntityBinding shipmentDataBinding =
            new ShipmentBinding(catalog, Shipment.class);
        EntryBinding cityKeyBinding =
            TupleBinding.getPrimitiveBinding(String.class);

        // Create map views for all stores and indices.
        // StoredSortedMap is used since the stores and indices are ordered
        // (they use the DB_BTREE access method).
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
    public StoredSortedValueSet getPartSet() {

        return (StoredSortedValueSet) partMap.values();
    }

    /**
     * Return an entity set view of the supplier storage container.
     */
    public StoredSortedValueSet getSupplierSet() {

        return (StoredSortedValueSet) supplierMap.values();
    }

    /**
     * Return an entity set view of the shipment storage container.
     */
    public StoredSortedValueSet getShipmentSet() {

        return (StoredSortedValueSet) shipmentMap.values();
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
     * PartKeyBinding is used to bind the stored key tuple entry for a part to
     * a key object representation.
     */
    private static class PartKeyBinding extends TupleBinding {

        /**
         * Construct the binding object.
         */
        private PartKeyBinding() {
        }

        /**
         * Create the key object from the stored key tuple entry.
         */
        public Object entryToObject(TupleInput input) {

            String number = input.readString();
            return new PartKey(number);
        }

        /**
         * Create the stored key tuple entry from the key object.
         */
        public void objectToEntry(Object object, TupleOutput output) {

            PartKey key = (PartKey) object;
            output.writeString(key.getNumber());
        }
    }

    /**
     * PartBinding is used to bind the stored key/data entry pair for a part
     * to a combined data object (entity).
     *
     * <p> The binding is "tricky" in that it uses the Part class for both the
     * stored data entry and the combined entity object.  To do this, Part's
     * key field(s) are transient and are set by the binding after the data
     * object has been deserialized. This avoids the use of a PartData class
     * completely. </p>
     */
    private static class PartBinding extends TupleSerialBinding {

        /**
         * Construct the binding object.
         */
        private PartBinding(ClassCatalog classCatalog, Class dataClass) {

            super(classCatalog, dataClass);
        }

        /**
         * Create the entity by combining the stored key and data.
         * This "tricky" binding returns the stored data as the entity, but
         * first it sets the transient key fields from the stored key.
         */
        public Object entryToObject(TupleInput keyInput, Object dataInput) {

            String number = keyInput.readString();
            Part part = (Part) dataInput;
            part.setKey(number);
            return part;
        }

        /**
         * Create the stored key from the entity.
         */
        public void objectToKey(Object object, TupleOutput output) {

            Part part = (Part) object;
            output.writeString(part.getNumber());
        }

        /**
         * Return the entity as the stored data.  There is nothing to do here
         * since the entity's key fields are transient.
         */
        public Object objectToData(Object object) {

            return object;
        }
    }

    /**
     * SupplierKeyBinding is used to bind the stored key tuple entry for a
     * supplier to a key object representation.
     */
    private static class SupplierKeyBinding extends TupleBinding {

        /**
         * Construct the binding object.
         */
        private SupplierKeyBinding() {
        }

        /**
         * Create the key object from the stored key tuple entry.
         */
        public Object entryToObject(TupleInput input) {

            String number = input.readString();
            return new SupplierKey(number);
        }

        /**
         * Create the stored key tuple entry from the key object.
         */
        public void objectToEntry(Object object, TupleOutput output) {

            SupplierKey key = (SupplierKey) object;
            output.writeString(key.getNumber());
        }
    }

    /**
     * SupplierBinding is used to bind the stored key/data entry pair for a
     * supplier to a combined data object (entity).
     *
     * <p> The binding is "tricky" in that it uses the Supplier class for both
     * the stored data entry and the combined entity object.  To do this,
     * Supplier's key field(s) are transient and are set by the binding after
     * the data object has been deserialized. This avoids the use of a
     * SupplierData class completely. </p>
     */
    private static class SupplierBinding extends TupleSerialBinding {

        /**
         * Construct the binding object.
         */
        private SupplierBinding(ClassCatalog classCatalog, Class dataClass) {

            super(classCatalog, dataClass);
        }

        /**
         * Create the entity by combining the stored key and data.
         * This "tricky" binding returns the stored data as the entity, but
         * first it sets the transient key fields from the stored key.
         */
        public Object entryToObject(TupleInput keyInput, Object dataInput) {

            String number = keyInput.readString();
            Supplier supplier = (Supplier) dataInput;
            supplier.setKey(number);
            return supplier;
        }

        /**
         * Create the stored key from the entity.
         */
        public void objectToKey(Object object, TupleOutput output) {

            Supplier supplier = (Supplier) object;
            output.writeString(supplier.getNumber());
        }

        /**
         * Return the entity as the stored data.  There is nothing to do here
         * since the entity's key fields are transient.
         */
        public Object objectToData(Object object) {

            return object;
        }
    }

    /**
     * ShipmentKeyBinding is used to bind the stored key tuple entry for a
     * shipment to a key object representation.
     */
    private static class ShipmentKeyBinding extends TupleBinding {

        /**
         * Construct the binding object.
         */
        private ShipmentKeyBinding() {
        }

        /**
         * Create the key object from the stored key tuple entry.
         */
        public Object entryToObject(TupleInput input) {

            String partNumber = input.readString();
            String supplierNumber = input.readString();
            return new ShipmentKey(partNumber, supplierNumber);
        }

        /**
         * Create the stored key tuple entry from the key object.
         */
        public void objectToEntry(Object object, TupleOutput output) {

            ShipmentKey key = (ShipmentKey) object;
            output.writeString(key.getPartNumber());
            output.writeString(key.getSupplierNumber());
        }
    }

    /**
     * ShipmentBinding is used to bind the stored key/data entry pair for a
     * shipment to a combined data object (entity).
     *
     * <p> The binding is "tricky" in that it uses the Shipment class for both
     * the stored data entry and the combined entity object.  To do this,
     * Shipment's key field(s) are transient and are set by the binding after
     * the data object has been deserialized. This avoids the use of a
     * ShipmentData class completely. </p>
     */
    private static class ShipmentBinding extends TupleSerialBinding {

        /**
         * Construct the binding object.
         */
        private ShipmentBinding(ClassCatalog classCatalog, Class dataClass) {

            super(classCatalog, dataClass);
        }

        /**
         * Create the entity by combining the stored key and data.
         * This "tricky" binding returns the stored data as the entity, but
         * first it sets the transient key fields from the stored key.
         */
        public Object entryToObject(TupleInput keyInput, Object dataInput) {

            String partNumber = keyInput.readString();
            String supplierNumber = keyInput.readString();
            Shipment shipment = (Shipment) dataInput;
            shipment.setKey(partNumber, supplierNumber);
            return shipment;
        }

        /**
         * Create the stored key from the entity.
         */
        public void objectToKey(Object object, TupleOutput output) {

            Shipment shipment = (Shipment) object;
            output.writeString(shipment.getPartNumber());
            output.writeString(shipment.getSupplierNumber());
        }

        /**
         * Return the entity as the stored data.  There is nothing to do here
         * since the entity's key fields are transient.
         */
        public Object objectToData(Object object) {

            return object;
        }
    }
}
