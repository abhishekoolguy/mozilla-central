/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=4 sw=4 et tw=80:
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "JavaScriptShared.h"
#include "jsfriendapi.h"
#include "xpcprivate.h"

using namespace js;
using namespace JS;
using namespace mozilla;
using namespace mozilla::jsipc;

ObjectStore::ObjectStore()
  : table_(SystemAllocPolicy())
{
}

bool
ObjectStore::init()
{
    return table_.init(32);
}

void
ObjectStore::trace(JSTracer *trc)
{
    for (ObjectTable::Range r(table_.all()); !r.empty(); r.popFront())
        JS_CallObjectTracer(trc, r.front().value, "ipc-object");
}

JSObject *
ObjectStore::find(ObjectId id)
{
    ObjectTable::Ptr p = table_.lookup(id);
    if (!p)
        return NULL;
    return p->value;
}

bool
ObjectStore::add(ObjectId id, JSObject *obj)
{
    return table_.put(id, obj);
}

void
ObjectStore::remove(ObjectId id)
{
    table_.remove(id);
}

ObjectIdCache::ObjectIdCache()
  : table_(SystemAllocPolicy())
{
}

bool
ObjectIdCache::init()
{
    return table_.init(32);
}

void
ObjectIdCache::trace(JSTracer *trc)
{
    for (ObjectIdTable::Range r(table_.all()); !r.empty(); r.popFront())
        JS_CallObjectTracer(trc, r.front().key, "ipc-id");
}

ObjectId
ObjectIdCache::find(JSObject *obj)
{
    ObjectIdTable::Ptr p = table_.lookup(obj);
    if (!p)
        return 0;
    return p->value;
}

bool
ObjectIdCache::add(JSObject *obj, ObjectId id)
{
    return table_.put(obj, id);
}

void
ObjectIdCache::remove(JSObject *obj)
{
    table_.remove(obj);
}

bool
JavaScriptShared::init()
{
    if (!objects_.init())
        return false;
    return true;
}

bool
JavaScriptShared::toGecko(JSContext *cx, jsid id, nsString *to)
{
    jsval idval;
    if (!JS_IdToValue(cx, id, &idval))
        return false;

    JSString *str = JS_ValueToString(cx, idval);
    if (!str)
        return false;

    const jschar *chars = JS_GetStringCharsZ(cx, str);
    if (!chars)
        return false;

    *to = chars;
    return true;
}

bool
JavaScriptShared::toVariant(JSContext *cx, jsval from, JSVariant *to)
{
    switch (JS_TypeOfValue(cx, from)) {
      case JSTYPE_VOID:
        *to = void_t();
        return true;

      case JSTYPE_NULL:
      {
        *to = uint32_t(0);
        return true;
      }

      case JSTYPE_OBJECT:
      case JSTYPE_FUNCTION:
      {
        JSObject *obj = JSVAL_TO_OBJECT(from);
        if (!obj) {
            JS_ASSERT(from == JSVAL_NULL);
            *to = uint32_t(0);
            return true;
        }

        if (xpc_JSObjectIsID(cx, obj)) {
            JSIID iid;
            const nsID *id = xpc_JSObjectToID(cx, obj);
            ConvertID(*id, &iid);
            *to = iid;
            return true;
        }

        ObjectId id;
        if (!makeId(cx, obj, &id))
            return false;
        *to = uint32_t(id);
        return true;
      }

      case JSTYPE_STRING:
      {
        nsDependentJSString dep;
        if (!dep.init(cx, from))
            return false;
        *to = dep;
        return true;
      }

      case JSTYPE_NUMBER:
        if (JSVAL_IS_INT(from))
            *to = double(JSVAL_TO_INT(from));
        else
            *to = JSVAL_TO_DOUBLE(from);
        return true;

      case JSTYPE_BOOLEAN:
        *to = !!JSVAL_TO_BOOLEAN(from);
        return true;

      default:
        MOZ_ASSERT(false);
        return false;
    }
}

bool
JavaScriptShared::toValue(JSContext *cx, const JSVariant &from, MutableHandleValue to)
{
    switch (from.type()) {
        case JSVariant::Tvoid_t:
          to.set(JSVAL_VOID);
          return true;

        case JSVariant::Tuint32_t:
        {
          uint32_t id = from.get_uint32_t();
          if (id) {
              JSObject *obj = unwrap(cx, id);
              if (!obj)
                  return false;
              to.set(OBJECT_TO_JSVAL(obj));
          } else {
              to.set(JSVAL_NULL);
          }
          return true;
        }

        case JSVariant::Tdouble:
          to.set(JS_NumberValue(from.get_double()));
          return true;

        case JSVariant::Tbool:
          to.set(BOOLEAN_TO_JSVAL(from.get_bool()));
          return true;

        case JSVariant::TnsString:
        {
          const nsString &old = from.get_nsString();
          JSString *str = JS_NewUCStringCopyN(cx, old.BeginReading(), old.Length());
          if (!str)
              return false;
          to.set(STRING_TO_JSVAL(str));
          return true;
        }

        case JSVariant::TJSIID:
        {
          nsID iid;
          const JSIID &id = from.get_JSIID();
          ConvertID(id, &iid);

          JSCompartment *compartment = GetContextCompartment(cx);
          RootedObject global(cx, JS_GetGlobalForCompartmentOrNull(cx, compartment));
          JSObject *obj = xpc_NewIDObject(cx, global, iid);
          if (!obj)
              return false;
          to.set(OBJECT_TO_JSVAL(obj));
          return true;
        }

        default:
          return false;
    }
}

/* static */ void
JavaScriptShared::ConvertID(const nsID &from, JSIID *to)
{
    to->m0() = from.m0;
    to->m1() = from.m1;
    to->m2() = from.m2;
    to->m3_0() = from.m3[0];
    to->m3_1() = from.m3[1];
    to->m3_2() = from.m3[2];
    to->m3_3() = from.m3[3];
    to->m3_4() = from.m3[4];
    to->m3_5() = from.m3[5];
    to->m3_6() = from.m3[6];
    to->m3_7() = from.m3[7];
}

/* static */ void
JavaScriptShared::ConvertID(const JSIID &from, nsID *to)
{
    to->m0 = from.m0();
    to->m1 = from.m1();
    to->m2 = from.m2();
    to->m3[0] = from.m3_0();
    to->m3[1] = from.m3_1();
    to->m3[2] = from.m3_2();
    to->m3[3] = from.m3_3();
    to->m3[4] = from.m3_4();
    to->m3[5] = from.m3_5();
    to->m3[6] = from.m3_6();
    to->m3[7] = from.m3_7();
}

static const uint32_t DefaultPropertyOp = 1;
static const uint32_t GetterOnlyPropertyStub = 2;
static const uint32_t UnknownPropertyOp = 3;

bool
JavaScriptShared::fromDesc(JSContext *cx, const JSPropertyDescriptor &desc, PPropertyDescriptor *out)
{
    out->attrs() = desc.attrs;
    out->shortid() = desc.shortid;
    if (!toVariant(cx, desc.value, &out->value()))
        return false;

    // rooting.
    if (!makeId(cx, desc.obj, &out->objId()))
        return false;

    if (!desc.getter) {
        out->getter() = 0;
    } else if (desc.attrs & JSPROP_GETTER) {
        JSObject *getter = JS_FUNC_TO_DATA_PTR(JSObject *, desc.getter);
        if (!makeId(cx, getter, &out->getter()))
            return false;
    } else {
        if (desc.getter == JS_PropertyStub)
            out->getter() = DefaultPropertyOp;
        else
            out->getter() = UnknownPropertyOp;
    }

    if (!desc.setter) {
        out->setter() = 0;
    } else if (desc.attrs & JSPROP_SETTER) {
        JSObject *setter = JS_FUNC_TO_DATA_PTR(JSObject  *, desc.setter);
        if (!makeId(cx, setter, &out->setter()))
            return false;
    } else {
        if (desc.setter == JS_StrictPropertyStub)
            out->setter() = DefaultPropertyOp;
        else if (desc.setter == js_GetterOnlyPropertyStub)
            out->setter() = GetterOnlyPropertyStub;
        else
            out->setter() = UnknownPropertyOp;
    }

    return true;
}

JSBool
UnknownPropertyStub(JSContext *cx, JSHandleObject obj, JSHandleId id, JSMutableHandleValue vp)
{
    JS_ReportError(cx, "getter could not be wrapped via CPOWs");
    return JS_FALSE;
}

JSBool
UnknownStrictPropertyStub(JSContext *cx, JSHandleObject obj, JSHandleId id, JSBool strict, JSMutableHandleValue vp)
{
    JS_ReportError(cx, "setter could not be wrapped via CPOWs");
    return JS_FALSE;
}

bool
JavaScriptShared::toDesc(JSContext *cx, const PPropertyDescriptor &in, JSPropertyDescriptor *out)
{
    out->attrs = in.attrs();
    out->shortid = in.shortid();
    if (!toValue(cx, in.value(), &out->value))
        return false;
    if (!unwrap(cx, in.objId(), &out->obj))
        return false;

    if (!in.getter()) {
        out->getter = NULL;
    } else if (in.attrs() & JSPROP_GETTER) {
        JSObject *getter;
        if (!unwrap(cx, in.getter(), &getter))
            return false;
        out->getter = JS_DATA_TO_FUNC_PTR(JSPropertyOp, getter);
    } else {
        if (in.getter() == DefaultPropertyOp)
            out->getter = JS_PropertyStub;
        else
            out->getter = UnknownPropertyStub;
    }

    if (!in.setter()) {
        out->setter = NULL;
    } else if (in.attrs() & JSPROP_SETTER) {
        JSObject *setter;
        if (!unwrap(cx, in.setter(), &setter))
            return false;
        out->setter = JS_DATA_TO_FUNC_PTR(JSStrictPropertyOp, setter);
    } else {
        if (in.setter() == DefaultPropertyOp)
            out->setter = JS_StrictPropertyStub;
        else if (in.setter() == GetterOnlyPropertyStub)
            out->setter = js_GetterOnlyPropertyStub;
        else
            out->setter = UnknownStrictPropertyStub;
    }

    return true;
}

bool
CpowIdHolder::ToObject(JSContext *cx, JSObject **objp)
{
    return js_->Unwrap(cx, cpows_, objp);
}

bool
JavaScriptShared::Unwrap(JSContext *cx, const InfallibleTArray<CpowEntry> &aCpows, JSObject **objp)
{
    *objp = NULL;

    if (!aCpows.Length())
        return true;

    JSObject *obj = JS_NewObject(cx, NULL, NULL, NULL);
    if (!obj)
        return false;

    for (size_t i = 0; i < aCpows.Length(); i++) {
        const nsString &name = aCpows[i].name();
        JSString *str = JS_NewUCStringCopyN(cx, name.BeginReading(), name.Length());
        if (!str)
            return false;

        jsid id;
        if (!JS_ValueToId(cx, STRING_TO_JSVAL(str), &id))
            return false;

        jsval v;
        if (!toValue(cx, aCpows[i].value(), &v))
            return false;

        if (!JS_DefinePropertyById(cx, obj, id, v, NULL, NULL, JSPROP_ENUMERATE))
            return false;
    }

    *objp = obj;
    return true;
}

bool
JavaScriptShared::Wrap(JSContext *cx, JSObject *aObj, InfallibleTArray<CpowEntry> *outCpows)
{
    if (!aObj)
        return true;

    AutoIdArray ids(cx, JS_Enumerate(cx, aObj));
    if (!ids)
        return false;

    for (size_t i = 0; i < ids.length(); i++) {
        jsid id = ids[i];

        nsString str;
        if (!toGecko(cx, id, &str))
            return false;

        jsval v;
        if (!JS_GetPropertyById(cx, aObj, id, &v))
            return false;

        JSVariant var;
        if (!toVariant(cx, v, &var))
            return false;

        outCpows->AppendElement(CpowEntry(str, var));
    }

    return true;
}
