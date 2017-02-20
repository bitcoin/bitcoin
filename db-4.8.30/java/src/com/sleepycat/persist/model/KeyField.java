/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.model;

import static java.lang.annotation.ElementType.FIELD;
import static java.lang.annotation.RetentionPolicy.RUNTIME;

import java.lang.annotation.Documented;
import java.lang.annotation.Retention;
import java.lang.annotation.Target;

import com.sleepycat.db.Environment;

/**
 * Indicates the sorting position of a key field in a composite key class when
 * the {@code Comparable} interface is not implemented.  The {@code KeyField}
 * integer value specifies the sort order of this field within the set of
 * fields in the composite key.
 *
 * <p>If the field type of a {@link PrimaryKey} or {@link SecondaryKey} is a
 * composite key class containing more than one key field, then a {@code
 * KeyField} annotation must be present on each non-transient instance field of
 * the composite key class.  The {@code KeyField} value must be a number
 * between one and the number of non-transient instance fields declared in the
 * composite key class.</p>
 *
 * <p>Note that a composite key class is a flat container for one or more
 * simple type fields.  All non-transient instance fields in the composite key
 * class are key fields, and its superclass must be {@code Object}.</p>
 *
 * <p>For example:</p>
 * <pre class="code">
 *  {@literal @Entity}
 *  class Animal {
 *      {@literal @PrimaryKey}
 *      Classification classification;
 *      ...
 *  }
 *
 *  {@literal @Persistent}
 *  class Classification {
 *      {@literal @KeyField(1) String kingdom;}
 *      {@literal @KeyField(2) String phylum;}
 *      {@literal @KeyField(3) String clazz;}
 *      {@literal @KeyField(4) String order;}
 *      {@literal @KeyField(5) String family;}
 *      {@literal @KeyField(6) String genus;}
 *      {@literal @KeyField(7) String species;}
 *      {@literal @KeyField(8) String subspecies;}
 *      ...
 *  }</pre>
 *
 * <p>This causes entities to be sorted first by {@code kingdom}, then by
 * {@code phylum} within {@code kingdom}, and so on.</p>
 *
 * <p>The fields in a composite key class may not be null.</p>
 *
 * <p><a name="comparable"><strong>Custom Sort Order</strong></a></p>
 *
 * <p>To override the default sort order, a composite key class may implement
 * the {@link Comparable} interface.  This allows overriding the sort order and
 * is therefore useful even when there is only one key field in the composite
 * key class.  For example, the following class sorts Strings using a Canadian
 * collator:</p>
 *
 * <pre class="code">
 *  import java.text.Collator;
 *  import java.util.Locale;
 *
 *  {@literal @Entity}
 *  class Animal {
 *      ...
 *      {@literal @SecondaryKey(relate=ONE_TO_ONE)}
 *      CollatedString canadianName;
 *      ...
 *  }
 *
 *  {@literal @Persistent}
 *  {@literal class CollatedString implements Comparable<CollatedString>} {
 *
 *      static Collator collator = Collator.getInstance(Locale.CANADA);
 *
 *      {@literal @KeyField(1)}
 *      String value;
 *
 *      CollatedString(String value) { this.value = value; }
 *
 *      private CollatedString() {}
 *
 *      public int compareTo(CollatedString o) {
 *          return collator.compare(value, o.value);
 *      }
 *  }</pre>
 *
 * <p>Several important rules should be considered when implementing a custom
 * comparison method.  Failure to follow these rules may result in the primary
 * or secondary index becoming unusable; in other words, the store will not be
 * able to function.</p>
 * <ol>
 * <li>The comparison method must always return the same result, given the same
 * inputs.  The behavior of the comparison method must not change over
 * time.</li>
 * <br>
 * <li>A corollary to the first rule is that the behavior of the comparison
 * method must not be dependent on state which may change over time.  For
 * example, if the above collation method used the default Java locale, and the
 * default locale is changed, then the sort order will change.</li>
 * <br>
 * <li>The comparison method must not assume that it is called after the store
 * has been opened.  With Berkeley DB Java Edition, the comparison method is
 * called during database recovery, which occurs in the {@link Environment}
 * constructor.</li>
 * <br>
 * <li>The comparison method must not assume that it will only be called with
 * keys that are currently present in the database.  The comparison method will
 * occasionally be called with deleted keys or with keys for records that were
 * not part of a committed transaction.</li>
 * </ol>
 *
 * @author Mark Hayes
 */
@Documented @Retention(RUNTIME) @Target(FIELD)
public @interface KeyField {

    int value();
}
