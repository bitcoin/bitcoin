/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package collections.ship.tuple;

import java.io.Serializable;

/**
 * A PartData serves as the value in the key/value pair for a part entity.
 *
 * <p> In this sample, PartData is used only as the storage data for the
 * value, while the Part object is used as the value's object representation.
 * Because it is used directly as storage data using serial format, it must be
 * Serializable. </p>
 *
 * @author Mark Hayes
 */
public class PartData implements Serializable {

    private String name;
    private String color;
    private Weight weight;
    private String city;

    public PartData(String name, String color, Weight weight, String city) {

        this.name = name;
        this.color = color;
        this.weight = weight;
        this.city = city;
    }

    public final String getName() {

        return name;
    }

    public final String getColor() {

        return color;
    }

    public final Weight getWeight() {

        return weight;
    }

    public final String getCity() {

        return city;
    }

    public String toString() {

        return "[PartData: name=" + name +
	    " color=" + color +
	    " weight=" + weight +
	    " city=" + city + ']';
    }
}
