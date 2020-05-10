/*
 * Copyright (c) 2008, 2013, Oracle and/or its affiliates. All rights reserved.
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

package sun.nio.cs;

import java.nio.charset.Charset;
import java.nio.charset.CharsetDecoder;
import java.nio.charset.CharsetEncoder;
import sun.nio.cs.StandardCharsets;
import sun.nio.cs.SingleByte;
import sun.nio.cs.HistoricallyNamedCharset;
import static sun.nio.cs.CharsetMapping.*;

public class MS1253 extends Charset implements HistoricallyNamedCharset
{
    public MS1253() {
        super("windows-1253", StandardCharsets.aliases_MS1253);
    }

    public String historicalName() {
        return "Cp1253";
    }

    public boolean contains(Charset cs) {
        return ((cs.name().equals("US-ASCII")) || (cs instanceof MS1253));
    }

    public CharsetDecoder newDecoder() {
        return new SingleByte.Decoder(this, b2c);
    }

    public CharsetEncoder newEncoder() {
        return new SingleByte.Encoder(this, c2b, c2bIndex);
    }

    private final static String b2cTable = 
        "\u20AC\uFFFD\u201A\u0192\u201E\u2026\u2020\u2021" +      // 0x80 - 0x87
        "\uFFFD\u2030\uFFFD\u2039\uFFFD\uFFFD\uFFFD\uFFFD" +      // 0x88 - 0x8f
        "\uFFFD\u2018\u2019\u201C\u201D\u2022\u2013\u2014" +      // 0x90 - 0x97
        "\uFFFD\u2122\uFFFD\u203A\uFFFD\uFFFD\uFFFD\uFFFD" +      // 0x98 - 0x9f
        "\u00A0\u0385\u0386\u00A3\u00A4\u00A5\u00A6\u00A7" +      // 0xa0 - 0xa7
        "\u00A8\u00A9\uFFFD\u00AB\u00AC\u00AD\u00AE\u2015" +      // 0xa8 - 0xaf
        "\u00B0\u00B1\u00B2\u00B3\u0384\u00B5\u00B6\u00B7" +      // 0xb0 - 0xb7
        "\u0388\u0389\u038A\u00BB\u038C\u00BD\u038E\u038F" +      // 0xb8 - 0xbf
        "\u0390\u0391\u0392\u0393\u0394\u0395\u0396\u0397" +      // 0xc0 - 0xc7
        "\u0398\u0399\u039A\u039B\u039C\u039D\u039E\u039F" +      // 0xc8 - 0xcf
        "\u03A0\u03A1\uFFFD\u03A3\u03A4\u03A5\u03A6\u03A7" +      // 0xd0 - 0xd7
        "\u03A8\u03A9\u03AA\u03AB\u03AC\u03AD\u03AE\u03AF" +      // 0xd8 - 0xdf
        "\u03B0\u03B1\u03B2\u03B3\u03B4\u03B5\u03B6\u03B7" +      // 0xe0 - 0xe7
        "\u03B8\u03B9\u03BA\u03BB\u03BC\u03BD\u03BE\u03BF" +      // 0xe8 - 0xef
        "\u03C0\u03C1\u03C2\u03C3\u03C4\u03C5\u03C6\u03C7" +      // 0xf0 - 0xf7
        "\u03C8\u03C9\u03CA\u03CB\u03CC\u03CD\u03CE\uFFFD" +      // 0xf8 - 0xff
        "\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007" +      // 0x00 - 0x07
        "\b\t\n\u000B\f\r\u000E\u000F" +      // 0x08 - 0x0f
        "\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017" +      // 0x10 - 0x17
        "\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F" +      // 0x18 - 0x1f
        "\u0020\u0021\"\u0023\u0024\u0025\u0026\'" +      // 0x20 - 0x27
        "\u0028\u0029\u002A\u002B\u002C\u002D\u002E\u002F" +      // 0x28 - 0x2f
        "\u0030\u0031\u0032\u0033\u0034\u0035\u0036\u0037" +      // 0x30 - 0x37
        "\u0038\u0039\u003A\u003B\u003C\u003D\u003E\u003F" +      // 0x38 - 0x3f
        "\u0040\u0041\u0042\u0043\u0044\u0045\u0046\u0047" +      // 0x40 - 0x47
        "\u0048\u0049\u004A\u004B\u004C\u004D\u004E\u004F" +      // 0x48 - 0x4f
        "\u0050\u0051\u0052\u0053\u0054\u0055\u0056\u0057" +      // 0x50 - 0x57
        "\u0058\u0059\u005A\u005B\\\u005D\u005E\u005F" +      // 0x58 - 0x5f
        "\u0060\u0061\u0062\u0063\u0064\u0065\u0066\u0067" +      // 0x60 - 0x67
        "\u0068\u0069\u006A\u006B\u006C\u006D\u006E\u006F" +      // 0x68 - 0x6f
        "\u0070\u0071\u0072\u0073\u0074\u0075\u0076\u0077" +      // 0x70 - 0x77
        "\u0078\u0079\u007A\u007B\u007C\u007D\u007E\u007F" ;      // 0x78 - 0x7f


    private final static char[] b2c = b2cTable.toCharArray();
    private final static char[] c2b = new char[0x600];
    private final static char[] c2bIndex = new char[0x100];

    static {
        char[] b2cMap = b2c;
        char[] c2bNR = null;
        SingleByte.initC2B(b2cMap, c2bNR, c2b, c2bIndex);
    }
}
