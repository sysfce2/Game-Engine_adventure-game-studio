//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

#include <stdlib.h>
#include <string.h>
#include "core/platform.h"
#include "ac/dynobj/cc_dynamicobject.h"
#include "ac/dynobj/managedobjectpool.h"
#include "debug/out.h"
#include "script/cc_common.h"
#include "util/stream.h"

using namespace AGS::Common;

ICCStringClass *stringClassImpl = nullptr;

// set the class that will be used for dynamic strings
void ccSetStringClassImpl(ICCStringClass *theClass) {
    stringClassImpl = theClass;
}

// register a memory handle for the object and allow script
// pointers to point to it
int32_t ccRegisterManagedObject(const void *object, ICCDynamicObject *callback, bool plugin_object) {
    return pool.AddObject((const char*)object, callback, plugin_object, false);
}

int32_t ccRegisterManagedObjectAndRef(const void *object, ICCDynamicObject *callback) {
    int32_t handle = pool.AddObject((const char*)object, callback, false, false);
    pool.AddRef(handle);
    return handle;
}

extern int32_t ccRegisterPersistentObject(const void *object, ICCDynamicObject *callback) {
    int32_t handle = pool.AddObject((const char*)object, callback, false, true);
    pool.AddRef(handle);
    return handle;
}

// register a de-serialized object
int32_t ccRegisterUnserializedObject(int index, const void *object, ICCDynamicObject *callback,
                                     bool plugin_object) {
    return pool.AddUnserializedObject((const char*)object, callback, index, plugin_object, false);
}

int32_t ccRegisterUnserializedPersistentObject(int index, const void *object, ICCDynamicObject *callback) {
    return pool.AddUnserializedObject((const char*)object, callback, index, false, true);
    // don't add ref, as it should come with the save data
}

// unregister a particular object
int ccUnRegisterManagedObject(const void *object) {
    return pool.RemoveObject((const char*)object);
}

// remove all registered objects
void ccUnregisterAllObjects() {
    pool.Reset();
}

// serialize all objects to disk
void ccSerializeAllObjects(Stream *out) {
    pool.WriteToDisk(out);
}

// un-serialise all objects (will remove all currently registered ones)
int ccUnserializeAllObjects(Stream *in, ICCObjectReader *callback) {
    return pool.ReadFromDisk(in, callback);
}

// dispose the object if RefCount==0
void ccAttemptDisposeObject(int32_t handle) {
    pool.CheckDispose(handle);
}

// translate between object handles and memory addresses
int32_t ccGetObjectHandleFromAddress(const void *address) {
    // set to null
    if (address == nullptr)
        return 0;

    int32_t handl = pool.AddressToHandle((const char*)address);

    ManagedObjectLog("Line %d WritePtr: %08X to %d", currentline, address, handl);

    if (handl == 0) {
        cc_error("Pointer cast failure: the object being pointed to is not in the managed object pool");
        return -1;
    }
    return handl;
}

const char *ccGetObjectAddressFromHandle(int32_t handle) {
    if (handle == 0) {
        return nullptr;
    }
    const char *addr = pool.HandleToAddress(handle);

    ManagedObjectLog("Line %d ReadPtr: %d to %08X", currentline, handle, addr);

    if (addr == nullptr) {
        cc_error("Error retrieving pointer: invalid handle %d", handle);
        return nullptr;
    }
    return addr;
}

ScriptValueType ccGetObjectAddressAndManagerFromHandle(int32_t handle, void *&object, ICCDynamicObject *&manager)
{
    if (handle == 0) {
        object = nullptr;
        manager = nullptr;
        return kScValUndefined;
    }
    ScriptValueType obj_type = pool.HandleToAddressAndManager(handle, object, manager);
    if (obj_type == kScValUndefined) {
        cc_error("Error retrieving pointer: invalid handle %d", handle);
    }
    return obj_type;
}

int ccAddObjectReference(int32_t handle) {
    if (handle == 0)
        return 0;

    return pool.AddRef(handle);
}

int ccReleaseObjectReference(int32_t handle) {
    if (handle == 0)
        return 0;

    if (pool.HandleToAddress(handle) == nullptr) {
        cc_error("Error releasing pointer: invalid handle %d", handle);
        return -1;
    }

    return pool.SubRefCheckDispose(handle);
}
