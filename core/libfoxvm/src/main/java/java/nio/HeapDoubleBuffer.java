/*
 * Copyright (c) 2000, 2013, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

// -- This file was mechanically generated: Do not edit! -- //

package java.nio;


/**

 * A read/write HeapDoubleBuffer.






 */

class HeapDoubleBuffer
    extends DoubleBuffer
{

    // For speed these fields are actually declared in X-Buffer;
    // these declarations are here as documentation
    /*

    protected final double[] hb;
    protected final int offset;

    */

    HeapDoubleBuffer(int cap, int lim) {            // package-private

        super(-1, 0, lim, cap, new double[cap], 0);
        /*
        hb = new double[cap];
        offset = 0;
        */




    }

    HeapDoubleBuffer(double[] buf, int off, int len) { // package-private

        super(-1, off, off + len, buf.length, buf, 0);
        /*
        hb = buf;
        offset = 0;
        */




    }

    protected HeapDoubleBuffer(double[] buf,
                                   int mark, int pos, int lim, int cap,
                                   int off)
    {

        super(mark, pos, lim, cap, buf, off);
        /*
        hb = buf;
        offset = off;
        */




    }

    public DoubleBuffer slice() {
        int pos = this.position();
        int lim = this.limit();
        int rem = (pos <= lim ? lim - pos : 0);
        return new HeapDoubleBuffer(hb,
                                        -1,
                                        0,
                                        rem,
                                        rem,
                                        pos + offset);
    }

    public DoubleBuffer duplicate() {
        return new HeapDoubleBuffer(hb,
                                        this.markValue(),
                                        this.position(),
                                        this.limit(),
                                        this.capacity(),
                                        offset);
    }

    public DoubleBuffer asReadOnlyBuffer() {

        return new HeapDoubleBufferR(hb,
                                     this.markValue(),
                                     this.position(),
                                     this.limit(),
                                     this.capacity(),
                                     offset);



    }



    protected int ix(int i) {
        return i + offset;
    }

    public double get() {
        return hb[ix(nextGetIndex())];
    }

    public double get(int i) {
        return hb[ix(checkIndex(i))];
    }







    public DoubleBuffer get(double[] dst, int offset, int length) {
        checkBounds(offset, length, dst.length);
        int pos = position();
        if (length > limit() - pos)
            throw new BufferUnderflowException();
        System.arraycopy(hb, ix(pos), dst, offset, length);
        position(pos + length);
        return this;
    }

    public boolean isDirect() {
        return false;
    }



    public boolean isReadOnly() {
        return false;
    }

    public DoubleBuffer put(double x) {

        hb[ix(nextPutIndex())] = x;
        return this;



    }

    public DoubleBuffer put(int i, double x) {

        hb[ix(checkIndex(i))] = x;
        return this;



    }

    public DoubleBuffer put(double[] src, int offset, int length) {

        checkBounds(offset, length, src.length);
        int pos = position();
        if (length > limit() - pos)
            throw new BufferOverflowException();
        System.arraycopy(src, offset, hb, ix(pos), length);
        position(pos + length);
        return this;



    }

    public DoubleBuffer put(DoubleBuffer src) {

        if (src instanceof HeapDoubleBuffer) {
            if (src == this)
                throw new IllegalArgumentException();
            HeapDoubleBuffer sb = (HeapDoubleBuffer)src;
            int pos = position();
            int sbpos = sb.position();
            int n = sb.limit() - sbpos;
            if (n > limit() - pos)
                throw new BufferOverflowException();
            System.arraycopy(sb.hb, sb.ix(sbpos),
                             hb, ix(pos), n);
            sb.position(sbpos + n);
            position(pos + n);
        } else if (src.isDirect()) {
            int n = src.remaining();
            int pos = position();
            if (n > limit() - pos)
                throw new BufferOverflowException();
            src.get(hb, ix(pos), n);
            position(pos + n);
        } else {
            super.put(src);
        }
        return this;



    }

    public DoubleBuffer compact() {

        int pos = position();
        int rem = limit() - pos;
        System.arraycopy(hb, ix(pos), hb, ix(0), rem);
        position(rem);
        limit(capacity());
        discardMark();
        return this;



    }












































































































































































































































































































































































    public ByteOrder order() {
        return ByteOrder.nativeOrder();
    }



}
