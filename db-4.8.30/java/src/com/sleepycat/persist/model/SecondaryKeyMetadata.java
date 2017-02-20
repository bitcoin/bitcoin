/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.model;

/**
 * The metadata for a secondary key field.  A secondary key may be specified
 * with the {@link SecondaryKey} annotation.
 *
 * <p>{@code SecondaryKeyMetadata} objects are thread-safe.  Multiple threads
 * may safely call the methods of a shared {@code SecondaryKeyMetadata}
 * object.</p>
 *
 * @author Mark Hayes
 */
public class SecondaryKeyMetadata extends FieldMetadata {

    private static final long serialVersionUID = 8118924993396722502L;

    private String keyName;
    private Relationship relationship;
    private String elementClassName;
    private String relatedEntity;
    private DeleteAction deleteAction;

    /**
     * Used by an {@code EntityModel} to construct secondary key metadata.
     */
    public SecondaryKeyMetadata(String name,
                                String className,
                                String declaringClassName,
                                String elementClassName,
                                String keyName,
                                Relationship relationship,
                                String relatedEntity,
                                DeleteAction deleteAction) {
        super(name, className, declaringClassName);
        this.elementClassName = elementClassName;
        this.keyName = keyName;
        this.relationship = relationship;
        this.relatedEntity = relatedEntity;
        this.deleteAction = deleteAction;
    }

    /**
     * Returns the class name of the array or collection element for a {@link
     * Relationship#ONE_TO_MANY ONE_TO_MANY} or {@link
     * Relationship#MANY_TO_MANY MANY_TO_MANY} relationship, or null for a
     * Relationship#ONE_TO_ONE ONE_TO_ONE} or {@link Relationship#MANY_TO_ONE
     * MANY_TO_ONE} relationship.
     */
    public String getElementClassName() {
        return elementClassName;
    }

    /**
     * Returns the key name, which may be different from the field name.
     */
    public String getKeyName() {
        return keyName;
    }

    /**
     * Returns the relationship between instances of the entity class and the
     * secondary keys.  This may be specified using the {@link
     * SecondaryKey#relate} annotation.
     */
    public Relationship getRelationship() {
        return relationship;
    }

    /**
     * Returns the class name of the related (foreign) entity, for which
     * foreign key constraints are specified using the {@link
     * SecondaryKey#relatedEntity} annotation.
     */
    public String getRelatedEntity() {
        return relatedEntity;
    }

    /**
     * Returns the action to take when a related entity is deleted having a
     * primary key value that exists as a secondary key value for this entity.
     * This may be specified using the {@link
     * SecondaryKey#onRelatedEntityDelete} annotation.
     */
    public DeleteAction getDeleteAction() {
        return deleteAction;
    }

    @Override
    public boolean equals(Object other) {
        if (other instanceof SecondaryKeyMetadata) {
            SecondaryKeyMetadata o = (SecondaryKeyMetadata) other;
            return super.equals(o) &&
                   relationship == o.relationship &&
                   ClassMetadata.nullOrEqual(deleteAction, o.deleteAction) &&
                   ClassMetadata.nullOrEqual(keyName, o.keyName) &&
                   ClassMetadata.nullOrEqual(elementClassName,
                                             o.elementClassName) &&
                   ClassMetadata.nullOrEqual(relatedEntity, o.relatedEntity);
        } else {
            return false;
        }
    }

    @Override
    public int hashCode() {
        return super.hashCode() +
               relationship.hashCode() +
               ClassMetadata.hashCode(deleteAction) +
               ClassMetadata.hashCode(keyName) +
               ClassMetadata.hashCode(elementClassName) +
               ClassMetadata.hashCode(relatedEntity);
    }
}
