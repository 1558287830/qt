/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtScript module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL-ONLY$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qscriptconverter_p.h"
#include "qscriptengine.h"
#include "qscriptengine_p.h"
#include "qscriptqobject_p.h"

#include <QtCore/qmetaobject.h>
#include <QtCore/qvarlengtharray.h>

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

QtInstanceData::QtInstanceData(QScriptEnginePrivate *engine, QObject *object,
                               QScriptEngine::ValueOwnership own,
                               const QScriptEngine::QObjectWrapOptions &opt)
    : m_engine(engine), m_cppObject(object), m_own(own), m_opt(opt)
{
}

QtInstanceData::~QtInstanceData()
{
//    qDebug("~QtInstanceData()");
    switch (m_own) {
    case QScriptEngine::QtOwnership:
        break;
    case QScriptEngine::AutoOwnership:
        if (m_cppObject && m_cppObject->parent())
            break;
    case QScriptEngine::ScriptOwnership:
        delete m_cppObject;
        break;
    }
}



// Connects this signal to the given callback.
// Returns undefined if the connection succeeded, otherwise throws an error.
v8::Handle<v8::Value> QtSignalData::connect(v8::Handle<v8::Function> callback, Qt::ConnectionType type)
{
     QtConnection *connection = new QtConnection(this);
     if (!connection->connect(callback, type)) {
         delete connection;
         return v8::ThrowException(v8::Exception::Error(v8::String::New("QtSignal.connect(): failed to connect")));
     }
     m_connections.append(connection);
     return v8::Undefined();
}

// Disconnect this signal from the given callback.
// Returns undefined if the disconnection succeeded, otherwise throws an error.
v8::Handle<v8::Value> QtSignalData::disconnect(v8::Handle<v8::Function> callback)
{
    for (int i = 0; i < m_connections.size(); ++i) {
        QtConnection *connection = m_connections.at(i);
        if (connection->callback()->StrictEquals(callback)) {
            if (!connection->disconnect())
                return v8::ThrowException(v8::Exception::Error(v8::String::New("QtSignal.disconnect(): failed to disconnect")));
            m_connections.removeAt(i);
            delete connection;
            return v8::Undefined();
        }
    }
    return v8::ThrowException(v8::Exception::Error(v8::String::New("QtSignal.disconnect(): function not connected to this signal")));
}


QtConnection::QtConnection(QtSignalData *signal)
    : m_signal(signal)
{
}

// Connects to this connection's signal, and binds this connection to the
// given callback.
// Returns true if the connection succeeded, otherwise returns false.
bool QtConnection::connect(v8::Handle<v8::Function> callback, Qt::ConnectionType type)
{
    Q_UNUSED(type); // ###
    Q_ASSERT(m_callback.IsEmpty());
    QtInstanceData *instance = QtInstanceData::get(m_signal->object());
    bool ok = QMetaObject::connect(instance->cppObject(), m_signal->index(),
                                   this, staticMetaObject.methodOffset());
    if (ok)
        m_callback = v8::Persistent<v8::Function>::New(callback);
    return ok;
}

// Disconnects from this connection's signal, and unbinds the callback.
bool QtConnection::disconnect()
{
    Q_ASSERT(!m_callback.IsEmpty());
    QtInstanceData *instance = QtInstanceData::get(m_signal->object());
    bool ok = QMetaObject::disconnect(instance->cppObject(), m_signal->index(),
                                      this, staticMetaObject.methodOffset());
    if (ok) {
        m_callback.Dispose();
        m_callback.Clear();
    }
    return ok;
}

// This slot is called when the C++ signal is emitted.
// It forwards the call to the JS callback.
void QtConnection::onSignal(void **argv)
{
    Q_ASSERT(!m_callback.IsEmpty());

    QScriptEnginePrivate *engine = QtInstanceData::get(m_signal->object())->engine();
    v8::Context::Scope contextScope(*engine);
    v8::HandleScope handleScope;

    const QMetaObject *meta = sender()->metaObject();
    QMetaMethod method = meta->method(m_signal->index());


    QList<QByteArray> parameterTypes = method.parameterTypes();
    int argc = parameterTypes.count();

    // Convert arguments to JS.
    QVarLengthArray<v8::Handle<v8::Value>, 8> jsArgv(argc);
    for (int i = 0; i < argc; ++i) {
        v8::Handle<v8::Value> convertedArg;
        void *arg = argv[i + 1];
        QByteArray typeName = parameterTypes.at(i);
        int type = QMetaType::type(typeName);
        if (!type) {
            qWarning("Unable to handle unregistered datatype '%s' "
                     "when invoking signal handler for %s::%s",
                     typeName.constData(), meta->className(), method.signature());
            convertedArg = v8::Undefined();
        } else {
            convertedArg = engine->metaTypeToJS(type, arg);
        }
        jsArgv[i] = convertedArg;
    }

    v8::TryCatch tryCatch;
    v8::Handle<v8::Object> receiver = m_signal->object();
    v8::Handle<v8::Value> result = m_callback->Call(receiver, argc, const_cast<v8::Handle<v8::Value>*>(jsArgv.constData()));
    if (result.IsEmpty()) {
        // ### emit signalHandlerException()
        Q_UNIMPLEMENTED();
    }
}


// moc-generated code.
// DO NOT EDIT!

static const uint qt_meta_data_QtConnection[] = {

 // content:
       5,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      14,   13,   13,   13, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_QtConnection[] = {
    "QtConnection\0\0onSignal()\0"
};

const QMetaObject QtConnection::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_QtConnection,
      qt_meta_data_QtConnection, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QtConnection::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QtConnection::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QtConnection::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QtConnection))
        return static_cast<void*>(const_cast< QtConnection*>(this));
    return QObject::qt_metacast(_clname);
}

int QtConnection::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: onSignal(_a); break;
        default: ;
        }
        _id -= 1;
    }
    return _id;
}



#if 0
struct StringType
{
    typedef QString Type;

    static v8::Handle<v8::Value> toJS(const QString &s)
    { return qtStringToJS(s); }

    static QString fromJS(v8::Handle<v8::Value> v)
    { return qtStringFromJS(v->ToString()); }
};

struct DoubleType
{
    typedef double Type;

    static v8::Handle<v8::Value> toJS(double d)
    { return v8::Number::New(d); }

    static double fromJS(v8::Handle<v8::Value> v)
    { return v->ToNumber()->Value(); }
};

struct BoolType
{
    typedef bool Type;

    static v8::Handle<v8::Value> toJS(bool b)
    { return v8::Boolean::New(b); }

    static bool fromJS(v8::Handle<v8::Value> v)
    { return v->ToBoolean()->Value(); }
};

template <class Type>
static v8::Handle<v8::Value> QtMetaPropertyFastGetter(v8::Local<v8::String> /*property*/,
                                                      const v8::AccessorInfo& info)
{
    v8::Local<v8::Object> self = info.Holder();
    QtInstanceData *data = QtInstanceData::get(self);

    QObject *qobject = data->cppObject();

    int propertyIndex = v8::Int32::Cast(*info.Data())->Value();

    typename Type::Type v;
    void *argv[] = { &v };

    QMetaObject::metacall(qobject, QMetaObject::ReadProperty, propertyIndex, argv);

    return Type::toJS(v);
}

template <class Type>
static void QtMetaPropertyFastSetter(v8::Local<v8::String> /*property*/,
                                     v8::Local<v8::Value> value,
                                     const v8::AccessorInfo& info)
{
    v8::Local<v8::Object> self = info.Holder();
    QtInstanceData *data = QtInstanceData::get(self);

    QObject *qobject = data->cppObject();

    int propertyIndex = v8::Int32::Cast(*info.Data())->Value();

    typename Type::Type v = Type::fromJS(value);
    void *argv[] = { &v };

    QMetaObject::metacall(qobject, QMetaObject::WriteProperty, propertyIndex, argv);
}
#endif


// This callback implements meta-object-defined property reads for objects
// that don't inherit QScriptable.
// - info.Holder() is a QObject wrapper
// - info.Data() is the meta-property index
static v8::Handle<v8::Value> QtMetaPropertyGetter(v8::Local<v8::String> /*property*/,
                                                  const v8::AccessorInfo& info)
{
    v8::Local<v8::Object> self = info.Holder();
    QtInstanceData *data = QtInstanceData::get(self);
    QScriptEnginePrivate *engine = data->engine();

    QObject *qobject = data->cppObject();
    Q_ASSERT(qobject != 0);
    const QMetaObject *meta = qobject->metaObject();

    int propertyIndex = v8::Int32::Cast(*info.Data())->Value();

    QMetaProperty prop = meta->property(propertyIndex);
    Q_ASSERT(prop.isReadable());

    QVariant value = prop.read(qobject);

    return engine->variantToJS(value);
}

// This callback implements meta-object-defined property writes for objects
// that don't inherit QScriptable.
// - info.Holder() is a QObject wrapper
// - info.Data() is the meta-property index
static void QtMetaPropertySetter(v8::Local<v8::String> /*property*/,
                                 v8::Local<v8::Value> value,
                                 const v8::AccessorInfo& info)
{
    v8::Local<v8::Object> self = info.Holder(); // This?
    QtInstanceData *data = QtInstanceData::get(self);
    QScriptEnginePrivate *engine = data->engine();

    QObject *qobject = data->cppObject();
    Q_ASSERT(qobject != 0);
    const QMetaObject *meta = qobject->metaObject();

    int propertyIndex = v8::Int32::Cast(*info.Data())->Value();

    QMetaProperty prop = meta->property(propertyIndex);
    Q_ASSERT(prop.isWritable());

    QVariant cppValue;
#if 0
    if (prop.isEnumType() && value->IsString()
        && !engine->hasDemarshalFunction(prop.userType())) {
        // Give QMetaProperty::write() a chance to convert from
        // string to enum value.
        cppValue = qtStringFromJS(value->ToString());
    } else
#endif
    {
        cppValue = QVariant(prop.userType(), (void *)0);
        if (!engine->metaTypeFromJS(value, cppValue.userType(), cppValue.data())) {
            // Needs more magic.
            Q_UNIMPLEMENTED();
        }
    }

    prop.write(qobject, cppValue);
}

// This callback implements reading a presumably existing dynamic property.
// Unlike meta-object-defined properties, dynamic properties are specific to
// an instance (not tied to a class). Dynamic properties can be added,
// changed and removed at any time. If the dynamic property with the given
// name no longer exists, this accessor will be uninstalled.
static v8::Handle<v8::Value> QtDynamicPropertyGetter(v8::Local<v8::String> property,
                                                     const v8::AccessorInfo& info)
{
    v8::Local<v8::Object> self = info.Holder(); // This?
    QtInstanceData *data = QtInstanceData::get(self);
    QScriptEnginePrivate *engine = data->engine();
    QObject *qobject = data->cppObject();

    QByteArray name = QScriptConverter::toString(property).toLatin1();
    QVariant value = qobject->property(name);
    if (!value.isValid()) {
        // The property no longer exists. Remove this accessor.
        self->ForceDelete(property);
        // ### Make sure this causes fallback to interceptor
        return v8::Handle<v8::Value>();
    }
    return engine->variantToJS(value);
}

// This callback implements writing a presumably existing dynamic property.
// If the dynamic property with the given name no longer exists, this accessor
// will be uninstalled.
static void QtDynamicPropertySetter(v8::Local<v8::String> property,
                                    v8::Local<v8::Value> value,
                                    const v8::AccessorInfo& info)
{
    v8::Local<v8::Object> self = info.Holder(); // This?
    QtInstanceData *data = QtInstanceData::get(self);
    QScriptEnginePrivate *engine = data->engine();
    QObject *qobject = data->cppObject();

    QByteArray name = QScriptConverter::toString(property).toLatin1();
    if (qobject->dynamicPropertyNames().indexOf(name) == -1) {
        // The property no longer exists. Remove this accessor.
        self->ForceDelete(property);
        // Call normal logic for property writes.
        Q_UNIMPLEMENTED();
    }

    QVariant cppValue = engine->variantFromJS(value);
    qobject->setProperty(name, cppValue);
}

// Generic implementation of Qt meta-method invocation.
// Uses QMetaType and friends to resolve types and convert arguments.
v8::Handle<v8::Value> callQtMetaMethod(QScriptEnginePrivate *engine, QObject *qobject,
                                       const QMetaObject *meta, int methodIndex,
                                       const v8::Arguments& args)
{
    QMetaMethod method = meta->method(methodIndex);
    QList<QByteArray> parameterTypeNames = method.parameterTypes();

    if (args.Length() < parameterTypeNames.size()) {
        // Throw error: too few args.
        Q_UNIMPLEMENTED();
    }

    int rtype = QMetaType::type(method.typeName());

    QVarLengthArray<QVariant, 10> cppArgs(1 + parameterTypeNames.size());
    cppArgs[0] = QVariant(rtype, (void *)0);

    // Convert arguments to C++ types.
    for (int i = 0; i < parameterTypeNames.size(); ++i) {
        int atype = QMetaType::type(parameterTypeNames.at(i));
        if (!atype)
            Q_UNIMPLEMENTED();

        v8::Handle<v8::Value> actual = args[i];
        QVariant v(atype, (void *)0);
        if (!engine->metaTypeFromJS(actual, atype, v.data())) {
            // Throw TypeError.
            Q_UNIMPLEMENTED();
        }
        cppArgs[1+i] = v;
    }

    // Prepare void** array for metacall.
    QVarLengthArray<void *, 10> argv(cppArgs.size());
    void **argvData = argv.data();
    for (int i = 0; i < cppArgs.size(); ++i)
        argvData[i] = const_cast<void *>(cppArgs[i].constData());

    // Call the C++ method!
    QMetaObject::metacall(qobject, QMetaObject::InvokeMetaMethod, methodIndex, argvData);

    // Convert and return result.
    return engine->metaTypeToJS(rtype, argvData[0]);
}

// This callback implements a Qt slot or Q_INVOKABLE call (non-overloaded).
static v8::Handle<v8::Value> QtMetaMethodCallback(const v8::Arguments& args)
{
    v8::Local<v8::Object> self = args.Holder(); // This??
    QtInstanceData *instance = QtInstanceData::get(self);

    QScriptEnginePrivate *engine = instance->engine();
    QObject *qobject = instance->cppObject();
    const QMetaObject *meta = qobject->metaObject();

    int methodIndex = v8::Int32::Cast(*args.Data())->Value();

    return callQtMetaMethod(engine, qobject, meta, methodIndex, args);
}

static inline bool methodNameEquals(const QMetaMethod &method,
                                    const char *signature, int nameLength)
{
    const char *otherSignature = method.signature();
    return !qstrncmp(otherSignature, signature, nameLength)
        && (otherSignature[nameLength] == '(');
}

static inline int methodNameLength(const char *signature)
{
    const char *s = signature;
    while (*s && (*s != '('))
        ++s;
    return s - signature;
}

static int conversionDistance(QScriptEnginePrivate *engine, v8::Handle<v8::Value> actual, int targetType)
{
    if (actual->IsNumber()) {
        switch (targetType) {
        case QMetaType::Double:
            // perfect
            return 0;
        case QMetaType::Float:
            return 1;
        case QMetaType::LongLong:
        case QMetaType::ULongLong:
            return 2;
        case QMetaType::Long:
        case QMetaType::ULong:
            return 3;
        case QMetaType::Int:
        case QMetaType::UInt:
            return 4;
        case QMetaType::Short:
        case QMetaType::UShort:
            return 5;
        case QMetaType::Char:
        case QMetaType::UChar:
            return 6;
        default:
            return 10;
        }
    } else if (actual->IsString()) {
        switch (targetType) {
        case QMetaType::QString:
            // perfect
            return 0;
        default:
            return 10;
        }
    } else if (actual->IsBoolean()) {
        switch (targetType) {
        case QMetaType::Bool:
            // perfect
            return 0;
        default:
            return 10;
        }
    } else if (actual->IsDate()) {
        switch (targetType) {
        case QMetaType::QDateTime:
            return 0;
        case QMetaType::QDate:
            return 1;
        case QMetaType::QTime:
            return 2;
        default:
            return 10;
        }
    }
#if 0
    else if (actual->IsRegExp()) {
        switch (targetType) {
        case QMetaType::QRegExp:
            // perfect
            return 0;
        default:
            return 10;
        }
    }
#endif
    else if (engine->isQtVariant(actual)) {
        if ((targetType == QMetaType::QVariant)
            || (engine->variantFromJS(actual).userType() == targetType)) {
            // perfect
            return 0;
        }
        return 10;
    } else if (actual->IsArray()) {
        switch (targetType) {
        case QMetaType::QStringList:
        case QMetaType::QVariantList:
            return 5;
        default:
            return 10;
        }
    } else if (engine->isQtObject(actual)) {
        switch (targetType) {
        case QMetaType::QObjectStar:
        case QMetaType::QWidgetStar:
            // perfect
            return 0;
        default:
            return 10;
        }
    } else if (actual->IsNull()) {
        switch (targetType) {
        case QMetaType::VoidStar:
        case QMetaType::QObjectStar:
        case QMetaType::QWidgetStar:
            // perfect
            return 0;
        default:
            return 10;
        }
    }
    return 100;
}

static int resolveOverloadedQtMetaMethodCall(QScriptEnginePrivate *engine, const QMetaObject *meta, int initialIndex, const v8::Arguments& args)
{
    int bestIndex = -1;
    int bestDistance = INT_MAX;
    int nameLength = 0;
    const char *initialMethodSignature = 0;
    for (int index = initialIndex; index >= 0; --index) {
        QMetaMethod method = meta->method(index);
        if (index == initialIndex) {
            initialMethodSignature = method.signature();
            nameLength = methodNameLength(initialMethodSignature);
        } else {
            if (!methodNameEquals(method, initialMethodSignature, nameLength))
                continue;
        }

        QList<QByteArray> parameterTypeNames = method.parameterTypes();
        int parameterCount = parameterTypeNames.size();
        if (args.Length() != parameterCount)
            continue;

        int distance = 0;
        for (int i = 0; (distance < bestDistance) && (i < parameterCount); ++i) {
            int targetType = QMetaType::type(parameterTypeNames.at(i));
            Q_ASSERT(targetType != 0);
            distance += conversionDistance(engine, args[i], targetType);
        }

        if (distance == 0) {
            // Perfect match, no need to look further.
            return index;
        }

        if (distance < bestDistance) {
            bestIndex = index;
            bestDistance = distance;
        }
    }
    return bestIndex;
}

// This callback implements a Qt slot or Q_INVOKABLE call (overloaded).
// In this case the types of the actual arguments must be taken into
// account to decide which C++ overload should be called.
static v8::Handle<v8::Value> QtOverloadedMetaMethodCallback(const v8::Arguments& args)
{
    v8::Local<v8::Object> self = args.Holder(); // This??
    QtInstanceData *instance = QtInstanceData::get(self);
    QScriptEnginePrivate *engine = instance->engine();

    QObject *qobject = instance->cppObject();
    const QMetaObject *meta = qobject->metaObject();

    int methodIndex = v8::Int32::Cast(*args.Data())->Value();
    methodIndex = resolveOverloadedQtMetaMethodCall(engine, meta, methodIndex, args);
    if (methodIndex == -1) {
        // Ambiguous call.
        Q_ASSERT(0);
    }

    return callQtMetaMethod(engine, qobject, meta, methodIndex, args);
}

// This callback implements calls of meta-methods that have no arguments and
// return void.
// Having a specialized callback allows us to avoid performing type resolution
// and such.
static v8::Handle<v8::Value> QtVoidVoidMetaMethodCallback(const v8::Arguments& args)
{
    v8::Local<v8::Object> self = args.Holder(); // This??
    QtInstanceData *data = QtInstanceData::get(self);
    QObject *qobject = data->cppObject();

    int methodIndex = v8::Int32::Cast(*args.Data())->Value();

    QMetaObject::metacall(qobject, QMetaObject::InvokeMetaMethod, methodIndex, 0);

    return v8::Undefined();
}

// This callback implements a fallback property getter for Qt wrapper objects.
// This is only called if the property is not a QMetaObject-defined property,
// Q_INVOKABLE method or slot. It handles signals (which are methods that must
// be bound to an object), and dynamic properties and child objects (since they
// are instance-specific, not defined by the QMetaObject).
static v8::Handle<v8::Value> QtLazyPropertyGetter(v8::Local<v8::String> property,
                                                  const v8::AccessorInfo& info)
{
//    qDebug() << "QtLazyPropertyGetter(" << qtStringFromJS(property) << ")";
    v8::Local<v8::Object> self = info.This();
    QtInstanceData *data = QtInstanceData::get(self);
    QScriptEnginePrivate *engine = data->engine();
    QObject *qobject = data->cppObject();

    QByteArray name = QScriptConverter::toString(property).toLatin1();

    // Look up dynamic property.
    {
        int index = qobject->dynamicPropertyNames().indexOf(name);
        if (index != -1) {
            QVariant value = qobject->property(name);
            // Install accessor for this dynamic property.
            // Dynamic properties can be removed from the C++ object without
            // us knowing it (well, we could install an event filter, but
            // that seems overkill). The getter makes sure that the property
            // is still there the next time a read is attempted, and
            // uninstalls itself if not, so that we fall back to this
            // interceptor again.
            self->SetAccessor(property,
                              QtDynamicPropertyGetter,
                              QtDynamicPropertySetter);

            return engine->variantToJS(value);
        }
    }

    // Look up child.
    if (!(data->options() & QScriptEngine::ExcludeChildObjects)) {
        QList<QObject*> children = qobject->children();
        for (int index = 0; index < children.count(); ++index) {
            QObject *child = children.at(index);
            QString childName = child->objectName();
            if (childName == QString::fromLatin1(name)) {
                Q_UNIMPLEMENTED();
                return engine->newQObject(child);
            }
        }
    }

    // Look up signal by name or signature.
    bool hasParens = name.indexOf('(') != -1;
    const QMetaObject *meta = qobject->metaObject();
    for (int index = meta->methodCount() - 1; index >= 0; --index) {
        QMetaMethod method = meta->method(index);
        if (method.methodType() != QMetaMethod::Signal)
            continue;
        if (method.access() == QMetaMethod::Private)
            continue;
        if (!hasParens && methodNameEquals(method, name.constData(), name.length())) {
            v8::Handle<v8::Object> signal = engine->newSignal(self, index, QtSignalData::ResolvedByName);
            // Set the property directly on the instance, so from now on it won't be
            // handled by the interceptor.
            self->ForceSet(property, signal);
            return signal;
        } else if (hasParens && !qstrcmp(method.signature(), name.constData())) {
            v8::Handle<v8::Object> signal = engine->newSignal(self, index, QtSignalData::ResolvedBySignature);
            // Set the property directly on the instance, so from now on it won't be
            // handled by the interceptor.
            self->ForceSet(property, signal);
            return signal;
        }
    }

    return v8::Handle<v8::Value>();
}

static v8::Handle<v8::Value> QtLazyPropertySetter(v8::Local<v8::String> property,
                                                  v8::Local<v8::Value> value,
                                                  const v8::AccessorInfo& info)
{
#if 0
    v8::Local<v8::Object> self = info.This();
    QtInstanceData *data = QtInstanceData::get(self);
    QScriptEnginePrivate *engine = data->engine();
    QObject *qobject = data->cppObject();

    QByteArray name = engine->qtStringFromJS(property).toLatin1();
    qDebug() << qobject << name;
    Q_UNIMPLEMENTED();
#endif
    Q_UNUSED(property);
    Q_UNUSED(value);
    Q_UNUSED(info);
    return v8::Handle<v8::Value>();
}

// This callback implements a catch-all property getter for QMetaObject wrapper objects.
static v8::Handle<v8::Value> QtMetaObjectPropertyGetter(v8::Local<v8::String> property,
                                                        const v8::AccessorInfo& info)
{
    Q_UNUSED(property);
    Q_UNUSED(info);
    Q_UNIMPLEMENTED();
#if 0
    v8::Local<v8::Object> self = info.Holder();
    QtMetaObjectData *data = QtMetaObjectData::get(self);
    QScriptEnginePrivate *engine = data->engine();

    const QMetaObject *meta = data->metaObject();

#if 0
    if (propertyName == exec->propertyNames().prototype) {
        if (data->ctor)
            slot.setValue(data->ctor.get(exec, propertyName));
        else
            slot.setValue(data->prototype);
        return true;
    }
#endif

    QByteArray name = engine->qtStringFromJS(property).toLatin1();

    for (int i = 0; i < meta->enumeratorCount(); ++i) {
        QMetaEnum e = meta->enumerator(i);
        for (int j = 0; j < e.keyCount(); ++j) {
            const char *key = e.key(j);
            if (!qstrcmp(key, name.constData()))
                return v8::Int32::New(e.value(j));
        }
    }
#endif
    return v8::Handle<v8::Value>();
}

// This callback implements call-as-function for signal wrapper objects.
static v8::Handle<v8::Value> QtSignalCallback(const v8::Arguments& args)
{
    Q_UNUSED(args);
    Q_UNIMPLEMENTED();
    return v8::Handle<v8::Value>();
}

// This callback implements the connect() method of signal wrapper objects.
// The this-object is a QtSignal wrapper.
// If the connect succeeds, this function returns undefined; otherwise,
// an error is thrown.
// If the connect succeeds, the associated JS function will be invoked
// when the C++ object emits the signal.
static v8::Handle<v8::Value> QtConnectCallback(const v8::Arguments& args)
{
    v8::Local<v8::Object> self = args.Holder();
    QtSignalData *data = QtSignalData::get(self);

    if (args.Length() == 0)
        return v8::ThrowException(v8::Exception::SyntaxError(v8::String::New("QtSignal.connect(): no arguments given")));

    if (data->resolveMode() == QtSignalData::ResolvedByName) {
        // ### Check if the signal is overloaded. If it is, throw an error,
        // since it's not possible to know which of the overloads we should connect to.
        // Can probably figure this out at class/instance construction time
    }

    // ### Should be able to take any [[Callable]], but there is no v8 API for that.
    if (!args[0]->IsFunction())
        return v8::ThrowException(v8::Exception::TypeError(v8::String::New("QtSignal.connect(): argument is not a function")));

    v8::Handle<v8::Function> callback(v8::Function::Cast(*args[0]));
    Q_ASSERT(!callback.IsEmpty());

    // Options:
    // 1) Connection manager per C++ object.
    // 2) Connection manager per Qt wrapper object.
    // 3) Connection manager per signal wrapper object.
    // 4) Connection object per connection.

    // disconnect() needs to be able to go introspect connections
    // of that signal only, for that wrapper only.

    // qDebug() << "connect" << data->object() << data->index();
    return data->connect(callback);
}

// This callback implements the disconnect() method of signal wrapper objects.
// The this-object is a QtSignal wrapper.
// If the disconnect succeeds, this function returns undefined; otherwise,
// an error is thrown.
static v8::Handle<v8::Value> QtDisconnectCallback(const v8::Arguments& args)
{
    v8::Local<v8::Object> self = args.Holder();
    QtSignalData *data = QtSignalData::get(self);

    if (args.Length() == 0)
        return v8::ThrowException(v8::Exception::SyntaxError(v8::String::New("QtSignal.disconnect(): no arguments given")));

    // ### Should be able to take any [[Callable]], but there is no v8 API for that.
    if (!args[0]->IsFunction())
        return v8::ThrowException(v8::Exception::TypeError(v8::String::New("QtSignal.disconnect(): argument is not a function")));

    return data->disconnect(v8::Handle<v8::Function>(v8::Function::Cast(*args[0])));
}

// v8 calls this function when a QObject wrapper object (val) is
// garbage-collected.
static void QtInstanceWeakCallback(v8::Persistent<v8::Value> val, void *arg)
{
    QtInstanceData *data = static_cast<QtInstanceData*>(arg);
    delete data;

    v8::HandleScope scope;
    v8::Local<v8::Object> object = v8::Object::Cast(*val);
    Q_ASSERT(!object.IsEmpty());
    object->SetPointerInInternalField(0, 0);
    val.Dispose();
    val.Clear();
}

// ### Unused
#if 0
static void QtMetaObjectWeakCallback(v8::Persistent<v8::Value> val, void *arg)
{
    Q_UNUSED(val);
    QtMetaObjectData *data = static_cast<QtMetaObjectData*>(arg);
    delete data;
}
#endif

v8::Handle<v8::FunctionTemplate> createQtClassTemplate(QScriptEnginePrivate *engine, const QMetaObject *mo)
{
//    qDebug() << "Creating template for" << mo->className();

    v8::Handle<v8::FunctionTemplate> funcTempl = v8::FunctionTemplate::New();
    funcTempl->SetClassName(v8::String::New(mo->className()));

    // Make the template inherit the super class's template.
    // This is different from the old back-end, where every wrapped
    // object exposed everything as own properties.
    const QMetaObject *superMo = mo->superClass();
    if (superMo)
        funcTempl->Inherit(engine->qtClassTemplate(superMo));

    v8::Handle<v8::ObjectTemplate> instTempl = funcTempl->InstanceTemplate();
    v8::Handle<v8::ObjectTemplate> protoTempl = funcTempl->PrototypeTemplate();
    // Internal field is used to hold QtInstanceData*.
    instTempl->SetInternalFieldCount(1);

    // Add accessors for meta-properties.
    for (int i = mo->propertyOffset(); i < mo->propertyCount(); ++i) {
        QMetaProperty prop = mo->property(i);
        // Choose suitable callbacks for type.
        v8::AccessorGetter getter;
        v8::AccessorSetter setter;
#if 0
        if (prop.userType() == QMetaType::QString) {
            getter = QtMetaPropertyFastGetter<StringType>;
            setter = QtMetaPropertyFastSetter<StringType>;
        } else
#endif
        {
            getter = QtMetaPropertyGetter;
            setter = QtMetaPropertySetter;
        }
        instTempl->SetAccessor(v8::String::New(prop.name()),
                               getter, setter,
                               v8::Int32::New(i));
    }

    // Figure out method names (own and inherited).
    QHash<QByteArray, QList<int> > ownMethodNameToIndexes;
    QHash<QByteArray, QList<int> > methodNameToIndexes;
    int methodOffset = mo->methodOffset();
    for (int i = 0; i < mo->methodCount(); ++i) {
        QMetaMethod method = mo->method(i);
        // Ignore private methods.
        if (method.access() == QMetaMethod::Private)
            continue;

        // Ignore signals, they're handled dynamically.
        if (method.methodType() == QMetaMethod::Signal)
            continue;

        QByteArray signature = method.signature();
        QByteArray name = signature.left(signature.indexOf('('));
        if (i >= methodOffset)
            ownMethodNameToIndexes[name].append(i);
        methodNameToIndexes[name].append(i);
    }

    // Add accessors for own meta-methods.
    {
        QHash<QByteArray, QList<int> >::const_iterator it;
        v8::Handle<v8::Signature> sig = v8::Signature::New(funcTempl);
        for (it = ownMethodNameToIndexes.constBegin(); it != ownMethodNameToIndexes.constEnd(); ++it) {
            QByteArray name = it.key();
            QList<int> indexes = it.value();
            int methodIndex = indexes.last(); // The largest index by that name.
            bool overloaded = methodNameToIndexes[name].size() > 1;
            QMetaMethod method = mo->method(methodIndex);

            // Choose appropriate callback based on return type and parameter types.
            v8::InvocationCallback callback = 0;
            if (overloaded) {
                // Generic handler for methods.
                callback = QtOverloadedMetaMethodCallback;
            } else if (!qstrlen(method.typeName()) && method.parameterTypes().isEmpty())
                callback = QtVoidVoidMetaMethodCallback;

            if (!callback) {
                // Generic handler for non-overloaded methods.
                callback = QtMetaMethodCallback;
            }

            v8::Handle<v8::FunctionTemplate> methodTempl = v8::FunctionTemplate::New(callback, v8::Int32::New(methodIndex), sig);
            protoTempl->Set(v8::String::New(name), methodTempl);
            if (indexes.count() == 1) {
                // No overloads, so by-name and by-signature properties should be the same function object.
                protoTempl->Set(v8::String::New(method.signature()), methodTempl);
            } else {
                // TODO: add unique by-signature properties for overloads.
            }
        }
    }

    if (mo == &QObject::staticMetaObject) {
        // Install QObject interceptor.
        // This interceptor will only get called if the access is not handled by the instance
        // itself nor other objects in the prototype chain.
        // The interceptor handles access to signals, dynamic properties and child objects.
        // The reason for putting the interceptor near the "back" of the prototype chain
        // is to avoid "slow" interceptor calls in what should be the common cases (accessing
        // meta-object-defined properties, and Q_INVOKABLE methods and slots).
        protoTempl->SetNamedPropertyHandler(QtLazyPropertyGetter,
                                            QtLazyPropertySetter);

        // TODO: Add QObject prototype functions findChild(), findChildren() (compat)
    }

    return funcTempl;
}

v8::Handle<v8::FunctionTemplate> createQtSignalTemplate()
{
    v8::Handle<v8::FunctionTemplate> funcTempl = v8::FunctionTemplate::New();
    funcTempl->SetClassName(v8::String::New("QtSignal"));

    v8::Handle<v8::ObjectTemplate> instTempl = funcTempl->InstanceTemplate();
    instTempl->SetCallAsFunctionHandler(QtSignalCallback);
    instTempl->SetInternalFieldCount(1); // QtSignalData*

    v8::Handle<v8::ObjectTemplate> protoTempl = funcTempl->PrototypeTemplate();
    v8::Handle<v8::Signature> sig = v8::Signature::New(funcTempl);
    protoTempl->Set(v8::String::New("connect"), v8::FunctionTemplate::New(QtConnectCallback, v8::Handle<v8::Value>(), sig));
    protoTempl->Set(v8::String::New("disconnect"), v8::FunctionTemplate::New(QtDisconnectCallback, v8::Handle<v8::Value>(), sig));

    return funcTempl;
}

v8::Handle<v8::FunctionTemplate> createQtMetaObjectTemplate()
{
    v8::Handle<v8::FunctionTemplate> funcTempl = v8::FunctionTemplate::New();
    funcTempl->SetClassName(v8::String::New("QMetaObject"));

    v8::Handle<v8::ObjectTemplate> instTempl = funcTempl->InstanceTemplate();
    instTempl->SetInternalFieldCount(1); // QtMetaObjectData*

    instTempl->SetNamedPropertyHandler(QtMetaObjectPropertyGetter);

    return funcTempl;
}

v8::Handle<v8::Object> newQtObject(QScriptEnginePrivate *engine, QObject *object,
                                   QScriptEngine::ValueOwnership own,
                                   const QScriptEngine::QObjectWrapOptions &opt)
{
    v8::Context::Scope contextScope(*engine);
    v8::HandleScope handleScope;
    v8::Handle<v8::FunctionTemplate> templ = engine->qtClassTemplate(object->metaObject());
    Q_ASSERT(!templ.IsEmpty());
    v8::Handle<v8::ObjectTemplate> instanceTempl = templ->InstanceTemplate();
    Q_ASSERT(!instanceTempl.IsEmpty());
    v8::Handle<v8::Object> instance = instanceTempl->NewInstance();
    Q_ASSERT(instance->InternalFieldCount() == 1);
    QtInstanceData *data = new QtInstanceData(engine, object, own, opt);
    instance->SetPointerInInternalField(0, data);

    // Add accessors for current dynamic properties.
    {
        QList<QByteArray> dpNames = object->dynamicPropertyNames();
        for (int i = 0; i < dpNames.size(); ++i) {
            QByteArray name = dpNames.at(i);
            instance->SetAccessor(v8::String::New(name),
                                  QtDynamicPropertyGetter,
                                  QtDynamicPropertySetter);
        }
    }

    if (!opt & QScriptEngine::ExcludeChildObjects) {
        // Add accessors for current child objects.
        QList<QObject*> children = object->children();
        for (int i = 0; i < children.size(); ++i) {
            QObject *child = children.at(i);
            if (child->objectName().isEmpty())
                continue;
            // Install an accessor.
            Q_UNIMPLEMENTED();
        }
    }

    v8::Persistent<v8::Object> persistent = v8::Persistent<v8::Object>::New(instance);
    persistent.MakeWeak(data, QtInstanceWeakCallback);
    return persistent;
}

QT_END_NAMESPACE
