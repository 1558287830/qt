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

#ifndef QSCRIPTVALUE_P_H
#define QSCRIPTVALUE_P_H

#include <v8.h>

#include "qscriptconverter_p.h"
#include "qscriptengine_p.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmath.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

/*
  \internal
  \class QScriptValuePrivate
  TODO we need to preallocate some values
    // TODO: bring back these!
    //
    //    inline void* operator new(size_t, QScriptEnginePrivate*);
    //    inline void operator delete(void*);
*/
class QScriptValuePrivate : public QSharedData
{
public:

    inline static QScriptValuePrivate* get(const QScriptValue& q);
    inline static QScriptValue get(const QScriptValuePrivate* d);
    inline static QScriptValue get(QScriptValuePrivate* d);

    inline ~QScriptValuePrivate();

    inline QScriptValuePrivate();
    inline QScriptValuePrivate(bool value);
    inline QScriptValuePrivate(int value);
    inline QScriptValuePrivate(uint value);
    inline QScriptValuePrivate(qsreal value);
    inline QScriptValuePrivate(const QString& value);
    inline QScriptValuePrivate(QScriptValue::SpecialValue value);

    inline QScriptValuePrivate(QScriptEnginePrivate* engine, bool value);
    inline QScriptValuePrivate(QScriptEnginePrivate* engine, int value);
    inline QScriptValuePrivate(QScriptEnginePrivate* engine, uint value);
    inline QScriptValuePrivate(QScriptEnginePrivate* engine, qsreal value);
    inline QScriptValuePrivate(QScriptEnginePrivate* engine, const QString& value);
    inline QScriptValuePrivate(QScriptEnginePrivate* engine, QScriptValue::SpecialValue value);
    inline QScriptValuePrivate(QScriptEnginePrivate* engine, v8::Handle<v8::Value>);

    inline bool toBool() const;
    inline qsreal toNumber() const;
    inline QScriptValuePrivate* toObject();
    inline QScriptValuePrivate* toObject(QScriptEnginePrivate* engine);
    inline QString toString() const;
    inline qsreal toInteger() const;
    inline qint32 toInt32() const;
    inline quint32 toUInt32() const;
    inline quint16 toUInt16() const;
    inline QObject *toQObject() const;

    inline bool isArray() const;
    inline bool isBool() const;
    inline bool isError() const;
    inline bool isFunction() const;
    inline bool isNull() const;
    inline bool isNumber() const;
    inline bool isObject() const;
    inline bool isString() const;
    inline bool isUndefined() const;
    inline bool isVariant() const;
    inline bool isValid() const;
    inline bool isDate() const;
    inline bool isRegExp() const;
    inline bool isQObject() const;
    inline bool isQMetaObject() const;

    inline bool equals(QScriptValuePrivate* other);
    inline bool strictlyEquals(QScriptValuePrivate* other);
    inline bool instanceOf(QScriptValuePrivate*) const;
    inline bool instanceOf(v8::Handle<v8::Object> other) const;

    inline QScriptValuePrivate* prototype() const;
    inline void setPrototype(QScriptValuePrivate* prototype);

    inline void setProperty(const QString& name, QScriptValuePrivate* value, const QScriptValue::PropertyFlags& flags);
    inline void setProperty(quint32 index, QScriptValuePrivate* value, const QScriptValue::PropertyFlags& flags);
    inline QScriptValuePrivate* property(const QString& name, const QScriptValue::ResolveFlags& mode) const;
    inline QScriptValuePrivate* property(quint32 index, const QScriptValue::ResolveFlags& mode) const;
    inline bool deleteProperty(const QString& name);
    inline QScriptValue::PropertyFlags propertyFlags(const QString& name, const QScriptValue::ResolveFlags& mode);

    inline QScriptValuePrivate* call(const QScriptValuePrivate* thisObject, const QScriptValueList& args);
    inline QScriptValuePrivate* call(const QScriptValuePrivate* thisObject, const QScriptValue& arguments);
    inline QScriptValuePrivate* construct(const QScriptValueList& args);
    inline QScriptValuePrivate* construct(const QScriptValue& arguments);

    inline bool assignEngine(QScriptEnginePrivate* engine);
    inline QScriptEnginePrivate* engine() const;

    inline operator v8::Persistent<v8::Value>() const;

    QExplicitlySharedDataPointer<QScriptEnginePrivate> m_engine;

    // Please, update class documentation when you change the enum.
    enum State {
        Invalid = 0,
        CString = 0x1000,
        CNumber,
        CBool,
        CNull,
        CUndefined,
        JSValue = 0x2000, // V8 values are equal or higher then this value.
        // JSPrimitive,
        // JSObject
    } m_state;

    union CValue {
        bool m_bool;
        qsreal m_number;
        QString* m_string;

        CValue() : m_number(0) {}
        CValue(bool value) : m_bool(value) {}
        CValue(int number) : m_number(number) {}
        CValue(uint number) : m_number(number) {}
        CValue(qsreal number) : m_number(number) {}
        CValue(QString* string) : m_string(string) {}
    } u;

    // v8::Persistent is not a POD, so can't be part of the union.
    v8::Persistent<v8::Value> m_value;

    inline bool isJSBased() const;
    inline bool isNumberBased() const;
    inline bool isStringBased() const;
    inline bool prepareArgumentsForCall(v8::Handle<v8::Value> argv[], const QScriptValueList& arguments) const;
};

QScriptValuePrivate* QScriptValuePrivate::get(const QScriptValue& q) { return q.d_ptr.data(); }

QScriptValue QScriptValuePrivate::get(const QScriptValuePrivate* d)
{
    return QScriptValue(const_cast<QScriptValuePrivate*>(d));
}

QScriptValue QScriptValuePrivate::get(QScriptValuePrivate* d)
{
    Q_ASSERT(d);
    return QScriptValue(d);
}

QScriptValuePrivate::QScriptValuePrivate()
    : m_state(Invalid)
{
}

// ### Until "Isolates" are supported in V8, we don't have a way to
// create values for a specific engine, because in practice only one
// engine is allowed to exist.

QScriptValuePrivate::QScriptValuePrivate(bool value)
    : m_state(CBool), u(value)
{
}

QScriptValuePrivate::QScriptValuePrivate(int value)
    : m_state(CNumber), u(value)
{
}

QScriptValuePrivate::QScriptValuePrivate(uint value)
    : m_state(CNumber), u(value)
{
}

QScriptValuePrivate::QScriptValuePrivate(qsreal value)
    : m_state(CNumber), u(value)
{
}

QScriptValuePrivate::QScriptValuePrivate(const QString& value)
    : m_state(CString), u(new QString(value))
{
}

QScriptValuePrivate::QScriptValuePrivate(QScriptValue::SpecialValue value)
    : m_state(value == QScriptValue::NullValue ? CNull : CUndefined)
{
}

QScriptValuePrivate::QScriptValuePrivate(QScriptEnginePrivate* engine, bool value)
    : m_engine(engine), m_state(JSValue)
{
    Q_ASSERT(engine);
    v8::Context::Scope contextScope(*engine);
    v8::HandleScope handleScope;
    m_value = m_engine->makeJSValue(value);
}

QScriptValuePrivate::QScriptValuePrivate(QScriptEnginePrivate* engine, int value)
    : m_engine(engine), m_state(JSValue)
{
    Q_ASSERT(engine);
    v8::Context::Scope contextScope(*engine);
    v8::HandleScope handleScope;
    m_value = m_engine->makeJSValue(value);
}

QScriptValuePrivate::QScriptValuePrivate(QScriptEnginePrivate* engine, uint value)
    : m_engine(engine), m_state(JSValue)
{
    Q_ASSERT(engine);
    v8::Context::Scope contextScope(*engine);
    v8::HandleScope handleScope;
    m_value = m_engine->makeJSValue(value);
}

QScriptValuePrivate::QScriptValuePrivate(QScriptEnginePrivate* engine, qsreal value)
    : m_engine(engine), m_state(JSValue)
{
    Q_ASSERT(engine);
    v8::Context::Scope contextScope(*engine);
    v8::HandleScope handleScope;
    m_value = m_engine->makeJSValue(value);
}

QScriptValuePrivate::QScriptValuePrivate(QScriptEnginePrivate* engine, const QString& value)
    : m_engine(engine), m_state(JSValue)
{
    Q_ASSERT(engine);
    v8::Context::Scope contextScope(*engine);
    v8::HandleScope handleScope;
    m_value = m_engine->makeJSValue(value);
}

QScriptValuePrivate::QScriptValuePrivate(QScriptEnginePrivate* engine, QScriptValue::SpecialValue value)
    : m_engine(engine), m_state(JSValue)
{
    Q_ASSERT(engine);
    v8::Context::Scope contextScope(*engine);
    v8::HandleScope handleScope;
    m_value = m_engine->makeJSValue(value);
}

QScriptValuePrivate::QScriptValuePrivate(QScriptEnginePrivate *engine, v8::Handle<v8::Value> value)
    : m_engine(engine), m_state(JSValue)
{
    Q_ASSERT(engine);
    // this can happen if a method in V8 returns an undefined value
    if (value.IsEmpty())
        m_state = Invalid;
    else
        m_value = v8::Persistent<v8::Value>::New(value);
}

QScriptValuePrivate::~QScriptValuePrivate()
{
    if (isJSBased()) {
        m_value.Dispose();
        m_value.Clear();
    } else if (isStringBased()) {
        delete u.m_string;
    }
}

bool QScriptValuePrivate::toBool() const
{
    switch (m_state) {
    case JSValue:
        {
            v8::HandleScope scope;
            return m_value->ToBoolean()->Value();
        }
    case CNumber:
        return !(qIsNaN(u.m_number) || !u.m_number);
    case CBool:
        return u.m_bool;
    case Invalid:
    case CNull:
    case CUndefined:
        return false;
    case CString:
        return u.m_string->length();
    }

    Q_ASSERT_X(false, "toBool()", "Not all states are included in the previous switch statement.");
    return false; // Avoid compiler warning.
}

qsreal QScriptValuePrivate::toNumber() const
{
    switch (m_state) {
    case JSValue:
    {
        v8::Context::Scope contextScope(*engine());
        v8::HandleScope scope;
        return m_value->ToNumber()->Value();
    }
    case CNumber:
        return u.m_number;
    case CBool:
        return u.m_bool ? 1 : 0;
    case CNull:
    case Invalid:
        return 0;
    case CUndefined:
        return qQNaN();
    case CString:
        bool ok;
        qsreal result = u.m_string->toDouble(&ok);
        if (ok)
            return result;
        result = u.m_string->toInt(&ok, 0); // Try other bases.
        if (ok)
            return result;
        if (*u.m_string == QLatin1String("Infinity") || *u.m_string == QLatin1String("-Infinity"))
            return qInf();
        return u.m_string->length() ? qQNaN() : 0;
    }

    Q_ASSERT_X(false, "toNumber()", "Not all states are included in the previous switch statement.");
    return 0; // Avoid compiler warning.
}

QScriptValuePrivate* QScriptValuePrivate::toObject(QScriptEnginePrivate* engine)
{
    Q_ASSERT(engine);
    if (this->engine() && engine != this->engine()) {
        qWarning("QScriptEngine::toObject: cannot convert value created in a different engine");
        return new QScriptValuePrivate();
    }

    v8::Context::Scope contextScope(*engine);
    v8::HandleScope scope;
    switch (m_state) {
    case Invalid:
    case CNull:
    case CUndefined:
        return new QScriptValuePrivate;
    case CString:
        return new QScriptValuePrivate(engine, engine->makeJSValue(*u.m_string)->ToObject());
    case CNumber:
        return new QScriptValuePrivate(engine, engine->makeJSValue(u.m_number)->ToObject());
    case CBool:
        return new QScriptValuePrivate(engine, engine->makeJSValue(u.m_bool)->ToObject());
    case JSValue:
        if (m_value->IsObject())
            return this;
        if (m_value->IsNull() || m_value->IsUndefined()) // avoid "Uncaught TypeError: Cannot convert null to object"
            return new QScriptValuePrivate();
        return new QScriptValuePrivate(engine, m_value->ToObject());
    default:
        Q_ASSERT_X(false, Q_FUNC_INFO, "Not all states are included in this switch");
        return new QScriptValuePrivate();
    }
}

/*!
  This method is created only for QScriptValue::toObject() purpose which is obsolete.
  \internal
 */
QScriptValuePrivate* QScriptValuePrivate::toObject()
{
    if (isJSBased())
        return toObject(engine());

    // Without an engine there is not much we can do.
    return new QScriptValuePrivate;
}

QString QScriptValuePrivate::toString() const
{
    switch (m_state) {
    case Invalid:
        return QString();
    case CBool:
        return u.m_bool ? QString::fromLatin1("true") : QString::fromLatin1("false");
    case CString:
        return *u.m_string;
    case CNumber:
        return QScriptConverter::toString(u.m_number);
    case CNull:
        return QString::fromLatin1("null");
    case CUndefined:
        return QString::fromLatin1("undefined");
    case JSValue:
        Q_ASSERT(!m_value.IsEmpty());
        v8::Context::Scope contextScope(*engine());
        v8::HandleScope handleScope;
        return QScriptConverter::toString(m_value->ToString());
    }

    Q_ASSERT_X(false, "toString()", "Not all states are included in the previous switch statement.");
    return QString(); // Avoid compiler warning.
}

QObject* QScriptValuePrivate::toQObject() const
{
    if (!isObject())
        return 0;
    return toQtObject(m_engine.data(), m_value->ToObject());
}

qsreal QScriptValuePrivate::toInteger() const
{
    qsreal result = toNumber();
    if (qIsNaN(result))
        return 0;
    if (qIsInf(result))
        return result;
    return (result > 0) ? qFloor(result) : -1 * qFloor(-result);
}

qint32 QScriptValuePrivate::toInt32() const
{
    qsreal result = toInteger();
    // Orginaly it should look like that (result == 0 || qIsInf(result) || qIsNaN(result)), but
    // some of these operation are invoked in toInteger subcall.
    if (qIsInf(result))
        return 0;
    return result;
}

quint32 QScriptValuePrivate::toUInt32() const
{
    qsreal result = toInteger();
    // Orginaly it should look like that (result == 0 || qIsInf(result) || qIsNaN(result)), but
    // some of these operation are invoked in toInteger subcall.
    if (qIsInf(result))
        return 0;
    return result;
}

quint16 QScriptValuePrivate::toUInt16() const
{
    return toInt32();
}

inline bool QScriptValuePrivate::isArray() const
{
    return isJSBased() && m_value->IsArray();
}

inline bool QScriptValuePrivate::isBool() const
{
    return m_state == CBool || (isJSBased() && m_value->IsBoolean());
}

inline bool QScriptValuePrivate::isError() const
{
    if (!isJSBased())
        return false;
    return m_value->IsObject() && engine()->isError(this);
}

inline bool QScriptValuePrivate::isFunction() const
{
    return isJSBased() && m_value->IsFunction();
}

inline bool QScriptValuePrivate::isNull() const
{
    return m_state == CNull || (isJSBased() && m_value->IsNull());
}

inline bool QScriptValuePrivate::isNumber() const
{
    return m_state == CNumber || (isJSBased() && m_value->IsNumber());
}

inline bool QScriptValuePrivate::isObject() const
{
    return isJSBased() && m_value->IsObject();
}

inline bool QScriptValuePrivate::isString() const
{
    return m_state == CString || (isJSBased() && m_value->IsString());
}

inline bool QScriptValuePrivate::isUndefined() const
{
    return m_state == CUndefined || (isJSBased() && m_value->IsUndefined());
}

inline bool QScriptValuePrivate::isValid() const
{
    return m_state != Invalid;
}

inline bool QScriptValuePrivate::isVariant() const
{
    bool result = isJSBased();
    if (result) {
        Q_UNIMPLEMENTED();
        return false;
    }
    return false;
}

bool QScriptValuePrivate::isDate() const
{
    bool result = isJSBased();
    if (result) {
        Q_UNIMPLEMENTED();
        return false;
    }
    return false;
}

bool QScriptValuePrivate::isRegExp() const
{
    bool result = isJSBased();
    if (result) {
        Q_UNIMPLEMENTED();
        return false;
    }
    return false;
}

bool QScriptValuePrivate::isQObject() const
{
    if (!isJSBased() || !m_value->IsObject())
        return false;
    return toQtObject(m_engine.data(), m_value->ToObject());
}

bool QScriptValuePrivate::isQMetaObject() const
{
    bool result = isJSBased();
    if (result) {
        Q_UNIMPLEMENTED();
        return false;
    }
    return false;
}

inline bool QScriptValuePrivate::equals(QScriptValuePrivate* other)
{
    if (!isValid())
        return !other->isValid();

    if (!other->isValid())
        return false;

    if (!isJSBased() && !other->isJSBased()) {
        switch (m_state) {
        case CNull:
        case CUndefined:
            return other->isUndefined() || other->isNull();
        case CNumber:
            switch (other->m_state) {
            case CBool:
            case CString:
                return u.m_number == other->toNumber();
            case CNumber:
                return u.m_number == other->u.m_number;
            default:
                return false;
            }
        case CBool:
            switch (other->m_state) {
            case CBool:
                return u.m_bool == other->u.m_bool;
            case CNumber:
                return toNumber() == other->u.m_number;
            case CString:
                return toNumber() == other->toNumber();
            default:
                return false;
            }
        case CString:
            switch (other->m_state) {
            case CBool:
                return toNumber() == other->toNumber();
            case CNumber:
                return toNumber() == other->u.m_number;
            case CString:
                return *u.m_string == *other->u.m_string;
            default:
                return false;
            }
        default:
            Q_ASSERT_X(false, "equals()", "Not all states are included in the previous switch statement.");
        }
    }

    v8::HandleScope handleScope;
    if (isJSBased() && !other->isJSBased()) {
        if (!other->assignEngine(engine())) {
            qWarning("equals(): Cannot compare to a value created in a different engine");
            return false;
        }
    } else if (!isJSBased() && other->isJSBased()) {
        if (!assignEngine(other->engine())) {
            qWarning("equals(): Cannot compare to a value created in a different engine");
            return false;
        }
    }

    Q_ASSERT(this->engine() && other->engine());
    v8::Context::Scope contextScope(*engine());
    v8::HandleScope scope;
    return m_value->Equals(other->m_value);
    return false;
}

inline bool QScriptValuePrivate::strictlyEquals(QScriptValuePrivate* other)
{
    if (isJSBased()) {
        // We can't compare these two values without binding to the same engine.
        if (!other->isJSBased()) {
            if (other->assignEngine(engine()))
                return m_value->StrictEquals(other->m_value);
            return false;
        }
        if (other->engine() != engine()) {
            qWarning("strictlyEquals(): Cannot compare to a value created in a different engine");
            return false;
        }
        return m_value->StrictEquals(other->m_value);
    }
    if (isStringBased()) {
        if (other->isStringBased())
            return *u.m_string == *(other->u.m_string);
        if (other->isJSBased()) {
            assignEngine(other->engine());
            return m_value->StrictEquals(other->m_value);
        }
    }
    if (isNumberBased()) {
        if (other->isNumberBased())
            return u.m_number == other->u.m_number;
        if (other->isJSBased()) {
            assignEngine(other->engine());
            return m_value->StrictEquals(other->m_value);
        }
    }

    if (!isValid() && !other->isValid())
        return true;

    return false;
}

inline bool QScriptValuePrivate::instanceOf(QScriptValuePrivate* other) const
{
    if (!isObject() || !other->isFunction())
        return false;
    v8::Context::Scope contextScope(*engine());
    v8::HandleScope handleScope;
    return instanceOf(v8::Handle<v8::Object>::Cast(other->m_value));
}

inline bool QScriptValuePrivate::instanceOf(v8::Handle<v8::Object> other) const
{
    Q_ASSERT(isObject());
    Q_ASSERT(other->IsFunction());

    v8::Handle<v8::Object> self = v8::Handle<v8::Object>::Cast(m_value);
    v8::Handle<v8::Value> selfPrototype = self->GetPrototype();
    v8::Handle<v8::Value> otherPrototype = other->Get(v8::String::New("prototype"));

    while (!selfPrototype->IsNull()) {
        if (selfPrototype->StrictEquals(otherPrototype))
            return true;
        // In general a prototype can be an object or null, but in the loop it can't be null, so
        // we can cast it safely.
        selfPrototype = v8::Handle<v8::Object>::Cast(selfPrototype)->GetPrototype();
    }
    return false;
}

inline QScriptValuePrivate* QScriptValuePrivate::prototype() const
{
    if (isJSBased() && m_value->IsObject()) {
        v8::Context::Scope contextScope(*engine());
        v8::HandleScope handleScope;
        return new QScriptValuePrivate(engine(), v8::Handle<v8::Object>::Cast(m_value)->GetPrototype());
    }
    return new QScriptValuePrivate();
}

inline void QScriptValuePrivate::setPrototype(QScriptValuePrivate* prototype)
{
    if (isObject() && prototype->isValid() && (prototype->isObject() || prototype->isNull())) {
        if (engine() != prototype->engine()) {
            if (prototype->engine()) {
                qWarning("QScriptValue::setPrototype() failed: cannot set a prototype created in a different engine");
                return;
            }
            prototype->assignEngine(engine());
        }
        v8::Context::Scope contextScope(*engine());
        v8::HandleScope handleScope;
        if (!v8::Handle<v8::Object>::Cast(m_value)->SetPrototype(prototype->m_value))
            qWarning("QScriptValue::setPrototype() failed: cyclic prototype value");
    }
}

inline void QScriptValuePrivate::setProperty(const QString& name, QScriptValuePrivate* value, const QScriptValue::PropertyFlags& flags)
{
    Q_UNUSED(flags); // FIXME it should be used.
    if (!isObject())
        return;

    if (!value->isJSBased())
        value->assignEngine(engine());

    if (!value->isValid()) {
        // Remove the property.
        v8::Context::Scope contextScope(*engine());
        v8::HandleScope handleScope;
        v8::Object::Cast(*m_value)->Delete(QScriptConverter::toString(name));
        return;
    }

    if (engine() != value->engine()) {
        qWarning("QScriptValue::setProperty() failed: cannot set value created in a different engine");
        return;
    }

    v8::Context::Scope contextScope(*engine());
    v8::HandleScope handleScope;
    v8::Object::Cast(*m_value)->Set(QScriptConverter::toString(name), value->m_value);
}

inline void QScriptValuePrivate::setProperty(quint32 index, QScriptValuePrivate* value, const QScriptValue::PropertyFlags& flags)
{
    Q_UNUSED(flags); // FIXME it should be used.
    if (!isObject())
        return;

    if (!isArray()) {
        setProperty(QString::number(index), value, flags);
        return;
    }

    if (!value->isJSBased())
        value->assignEngine(engine());

    if (engine() != value->engine()) {
        qWarning("QScriptValue::setProperty() failed: cannot set value created in a different engine");
        return;
    }

    v8::Context::Scope contextScope(*engine());
    v8::HandleScope handleScope;
    v8::Array::Cast(*m_value)->Set(index, value->m_value);
}

inline QScriptValuePrivate* QScriptValuePrivate::property(const QString& name, const QScriptValue::ResolveFlags& mode) const
{
    Q_UNUSED(mode);
    if (!isObject())
        return new QScriptValuePrivate();

    v8::Context::Scope contextScope(*engine());
    v8::HandleScope handleScope;
    v8::Handle<v8::Object> self(v8::Object::Cast(*m_value));
    v8::Handle<v8::String> jsname = QScriptConverter::toString(name);

    if ( (mode != QScriptValue::ResolveLocal && !self->Has(jsname))
        || (mode == QScriptValue::ResolveLocal && !self->HasRealNamedProperty(jsname)))
        return new QScriptValuePrivate();

    v8::Handle<v8::Value> result = self->Get(jsname);
    return new QScriptValuePrivate(engine(), result);
}

inline QScriptValuePrivate* QScriptValuePrivate::property(quint32 index, const QScriptValue::ResolveFlags& mode) const
{
    Q_UNUSED(mode);
    if (!isObject())
        return new QScriptValuePrivate();

    if (!isArray())
        return property(QString::number(index), mode);

    v8::Context::Scope contextScope(*engine());
    v8::HandleScope handleScope;
    v8::Handle<v8::Object> self(v8::Object::Cast(*m_value));
    v8::Handle<v8::Value> result = self->Get(index);
    return new QScriptValuePrivate(engine(), result);
}

inline bool QScriptValuePrivate::deleteProperty(const QString& name)
{
    if (!isObject())
        return false;

    v8::Context::Scope contextScope(*engine());
    v8::HandleScope handleScope;
    v8::Handle<v8::Object> self(v8::Handle<v8::Object>::Cast(m_value));
    return self->Delete(QScriptConverter::toString(name));
}

inline QScriptValue::PropertyFlags QScriptValuePrivate::propertyFlags(const QString& name, const QScriptValue::ResolveFlags& mode)
{
    Q_UNUSED(name); // FIXME it should be used.
    Q_UNUSED(mode); // FIXME it should be used.
    if (!isObject())
        return QScriptValue::PropertyFlags(0);
    Q_UNIMPLEMENTED();
    return QScriptValue::PropertyFlags(0);
}

inline QScriptValuePrivate* QScriptValuePrivate::call(const QScriptValuePrivate* thisObject, const QScriptValueList& args)
{
    if (!isFunction())
        return new QScriptValuePrivate();

    v8::Context::Scope contextScope(*engine());
    v8::HandleScope handleScope;

    // Convert all arguments and bind to the engine.
    int argc = args.size();
    QVarLengthArray<v8::Handle<v8::Value>, 8> argv(argc);
    if (!prepareArgumentsForCall(argv.data(), args)) {
        qWarning("QScriptValue::call() failed: cannot call function with values created in a different engine");
        return new QScriptValuePrivate();
    }

    // Make the call
    if (!thisObject || !thisObject->isObject())
        thisObject = m_engine->globalObject();
    v8::Handle<v8::Object> recv(v8::Object::Cast(*thisObject->m_value));
    v8::Handle<v8::Value> result = v8::Function::Cast(*m_value)->Call(recv, argc, argv.data());
    return new QScriptValuePrivate(engine(), result);
}

inline QScriptValuePrivate* QScriptValuePrivate::call(const QScriptValuePrivate* thisObject, const QScriptValue& arguments)
{
    if (!isFunction())
        return new QScriptValuePrivate();

    v8::Context::Scope contextScope(*engine());
    v8::HandleScope handleScope;

    if (!thisObject || !thisObject->isObject())
        thisObject = m_engine->globalObject();
    v8::Handle<v8::Object> recv(v8::Object::Cast(*thisObject->m_value));

    // Convert all arguments and bind to the engine.
    QScriptValuePrivate *args = QScriptValuePrivate::get(arguments);

    if (!args->isJSBased() && !args->assignEngine(engine()))
        return new QScriptValuePrivate();

    v8::Handle<v8::Value> result;
    if (args->isNull() || args->isUndefined()) {
        result = v8::Function::Cast(*m_value)->Call(recv, 0, 0);
    } else if (args->isArray()) {
        v8::Handle<v8::Array> array(v8::Array::Cast(*args->m_value));
        int argc = array->Length();
        QVarLengthArray<v8::Handle<v8::Value>, 8> argv(argc);
        for (int i = 0; i < argc; ++i)
            argv[i] = array->Get(i);
        result = v8::Function::Cast(*m_value)->Call(recv, argc, argv.data());
    } else {
        v8::ThrowException(v8::Exception::TypeError(v8::String::New("Arguments must be an array")));
        return new QScriptValuePrivate();
    }

    return new QScriptValuePrivate(engine(), result);
}


inline QScriptValuePrivate* QScriptValuePrivate::construct(const QScriptValueList& args)
{
    if (!isFunction())
        return new QScriptValuePrivate();

    // Convert all arguments and bind to the engine.
    int argc = args.size();
    QVarLengthArray<v8::Handle<v8::Value>, 8> argv(argc);
    if (!prepareArgumentsForCall(argv.data(), args)) {
        qWarning("QScriptValue::construct() failed: cannot call function with values created in a different engine");
        return new QScriptValuePrivate();
    }

    v8::Context::Scope contextScope(*engine());
    v8::HandleScope handleScope;
    v8::Handle<v8::Object> result;
    result = v8::Handle<v8::Function>::Cast(m_value)->NewInstance(argc, argv.data());
    Q_ASSERT(!result.IsEmpty());
    Q_ASSERT(result->IsObject());
    return new QScriptValuePrivate(engine(), result);
}

inline QScriptValuePrivate* QScriptValuePrivate::construct(const QScriptValue& arguments)
{
    if (!isFunction())
        return new QScriptValuePrivate();

    v8::Context::Scope contextScope(*engine());
    v8::HandleScope handleScope;

    // Convert all arguments and bind to the engine.
    QScriptValuePrivate *args = QScriptValuePrivate::get(arguments);

    if (!args->isJSBased() && !args->assignEngine(engine()))
        return new QScriptValuePrivate();

    v8::Handle<v8::Value> result;
    if (args->isNull() || args->isUndefined()) {
        result = v8::Function::Cast(*m_value)->NewInstance(0, 0);
    } else if (args->isArray()) {
        v8::Handle<v8::Array> array(v8::Array::Cast(*args->m_value));
        int argc = array->Length();
        QVarLengthArray<v8::Handle<v8::Value>, 8> argv(argc);
        for (int i = 0; i < argc; ++i)
            argv[i] = array->Get(i);
        result = v8::Function::Cast(*m_value)->NewInstance(argc, argv.data());
    } else {
        v8::ThrowException(v8::Exception::TypeError(v8::String::New("Arguments must be an array")));
        return new QScriptValuePrivate();
    }

    return new QScriptValuePrivate(engine(), result);
}


bool QScriptValuePrivate::assignEngine(QScriptEnginePrivate* engine)
{
    Q_ASSERT(engine);
    v8::HandleScope handleScope;
    switch (m_state) {
    case Invalid:
        return false;
    case CBool:
        m_value = engine->makeJSValue(u.m_bool);
        break;
    case CString:
        m_value = engine->makeJSValue(*u.m_string);
        delete u.m_string;
        break;
    case CNumber:
        m_value = engine->makeJSValue(u.m_number);
        break;
    case CNull:
        m_value = engine->makeJSValue(QScriptValue::NullValue);
        break;
    case CUndefined:
        m_value = engine->makeJSValue(QScriptValue::UndefinedValue);
        break;
    default:
        if (this->engine() == engine)
            return true;
        else if (!isJSBased())
            Q_ASSERT_X(!isJSBased(), "assignEngine()", "Not all states are included in the previous switch statement.");
        else
            qWarning("JSValue can't be rassigned to an another engine.");
        return false;
    }
    m_engine = engine;
    m_state = JSValue;
    return true;
}

QScriptEnginePrivate* QScriptValuePrivate::engine() const
{
    // As long as m_engine is an autoinitializated pointer we can safely return it without
    // checking current state.
    return const_cast<QScriptEnginePrivate*>(m_engine.constData());
}

inline QScriptValuePrivate::operator v8::Persistent<v8::Value>() const
{
    return m_value;
}

/*!
  \internal
  Returns true if QSV have an engine associated.
*/
bool QScriptValuePrivate::isJSBased() const { return m_state >= JSValue; }

/*!
  \internal
  Returns true if current value of QSV is placed in m_number.
*/
bool QScriptValuePrivate::isNumberBased() const { return m_state == CNumber || m_state == CBool; }

/*!
  \internal
  Returns true if current value of QSV is placed in m_string.
*/
bool QScriptValuePrivate::isStringBased() const { return m_state == CString; }

/*!
  \internal
  Converts arguments and bind them to the engine.
  \attention argv should be big enough
*/
inline bool QScriptValuePrivate::prepareArgumentsForCall(v8::Handle<v8::Value> argv[], const QScriptValueList& args) const
{
    QScriptValueList::const_iterator i = args.constBegin();
    for (int j = 0; i != args.constEnd(); j++, i++) {
        QScriptValuePrivate* value = QScriptValuePrivate::get(*i);
        if (!value->isJSBased() && !value->assignEngine(engine()))
            return false;
        argv[j] = *value;
    }
    return true;
}

QT_END_NAMESPACE

#endif
