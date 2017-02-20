/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2008-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.util;

public class ErrorBuffer {
	String[] msgs;
	int first, current;

	public ErrorBuffer(int num_msg) {
		msgs = new String[num_msg];
		clear();
	}

	public void append(String msg) {
		msgs[current] = msg;
		current = (current + 1) % msgs.length;
		if (current == first)
			first = (first + 1) % msgs.length;
	}

	public String get() {
		StringBuffer buf = new StringBuffer();
		for (int i = first; i != current; i = (i + 1) % msgs.length) {
			buf.append(msgs[i]);
			if ((i + 1) % msgs.length != current)
				buf.append("\n");
		}
		return buf.toString();
	}

	public void clear() {
		current = first = 0;
	}
}

