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

#ifndef QSCRIPTENGINE_P_H
#define QSCRIPTENGINE_P_H

#include <QtCore/qhash.h>
#include <QtCore/qvariant.h>

#include <private/qobject_p.h>

#include "qscriptconverter_p.h"
#include "qscriptshareddata_p.h"
#include "qscriptqobject_p.h"
#include "qscriptoriginalglobalobject_p.h"
#include "qscriptvalue.h"
#include "qscriptprogram_p.h"
#include <v8.h>

QT_BEGIN_NAMESPACE

class QDateTime;
class QScriptEngine;
class QScriptContextPrivate;
class QScriptClassPrivate;

class QScriptEnginePrivate
    : public QScriptSharedData
{
    Q_DECLARE_PUBLIC(QScriptEngine)
public:
    static QScriptEnginePrivate* get(QScriptEngine* q) { Q_ASSERT(q); return q->d_func(); }
    static QScriptEngine* get(QScriptEnginePrivate* d) { Q_ASSERT(d); return d->q_func(); }

    QScriptEnginePrivate(QScriptEngine*, QScriptEngine::ContextOwnership ownership = QScriptEngine::CreateNewContext);
    ~QScriptEnginePrivate();

    inline QScriptValuePrivate* evaluate(const QString& program, const QString& fileName, int lineNumber);
    inline QScriptValuePrivate* evaluate(QScriptProgramPrivate* program);
    QScriptValuePrivate* evaluate(v8::Handle<v8::Script> script, v8::TryCatch& tryCatch);
    inline void collectGarbage();
    inline void reportAdditionalMemoryCost(int cost);
    QScriptValuePrivate* globalObject() const;
    void setGlobalObject(QScriptValuePrivate* newGlobalObjectValue);

    QScriptValuePrivate* newArray(uint length);
    QScriptValuePrivate* newObject();
    QScriptValuePrivate* newScriptClassObject(QScriptClassPrivate* scriptclass, QScriptValuePrivate* previousValue = 0);
    QScriptValuePrivate* newObject(QScriptClassPrivate* scriptclass, QScriptValuePrivate* data);
    QScriptValuePrivate *newFunction(QScriptEngine::FunctionSignature fun, QScriptValuePrivate *prototype, int length);
    QScriptValuePrivate *newFunction(QScriptEngine::FunctionWithArgSignature fun, void *arg);

    v8::Handle<v8::Object> newQObject(
        QObject *object, QScriptEngine::ValueOwnership own = QScriptEngine::QtOwnership,
        const QScriptEngine::QObjectWrapOptions &opt = 0);
    v8::Handle<v8::Object> newSignal(v8::Handle<v8::Object>, int index, QtSignalData::ResolveMode);

    v8::Handle<v8::Object> newVariant(const QVariant &value);
    v8::Handle<v8::Object> newQMetaObject(const QMetaObject* mo, const QScriptValue &ctor);

    v8::Handle<v8::FunctionTemplate> createSignalTemplate();
    v8::Handle<v8::FunctionTemplate> createMetaObjectTemplate();
    v8::Handle<v8::FunctionTemplate> createVariantTemplate();

    v8::Handle<v8::FunctionTemplate> qtClassTemplate(const QMetaObject *);
    v8::Handle<v8::FunctionTemplate> qobjectTemplate();

    inline v8::Handle<v8::Value> makeJSValue(bool value);
    inline v8::Handle<v8::Value> makeJSValue(int value);
    inline v8::Handle<v8::Value> makeJSValue(uint value);
    inline v8::Handle<v8::Value> makeJSValue(qsreal value);
    inline v8::Handle<v8::Value> makeJSValue(QScriptValue::SpecialValue value);
    inline v8::Handle<v8::Value> makeJSValue(const QString& value);
    inline bool isError(const QScriptValuePrivate* value) const;
    inline QScriptValue::PropertyFlags getPropertyFlags(v8::Handle<v8::Object> object, v8::Handle<v8::String> property, const QScriptValue::ResolveFlags& mode);

    QDateTime qtDateTimeFromJS(v8::Handle<v8::Date> jsDate);
    v8::Handle<v8::Value> qtDateTimeToJS(const QDateTime &dt);

#ifndef QT_NO_REGEXP
    QRegExp qtRegExpFromJS(v8::Handle<v8::Object> jsRegExp);
    v8::Handle<v8::Object> qtRegExpToJS(const QRegExp &re);
    v8::Handle<v8::Object> qtRegExpToJS(const QString &pattern, const QString &flags);
#endif

    v8::Handle<v8::Array> stringListToJS(const QStringList &lst);
    QStringList stringListFromJS(v8::Handle<v8::Array> jsArray);

    v8::Handle<v8::Array> variantListToJS(const QVariantList &lst);
    QVariantList variantListFromJS(v8::Handle<v8::Array> jsArray);

    v8::Handle<v8::Object> variantMapToJS(const QVariantMap &vmap);
    QVariantMap variantMapFromJS(v8::Handle<v8::Object> jsObject);

    v8::Handle<v8::Value> variantToJS(const QVariant &value);
    QVariant variantFromJS(v8::Handle<v8::Value> value);

    v8::Handle<v8::Value> metaTypeToJS(int type, const void *data);
    bool metaTypeFromJS(v8::Handle<v8::Value> value, int type, void *data);

    bool isQtObject(v8::Handle<v8::Value> value);
    QObject *qtObjectFromJS(v8::Handle<v8::Value> value);
    bool convertToNativeQObject(v8::Handle<v8::Value> value,
                                const QByteArray &targetType,
                                void **result);

    inline bool isQtVariant(v8::Handle<v8::Value> value) const;
    inline bool isQtSignal(v8::Handle<v8::Value> value) const;
    inline bool isQtMetaObject(v8::Handle<v8::Value> value) const;
    QVariant &variantValue(v8::Handle<v8::Value> value);

    void installTranslatorFunctions(QScriptValuePrivate* object);
    void installTranslatorFunctions(v8::Handle<v8::Value> object);

    QScriptValue scriptValueFromInternal(v8::Handle<v8::Value>);
    v8::Handle<v8::Value> scriptValueToInternal(const QScriptValue &);

    inline operator v8::Persistent<v8::Context>();
    inline void clearExceptions();
    inline void setException(v8::Handle<v8::Value> exception);
    inline bool hasUncaughtException() const;
    QScriptValuePrivate* uncaughtException() const;

    v8::Handle<v8::String> qtDataId();

    inline void enterIsolate() const;
    inline void exitIsolate() const;

    void pushScope(QScriptValuePrivate* value);
    QScriptValuePrivate* popScope();

    inline QScriptContextPrivate *setCurrentQSContext(QScriptContextPrivate *ctx);
    inline QScriptContextPrivate *currentContext() { return m_currentQsContext; }
private:
    QScriptEngine* q_ptr;
    v8::Isolate *m_isolate;
    v8::Persistent<v8::Context> m_v8Context;
    QVarLengthArray<v8::Persistent<v8::Context>, 8> m_v8Contexts;
    QVarLengthArray<QPair<v8::Persistent<v8::Context>, v8::Persistent<v8::Value> >, 8> m_v8Scopes;
    v8::Persistent<v8::Value> m_exception;
    QScriptOriginalGlobalObject m_originalGlobalObject;
    v8::Persistent<v8::String> m_qtDataId;

    QHash<const QMetaObject *, v8::Persistent<v8::FunctionTemplate> > m_qtClassTemplates;
    v8::Persistent<v8::FunctionTemplate> m_signalTemplate;
    v8::Persistent<v8::FunctionTemplate> m_variantTemplate;
    v8::Persistent<v8::FunctionTemplate> m_metaObjectTemplate;
    v8::Persistent<v8::ObjectTemplate> m_globalObjectTemplate;
    QScriptContextPrivate *m_currentQsContext;
    QScopedPointer<QScriptContextPrivate> m_baseQsContext;
};

v8::Handle<v8::Value> QScriptEnginePrivate::makeJSValue(bool value)
{
    return value ? v8::True() : v8::False();
}

v8::Handle<v8::Value> QScriptEnginePrivate::makeJSValue(int value)
{
    return v8::Integer::New(value);
}

v8::Handle<v8::Value> QScriptEnginePrivate::makeJSValue(uint value)
{
    return v8::Integer::NewFromUnsigned(value);
}

v8::Handle<v8::Value> QScriptEnginePrivate::makeJSValue(qsreal value)
{
    return v8::Number::New(value);
}

v8::Handle<v8::Value> QScriptEnginePrivate::makeJSValue(QScriptValue::SpecialValue value) {
    if (value == QScriptValue::NullValue)
        return v8::Null();
    return v8::Undefined();
}

v8::Handle<v8::Value> QScriptEnginePrivate::makeJSValue(const QString& value)
{
    return QScriptConverter::toString(value);
}

inline QScriptEnginePrivate::operator v8::Persistent<v8::Context>()
{
    return m_v8Context;
}

/*!
  \internal
  Check if given value is an Error instance
  \attention value had to be an object.
*/
inline bool QScriptEnginePrivate::isError(const QScriptValuePrivate* value) const
{
    return m_originalGlobalObject.isError(value);
}

inline QScriptValue::PropertyFlags QScriptEnginePrivate::getPropertyFlags(v8::Handle<v8::Object> object, v8::Handle<v8::String> property, const QScriptValue::ResolveFlags& mode)
{
    return m_originalGlobalObject.getPropertyFlags(object, property, mode);
}

QScriptValuePrivate* QScriptEnginePrivate::evaluate(const QString& program, const QString& fileName, int lineNumber)
{
    Q_UNUSED(lineNumber);
    v8::TryCatch tryCatch;
    v8::Handle<v8::Script> script = v8::Script::Compile(QScriptConverter::toString(program), QScriptConverter::toString(fileName));
    return evaluate(script, tryCatch);
}

inline QScriptValuePrivate* QScriptEnginePrivate::evaluate(QScriptProgramPrivate* program)
{
    v8::TryCatch tryCatch;
    v8::Handle<v8::Script> script = program->compiled(this);
    return evaluate(script, tryCatch);
}

void QScriptEnginePrivate::collectGarbage()
{
    v8::V8::LowMemoryNotification();
}

void QScriptEnginePrivate::reportAdditionalMemoryCost(int cost)
{
    /*// The check is needed only for compatibility.
    if (cost > 0)
        v8::V8::AdjustAmountOfExternalAllocatedMemory(cost);*/
    // in V8, AdjustAmountOfExternalAllocatedMemory need to be balanced
    // by a negative number when the memory is released, else
    // the garbage collector will think it still has lot of memory and
    // will be run too often.
    if (cost > 0)
        Q_UNIMPLEMENTED();
}

inline void QScriptEnginePrivate::setException(v8::Handle<v8::Value> exception)
{
    if (!m_exception.IsEmpty())
        m_exception.Dispose();

    m_exception = v8::Persistent<v8::Value>::New(exception);
}

inline void QScriptEnginePrivate::clearExceptions()
{
    m_exception.Dispose();
    m_exception.Clear();
}

inline bool QScriptEnginePrivate::hasUncaughtException() const
{
    return !m_exception.IsEmpty();
}

inline void QScriptEnginePrivate::enterIsolate() const
{
    m_isolate->Enter();
    m_v8Context->Enter();
}

inline void QScriptEnginePrivate::exitIsolate() const
{
    m_v8Context->Exit();
    m_isolate->Exit();
}

inline bool QScriptEnginePrivate::isQtVariant(v8::Handle<v8::Value> value) const
{
    return m_variantTemplate->HasInstance(value);
}

inline bool QScriptEnginePrivate::isQtSignal(v8::Handle<v8::Value> value) const
{
    return m_signalTemplate->HasInstance(value);
}

inline bool QScriptEnginePrivate::isQtMetaObject(v8::Handle<v8::Value> value) const
{
    return m_metaObjectTemplate->HasInstance(value);
}

/* set the current QScriptContext, and return the old value */
QScriptContextPrivate* QScriptEnginePrivate::setCurrentQSContext(QScriptContextPrivate *ctx)
{
    qSwap(ctx, m_currentQsContext);
    return ctx;
}

QT_END_NAMESPACE

#endif
