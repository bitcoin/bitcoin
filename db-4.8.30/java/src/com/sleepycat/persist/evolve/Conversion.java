/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.evolve;

import java.io.Serializable;

import com.sleepycat.persist.model.EntityModel;
import com.sleepycat.persist.raw.RawObject;
import com.sleepycat.persist.raw.RawType;

/**
 * Converts an old version of an object value to conform to the current class
 * or field definition.
 *
 * <p>The {@code Conversion} interface is implemented by the user.  A
 * {@code Conversion} instance is passed to the {@link Converter#Converter}
 * constructor.</p>
 *
 * <p>The {@code Conversion} interface extends {@link Serializable} and the
 * {@code Conversion} instance is serialized for storage using standard Java
 * serialization.  Normally, the {@code Conversion} class should only have
 * transient fields that are initialized in the {@link #initialize} method.
 * While non-transient fields are allowed, care must be taken to only include
 * fields that are serializable and will not pull in large amounts of data.</p>
 *
 * <p>When a class conversion is specified, two special considerations
 * apply:</p>
 * <ol>
 * <li>A class conversion is only applied when to instances of that class.  The
 * conversion will not be applied when the class when it appears as a
 * superclass of the instance's class.  In this case, a conversion for the
 * instance's class must also be specified.</li>
 * <li>Although field renaming (as well as all other changes) is handled by the
 * conversion method, a field Renamer is still needed when a secondary key
 * field is renamed and field Deleter is still needed when a secondary key
 * field is deleted.  This is necessary for evolution of the metadata;
 * specifically, if the key name changes the database must be renamed and if
 * the key field is deleted the secondary database must be deleted.</li>
 * </ol>
 *
 * <p>The {@code Conversion} class must implement the standard equals method.
 * See {@link #equals} for more information.</p>
 *
 * <p>Conversions of simple types are generally simple.  For example, a {@code
 * String} field that contains only integer values can be easily converted to
 * an {@code int} field:</p>
 * <pre class="code">
 *  // The old class.  Version 0 is implied.
 *  //
 *  {@literal @Persistent}
 *  class Address {
 *      String zipCode;
 *      ...
 *  }
 *
 *  // The new class.  A new version number must be assigned.
 *  //
 *  {@literal @Persistent(version=1)}
 *  class Address {
 *      int zipCode;
 *      ...
 *  }
 *
 *  // The conversion class.
 *  //
 *  class MyConversion1 implements Conversion {
 *
 *      public void initialize(EntityModel model) {
 *          // No initialization needed.
 *      }
 *
 *      public Object convert(Object fromValue) {
 *          return Integer.valueOf((String) fromValue);
 *      }
 *
 *      {@code @Override}
 *      public boolean equals(Object o) {
 *          return o instanceof MyConversion1;
 *      }
 *  }
 *
 *  // Create a field converter mutation.
 *  //
 *  Converter converter = new Converter(Address.class.getName(), 0,
 *                                      "zipCode", new MyConversion1());
 *
 *  // Configure the converter as described {@link Mutations here}.</pre>
 *
 * <p>A conversion may perform arbitrary transformations on an object.  For
 * example, a conversion may transform a single String address field into an
 * Address object containing four fields for street, city, state and zip
 * code.</p>
 * <pre class="code">
 *  // The old class.  Version 0 is implied.
 *  //
 *  {@literal @Entity}
 *  class Person {
 *      String address;
 *      ...
 *  }
 *
 *  // The new class.  A new version number must be assigned.
 *  //
 *  {@literal @Entity(version=1)}
 *  class Person {
 *      Address address;
 *      ...
 *  }
 *
 *  // The new address class.
 *  //
 *  {@literal @Persistent}
 *  class Address {
 *      String street;
 *      String city;
 *      String state;
 *      int zipCode;
 *      ...
 *  }
 *
 *  class MyConversion2 implements Conversion {
 *      private transient RawType addressType;
 *
 *      public void initialize(EntityModel model) {
 *          addressType = model.getRawType(Address.class.getName());
 *      }
 *
 *      public Object convert(Object fromValue) {
 *
 *          // Parse the old address and populate the new address fields
 *          //
 *          String oldAddress = (String) fromValue;
 *          {@literal Map<String,Object> addressValues = new HashMap<String,Object>();}
 *          addressValues.put("street", parseStreet(oldAddress));
 *          addressValues.put("city", parseCity(oldAddress));
 *          addressValues.put("state", parseState(oldAddress));
 *          addressValues.put("zipCode", parseZipCode(oldAddress));
 *
 *          // Return new raw Address object
 *          //
 *          return new RawObject(addressType, addressValues, null);
 *      }
 *
 *      {@code @Override}
 *      public boolean equals(Object o) {
 *          return o instanceof MyConversion2;
 *      }
 *
 *      private String parseStreet(String oldAddress) { ... }
 *      private String parseCity(String oldAddress) { ... }
 *      private String parseState(String oldAddress) { ... }
 *      private Integer parseZipCode(String oldAddress) { ... }
 *  }
 *
 *  // Create a field converter mutation.
 *  //
 *  Converter converter = new Converter(Person.class.getName(), 0,
 *                                      "address", new MyConversion2());
 *
 *  // Configure the converter as described {@link Mutations here}.</pre>
 *
 * <p>Note that when a conversion returns a {@link RawObject}, it must return
 * it with a {@link RawType} that is current as defined by the current class
 * definitions.  The proper types can be obtained from the {@link EntityModel}
 * in the conversion's {@link #initialize initialize} method.</p>
 *
 * <p>A variation on the example above is where several fields in a class
 * (street, city, state and zipCode) are converted to a single field (address).
 * In this case a class converter rather than a field converter is used.</p>
 *
 * <pre class="code">
 *  // The old class.  Version 0 is implied.
 *  //
 *  {@literal @Entity}
 *  class Person {
 *      String street;
 *      String city;
 *      String state;
 *      int zipCode;
 *      ...
 *  }
 *
 *  // The new class.  A new version number must be assigned.
 *  //
 *  {@literal @Entity(version=1)}
 *  class Person {
 *      Address address;
 *      ...
 *  }
 *
 *  // The new address class.
 *  //
 *  {@literal @Persistent}
 *  class Address {
 *      String street;
 *      String city;
 *      String state;
 *      int zipCode;
 *      ...
 *  }
 *
 *  class MyConversion3 implements Conversion {
 *      private transient RawType newPersonType;
 *      private transient RawType addressType;
 *
 *      public void initialize(EntityModel model) {
 *          newPersonType = model.getRawType(Person.class.getName());
 *          addressType = model.getRawType(Address.class.getName());
 *      }
 *
 *      public Object convert(Object fromValue) {
 *
 *          // Get field value maps for old and new objects.
 *          //
 *          RawObject person = (RawObject) fromValue;
 *          {@literal Map<String,Object> personValues = person.getValues();}
 *          {@literal Map<String,Object> addressValues = new HashMap<String,Object>();}
 *          RawObject address = new RawObject(addressType, addressValues, null);
 *
 *          // Remove the old address fields and insert the new one.
 *          //
 *          addressValues.put("street", personValues.remove("street"));
 *          addressValues.put("city", personValues.remove("city"));
 *          addressValues.put("state", personValues.remove("state"));
 *          addressValues.put("zipCode", personValues.remove("zipCode"));
 *          personValues.put("address", address);
 *
 *          return new RawObject(newPersonType, personValues, person.getSuper());
 *      }
 *
 *      {@code @Override}
 *      public boolean equals(Object o) {
 *          return o instanceof MyConversion3;
 *      }
 *  }
 *
 *  // Create a class converter mutation.
 *  //
 *  Converter converter = new Converter(Person.class.getName(), 0,
 *                                      new MyConversion3());
 *
 *  // Configure the converter as described {@link Mutations here}.</pre>
 *
 *
 * <p>A conversion can also handle changes to class hierarchies.  For example,
 * if a "name" field originally declared in class A is moved to its superclass
 * B, a conversion can move the field value accordingly:</p>
 *
 * <pre class="code">
 *  // The old classes.  Version 0 is implied.
 *  //
 *  {@literal @Persistent}
 *  class A extends B {
 *      String name;
 *      ...
 *  }
 *  {@literal @Persistent}
 *  abstract class B {
 *      ...
 *  }
 *
 *  // The new classes.  A new version number must be assigned.
 *  //
 *  {@literal @Persistent(version=1)}
 *  class A extends B {
 *      ...
 *  }
 *  {@literal @Persistent(version=1)}
 *  abstract class B {
 *      String name;
 *      ...
 *  }
 *
 *  class MyConversion4 implements Conversion {
 *      private transient RawType newAType;
 *      private transient RawType newBType;
 *
 *      public void initialize(EntityModel model) {
 *          newAType = model.getRawType(A.class.getName());
 *          newBType = model.getRawType(B.class.getName());
 *      }
 *
 *      public Object convert(Object fromValue) {
 *          RawObject oldA = (RawObject) fromValue;
 *          RawObject oldB = oldA.getSuper();
 *          {@literal Map<String,Object> aValues = oldA.getValues();}
 *          {@literal Map<String,Object> bValues = oldB.getValues();}
 *          bValues.put("name", aValues.remove("name"));
 *          RawObject newB = new RawObject(newBType, bValues, oldB.getSuper());
 *          RawObject newA = new RawObject(newAType, aValues, newB);
 *          return newA;
 *      }
 *
 *      {@code @Override}
 *      public boolean equals(Object o) {
 *          return o instanceof MyConversion4;
 *      }
 *  }
 *
 *  // Create a class converter mutation.
 *  //
 *  Converter converter = new Converter(A.class.getName(), 0,
 *                                      new MyConversion4());
 *
 *  // Configure the converter as described {@link Mutations here}.</pre>
 *
 * <p>A conversion may return an instance of a different class entirely, as
 * long as it conforms to current class definitions and is the type expected
 * in the given context (a subtype of the old type, or a type compatible with
 * the new field type).  For example, a field that is used to discriminate
 * between two types of objects could be removed and replaced by two new
 * subclasses:</p> <pre class="code">
 *  // The old class.  Version 0 is implied.
 *  //
 *  {@literal @Persistent}
 *  class Pet {
 *      boolean isCatNotDog;
 *      ...
 *  }
 *
 *  // The new classes.  A new version number must be assigned to the Pet class.
 *  //
 *  {@literal @Persistent(version=1)}
 *  class Pet {
 *      ...
 *  }
 *  {@literal @Persistent}
 *  class Cat extends Pet {
 *      ...
 *  }
 *  {@literal @Persistent}
 *  class Dog extends Pet {
 *      ...
 *  }
 *
 *  class MyConversion5 implements Conversion {
 *      private transient RawType newPetType;
 *      private transient RawType dogType;
 *      private transient RawType catType;
 *
 *      public void initialize(EntityModel model) {
 *          newPetType = model.getRawType(Pet.class.getName());
 *          dogType = model.getRawType(Dog.class.getName());
 *          catType = model.getRawType(Cat.class.getName());
 *      }
 *
 *      public Object convert(Object fromValue) {
 *          RawObject pet = (RawObject) fromValue;
 *          {@literal Map<String,Object> petValues = pet.getValues();}
 *          Boolean isCat = (Boolean) petValues.remove("isCatNotDog");
 *          RawObject newPet = new RawObject(newPetType, petValues,
 *                                           pet.getSuper());
 *          RawType newSubType = isCat ? catType : dogType;
 *          return new RawObject(newSubType, Collections.emptyMap(), newPet);
 *      }
 *
 *      {@code @Override}
 *      public boolean equals(Object o) {
 *          return o instanceof MyConversion5;
 *      }
 *  }
 *
 *  // Create a class converter mutation.
 *  //
 *  Converter converter = new Converter(Pet.class.getName(), 0,
 *                                      new MyConversion5());
 *
 *  // Configure the converter as described {@link Mutations here}.</pre>
 *
 * <p>The primary limitation of a conversion is that it may access at most a
 * single entity instance at one time.  Conversions involving multiple entities
 * at once may be made by performing a <a
 * href="package-summary.html#storeConversion">store conversion</a>.</p>
 *
 * @see com.sleepycat.persist.evolve Class Evolution
 * @author Mark Hayes
 */
public interface Conversion extends Serializable {

    /**
     * Initializes the conversion, allowing it to obtain raw type information
     * from the entity model.
     */
    void initialize(EntityModel model);

    /**
     * Converts an old version of an object value to conform to the current
     * class or field definition.
     *
     * <p>If a {@link RuntimeException} is thrown by this method, it will be
     * thrown to the original caller.  Similarly, a {@link
     * IllegalArgumentException} will be thrown to the original caller if the
     * object returned by this method does not conform to current class
     * definitions.</p>
     *
     * <p>The class of the input and output object may be one of the simple
     * types or {@link RawObject}.  For primitive types, the primitive wrapper
     * class is used.</p>
     *
     * @param fromValue the object value being converted.  The type of this
     * value is defined by the old class version that is being converted.
     *
     * @return the converted object.  The type of this value must conform to
     * a current class definition.  If this is a class conversion, it must
     * be the current version of the class.  If this is a field conversion, it
     * must be of a type compatible with the current declared type of the
     * field.
     */
    Object convert(Object fromValue);

    /**
     * The standard {@code equals} method that must be implemented by
     * conversion class.
     *
     * <p>When mutations are specified when opening a store, the specified and
     * previously stored mutations are compared for equality.  If they are
     * equal, there is no need to replace the existing mutations in the stored
     * catalog.  To accurately determine equality, the conversion class must
     * implement the {@code equals} method.</p>
     *
     * <p>If the {@code equals} method is not explicitly implemented by the
     * conversion class or a superclass other than {@code Object}, {@code
     * IllegalArgumentException} will be thrown when the store is opened.</p>
     *
     * <p>Normally whenever {@code equals} is implemented the {@code hashCode}
     * method should also be implemented to support hash sets and maps.
     * However, hash sets and maps containing <code>Conversion</code> objects
     * are not used by the DPL and therefore the DPL does not require
     * {@code hashCode} to be implemented.</p>
     */
    boolean equals(Object other);
}
