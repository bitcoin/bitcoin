/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package collections.ship.marshal;

import com.sleepycat.bind.EntityBinding;
import com.sleepycat.bind.EntryBinding;
import com.sleepycat.bind.serial.ClassCatalog;
import com.sleepycat.bind.serial.TupleSerialBinding;
import com.sleepycat.bind.tuple.TupleBinding;
import com.sleepycat.bind.tuple.TupleInput;
import com.sleepycat.bind.tuple.TupleOutput;
import com.sleepycat.collections.StoredSortedMap;
import com.sleepycat.collections.StoredSortedValueSet;
import com.sleepycat.util.RuntimeExceptionWrapper;

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
            new MarshalledKeyBinding(PartKey.class);
        EntityBinding partDataBinding =
            new MarshalledEntityBinding(catalog, Part.class);
        EntryBinding supplierKeyBinding =
            new MarshalledKeyBinding(SupplierKey.class);
        EntityBinding supplierDataBinding =
            new MarshalledEntityBinding(catalog, Supplier.class);
        EntryBinding shipmentKeyBinding =
            new MarshalledKeyBinding(ShipmentKey.class);
        EntityBinding shipmentDataBinding =
            new MarshalledEntityBinding(catalog, Shipment.class);
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
     * MarshalledKeyBinding is used to bind the stored key tuple entry to a key
     * object representation.  To do this, it calls the MarshalledKey interface
     * implemented by the key class.
     */
    private static class MarshalledKeyBinding extends TupleBinding {

        private Class keyClass;

        /**
         * Construct the binding object.
         */
        private MarshalledKeyBinding(Class keyClass) {

            // The key class will be used to instantiate the key object.
            //
            if (!MarshalledKey.class.isAssignableFrom(keyClass)) {
                throw new IllegalArgumentException(keyClass.toString() +
                                       " does not implement MarshalledKey");
            }
            this.keyClass = keyClass;
        }

        /**
         * Create the key object from the stored key tuple entry.
         */
        public Object entryToObject(TupleInput input) {

            try {
                MarshalledKey key = (MarshalledKey) keyClass.newInstance();
                key.unmarshalKey(input);
                return key;
            } catch (IllegalAccessException e) {
                throw new RuntimeExceptionWrapper(e);
            } catch (InstantiationException e) {
                throw new RuntimeExceptionWrapper(e);
            }
        }

        /**
         * Create the stored key tuple entry from the key object.
         */
        public void objectToEntry(Object object, TupleOutput output) {

            MarshalledKey key = (MarshalledKey) object;
            key.marshalKey(output);
        }
    }

    /**
     * MarshalledEntityBinding is used to bind the stored key/data entry pair
     * to a combined to an entity object representation.  To do this, it calls
     * the MarshalledEnt interface implemented by the entity class.
     *
     * <p> The binding is "tricky" in that it uses the entity class for both
     * the stored data entry and the combined entity object.  To do this,
     * entity's key field(s) are transient and are set by the binding after the
     * data object has been deserialized. This avoids the use of a "data" class
     * completely. </p>
     */
    private static class MarshalledEntityBinding extends TupleSerialBinding {

        /**
         * Construct the binding object.
         */
        private MarshalledEntityBinding(ClassCatalog classCatalog,
                                        Class entityClass) {

            super(classCatalog, entityClass);

            // The entity class will be used to instantiate the entity object.
            //
            if (!MarshalledEnt.class.isAssignableFrom(entityClass)) {
                throw new IllegalArgumentException(entityClass.toString() +
                                       " does not implement MarshalledEnt");
            }
        }

        /**
         * Create the entity by combining the stored key and data.
         * This "tricky" binding returns the stored data as the entity, but
         * first it sets the transient key fields from the stored key.
         */
        public Object entryToObject(TupleInput tupleInput, Object javaInput) {

            MarshalledEnt entity = (MarshalledEnt) javaInput;
            entity.unmarshalPrimaryKey(tupleInput);
            return entity;
        }

        /**
         * Create the stored key from the entity.
         */
        public void objectToKey(Object object, TupleOutput output) {

            MarshalledEnt entity = (MarshalledEnt) object;
            entity.marshalPrimaryKey(output);
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
