/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.impl;

import java.lang.reflect.Array;
import java.util.IdentityHashMap;
import java.util.Map;
import java.util.Set;

import com.sleepycat.persist.model.EntityModel;
import com.sleepycat.persist.model.PersistentProxy;
import com.sleepycat.persist.raw.RawObject;

/**
 * Format for types proxied by a PersistentProxy.
 *
 * @author Mark Hayes
 */
public class ProxiedFormat extends Format {

    private static final long serialVersionUID = -1000032651995478768L;

    private Format proxyFormat;
    private transient String proxyClassName;

    ProxiedFormat(Class proxiedType, String proxyClassName) {
        super(proxiedType);
        this.proxyClassName = proxyClassName;
    }

    /**
     * Returns the proxy class name.  The proxyClassName field is non-null for
     * a constructed object and null for a de-serialized object.  Whenever the
     * proxyClassName field is null (for a de-serialized object), the
     * proxyFormat will be non-null.
     */
    private String getProxyClassName() {
        if (proxyClassName != null) {
            return proxyClassName;
        } else {
            assert proxyFormat != null;
            return proxyFormat.getClassName();
        }
    }

    /**
     * In the future if we implement container proxies, which support nested
     * references to the container, then we will return false if this is a
     * container proxy.  [#15815]
     */
    @Override
    boolean areNestedRefsProhibited() {
        return true;
    }

    @Override
    void collectRelatedFormats(Catalog catalog,
                               Map<String,Format> newFormats) {
        /* Collect the proxy format. */
        assert proxyClassName != null;
        catalog.createFormat(proxyClassName, newFormats);
    }

    @Override
    void initialize(Catalog catalog, EntityModel model, int initVersion) {
        /* Set the proxy format for a new (never initialized) format. */
        if (proxyFormat == null) {
            assert proxyClassName != null;
            proxyFormat = catalog.getFormat(proxyClassName);
        }
        /* Make the linkage from proxy format to proxied format. */
        proxyFormat.setProxiedFormat(this);
    }

    @Override
    Object newArray(int len) {
        return Array.newInstance(getType(), len);
    }

    @Override
    public Object newInstance(EntityInput input, boolean rawAccess) {
        Reader reader = proxyFormat.getReader();
        if (rawAccess) {
            return reader.newInstance(null, true);
        } else {
            PersistentProxy proxy =
                (PersistentProxy) reader.newInstance(null, false);
            proxy = (PersistentProxy) reader.readObject(proxy, input, false);
            return proxy.convertProxy();
        }
    }

    @Override
    public Object readObject(Object o, EntityInput input, boolean rawAccess) {
        if (rawAccess) {
            o = proxyFormat.getReader().readObject(o, input, true);
        }
        /* Else, do nothing here -- newInstance reads the value. */
        return o;
    }

    @Override
    void writeObject(Object o, EntityOutput output, boolean rawAccess) {
        if (rawAccess) {
            proxyFormat.writeObject(o, output, true);
        } else {
            PersistentProxy proxy =
                (PersistentProxy) proxyFormat.newInstance(null, false);
            proxy.initializeProxy(o);
            proxyFormat.writeObject(proxy, output, false);
        }
    }

    @Override
    Object convertRawObject(Catalog catalog,
                            boolean rawAccess,
                            RawObject rawObject,
                            IdentityHashMap converted) {
        PersistentProxy proxy = (PersistentProxy) proxyFormat.convertRawObject
            (catalog, rawAccess, rawObject, converted);
        Object o = proxy.convertProxy();
        converted.put(rawObject, o);
        return o;
    }

    @Override
    void skipContents(RecordInput input) {
        proxyFormat.skipContents(input);
    }

    @Override
    void copySecMultiKey(RecordInput input, Format keyFormat, Set results) {
        CollectionProxy.copyElements(input, this, keyFormat, results);
    }

    @Override
    boolean evolve(Format newFormatParam, Evolver evolver) {
        if (!(newFormatParam instanceof ProxiedFormat)) {
            evolver.addEvolveError
                (this, newFormatParam, null,
                 "A proxied class may not be changed to a different type");
            return false;
        }
        ProxiedFormat newFormat = (ProxiedFormat) newFormatParam;
        if (!evolver.evolveFormat(proxyFormat)) {
            return false;
        }
        Format newProxyFormat = proxyFormat.getLatestVersion();
        if (!newProxyFormat.getClassName().equals
                (newFormat.getProxyClassName())) {
            evolver.addEvolveError
                (this, newFormat, null,
                 "The proxy class for this type has been changed from: " +
                 newProxyFormat.getClassName() + " to: " +
                 newFormat.getProxyClassName());
            return false;
        }
        if (newProxyFormat != proxyFormat) {
            evolver.useEvolvedFormat(this, this, newFormat);
        } else {
            evolver.useOldFormat(this, newFormat);
        }
        return true;
    }
}
