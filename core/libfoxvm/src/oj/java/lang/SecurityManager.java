/*
 * Copyright (c) 1995, 2013, Oracle and/or its affiliates. All rights reserved.
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

package java.lang;

import java.io.FileDescriptor;
import java.net.InetAddress;
import java.security.Permission;

/**
 * FOXVM-changed: FoxVM does not support SecurityManager.
 */
public
class SecurityManager {

    /**
     * @deprecated Use <code>checkPermission</code> instead.
     */
    @Deprecated
    protected boolean inCheck;

    /**
     * @deprecated Use <code>checkPermission</code> instead.
     */
    @Deprecated
    public boolean getInCheck() {
        return inCheck;
    }

    public SecurityManager() {
    }

    protected Class[] getClassContext() {
        return null;
    };

    /**
     * @deprecated Use <code>checkPermission</code> instead.
     */
    @Deprecated
    protected ClassLoader currentClassLoader()
    {
        return null;
    }

    /**
     * @deprecated Use <code>checkPermission</code> instead.
     */
    @Deprecated
    protected Class<?> currentLoadedClass() {
        return null;
    }

    /**
     * @deprecated Use <code>checkPermission</code> instead.
     */
    @Deprecated
    protected int classDepth(String name){
        return -1;
    }

    /**
     * @deprecated Use <code>checkPermission</code> instead.
     */
    @Deprecated
    protected int classLoaderDepth()
    {
        return -1;
    }

    /**
     * @deprecated Use <code>checkPermission</code> instead.
     */
    @Deprecated
    protected boolean inClass(String name) {
        return false;
    }

    /**
     * @deprecated Use <code>checkPermission</code> instead.
     */
    @Deprecated
    protected boolean inClassLoader() {
        return false;
    }

    public Object getSecurityContext() {
        return null;
    }

    public void checkPermission(Permission perm) {
    }

    public void checkPermission(Permission perm, Object context) {
    }

    public void checkCreateClassLoader() {
    }

    public void checkAccess(Thread t) {
    }

    public void checkAccess(ThreadGroup g) {
    }

    public void checkExit(int status) {
    }

    public void checkExec(String cmd) {
    }

    public void checkLink(String lib) {
    }

    public void checkRead(FileDescriptor fd) {
    }

    public void checkRead(String file) {
    }

    public void checkRead(String file, Object context) {
    }

    public void checkWrite(FileDescriptor fd) {
    }

    public void checkWrite(String file) {
    }

    public void checkDelete(String file) {
    }

    public void checkConnect(String host, int port) {
    }

    public void checkConnect(String host, int port, Object context) {
    }

    public void checkListen(int port) {
    }

    public void checkAccept(String host, int port) {
    }

    public void checkMulticast(InetAddress maddr) {
    }

    /**
     * @deprecated Use <code>checkPermission</code> instead.
     */
    @Deprecated
    public void checkMulticast(InetAddress maddr, byte ttl) {
    }

    public void checkPropertiesAccess() {
    }

    public void checkPropertyAccess(String key) {
    }

    /**
     * @deprecated Use <code>checkPermission</code> instead.
     */
    @Deprecated
    public boolean checkTopLevelWindow(Object window) {
        return true;
    }

    public void checkPrintJobAccess() {
    }

    /**
     * @deprecated Use <code>checkPermission</code> instead.
     */
    @Deprecated
    public void checkSystemClipboardAccess() {
    }

    /**
     * @deprecated Use <code>checkPermission</code> instead.
     */
    @Deprecated
    public void checkAwtEventQueueAccess() {
    }

    public void checkPackageAccess(String pkg) {
    }

    public void checkPackageDefinition(String pkg) {
    }

    public void checkSetFactory() {
    }

    /**
     * @deprecated Use <code>checkPermission</code> instead.
     */
    @Deprecated
    public void checkMemberAccess(Class<?> clazz, int which) {
    }

    public void checkSecurityAccess(String target) {
    }

    /**
     * Returns the thread group into which to instantiate any new
     * thread being created at the time this is being called.
     * By default, it returns the thread group of the current
     * thread. This should be overridden by a specific security
     * manager to return the appropriate thread group.
     *
     * @return  ThreadGroup that new threads are instantiated into
     * @since   JDK1.1
     * @see     java.lang.ThreadGroup
     */
    public ThreadGroup getThreadGroup() {
        return Thread.currentThread().getThreadGroup();
    }

}
