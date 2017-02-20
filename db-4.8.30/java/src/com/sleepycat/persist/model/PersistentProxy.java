/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.model;

import com.sleepycat.persist.evolve.Converter; // for javadoc
import com.sleepycat.persist.raw.RawStore; // for javadoc

/**
 * Implemented by a proxy class to represent the persistent state of a
 * (non-persistent) proxied class.  Normally classes that are outside the scope
 * of the developer's control must be proxied since they cannot be annotated,
 * and because it is desirable to insulate the stored format from changes to
 * the instance fields of the proxied class.  This is useful for classes in the
 * standard Java libraries, for example.
 *
 * <p>{@code PersistentProxy} objects are not required to be thread-safe.  A
 * single thread will create and call the methods of a given {@code
 * PersistentProxy} object.</p>
 *
 * <p>There are three requirements for a proxy class:</p>
 * <ol>
 * <li>It must implement the <code>PersistentProxy</code> interface.</li>
 * <li>It must be specified as a persistent proxy class in the entity model.
 * When using the {@link AnnotationModel}, a proxy class is indicated by the
 * {@link Persistent} annotation with the {@link Persistent#proxyFor}
 * property.</li>
 * <li>It must be explicitly registered by calling {@link
 * EntityModel#registerClass} before opening the store.</li>
 * </ol>
 *
 * <p>In order to serialize an instance of the proxied class before it is
 * stored, an instance of the proxy class is created.  The proxied instance is
 * then passed to the proxy's {@link #initializeProxy initializeProxy} method.
 * When this method returns, the proxy instance contains the state of the
 * proxied instance.  The proxy instance is then serialized and stored in the
 * same way as for any persistent object.</p>
 *
 * <p>When an instance of the proxy object is deserialized after it is
 * retrieved from storage, its {@link #convertProxy} method is called.  The
 * instance of the proxied class returned by this method is then returned as a
 * field in the persistent instance.</p>
 *
 * <p>For example:</p>
 * <pre class="code">
 *  import java.util.Locale;
 *
 *  {@literal @Persistent(proxyFor=Locale.class)}
 *  class LocaleProxy implements {@literal PersistentProxy<Locale>} {
 *
 *      String language;
 *      String country;
 *      String variant;
 *
 *      private LocaleProxy() {}
 *
 *      public void initializeProxy(Locale object) {
 *          language = object.getLanguage();
 *          country = object.getCountry();
 *          variant = object.getVariant();
 *      }
 *
 *      public Locale convertProxy() {
 *          return new Locale(language, country, variant);
 *      }
 *  }</pre>
 *
 * <p>The above definition allows the {@code Locale} class to be used in any
 * persistent class, for example:</p>
 * <pre class="code">
 *  {@literal @Persistent}
 *  class LocalizedText {
 *      String text;
 *      Locale locale;
 *  }</pre>
 *
 * <p>A proxied class may not be used as a superclass for a persistent class or
 * entity class.  For example, the following is not allowed.</p>
 * <pre class="code">
 *  {@literal @Persistent}
 *  class LocalizedText extends Locale { // NOT ALLOWED
 *      String text;
 *  }</pre>
 *
 * <p>A proxy for proxied class P does not handle instances of subclasses of P.
 * To proxy a subclass of P, a separate proxy class is needed.</p>
 *
 * <p>Several {@link <a href="Entity.html#proxyTypes">built in proxy types</a>}
 * are used implicitly.  An application defined proxy will be used instead of a
 * built-in proxy, if both exist for the same proxied class.</p>
 *
 * <p>With respect to class evolution, a proxy instance is no different than
 * any other persistent instance.  When using a {@link RawStore} or {@link
 * Converter}, only the raw data of the proxy instance will be visible.  Raw
 * data for the proxied instance never exists.</p>
 *
 * <p>Currently a proxied object may not contain a reference to itself.  For
 * simple proxied objects such as the Locale class shown above, this naturally
 * won't occur.  But for proxied objects that are containers -- the built-in
 * Collection and Map classes for example -- this can occur if the container is
 * added as an element of itself.  This should be avoided.  If an attempt to
 * store such an object is made, an {@code IllegalArgumentException} will be
 * thrown.</p>
 *
 * <p>Note that a proxy class may not be a subclass of an entity class.</p>
 *
 * @author Mark Hayes
 */
public interface PersistentProxy<T> {

    /**
     * Copies the state of a given proxied class instance to this proxy
     * instance.
     */
    void initializeProxy(T object);

    /**
     * Returns a new proxied class instance to which the state of this proxy
     * instance has been copied.
     */
    T convertProxy();
}
