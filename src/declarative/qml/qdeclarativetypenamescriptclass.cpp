/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "private/qdeclarativetypenamescriptclass_p.h"

#include "private/qdeclarativeengine_p.h"
#include "private/qdeclarativetypenamecache_p.h"

QT_BEGIN_NAMESPACE

struct TypeNameData : public QScriptDeclarativeClass::Object {
    TypeNameData(QObject *o, QDeclarativeType *t, QDeclarativeTypeNameScriptClass::TypeNameMode m) : object(o), type(t), typeNamespace(0), mode(m) {}
    TypeNameData(QObject *o, QDeclarativeTypeNameCache *n, QDeclarativeTypeNameScriptClass::TypeNameMode m) : object(o), type(0), typeNamespace(n), mode(m) {
        if (typeNamespace) typeNamespace->addref();
    }
    ~TypeNameData() {
        if (typeNamespace) typeNamespace->release();
    }

    QObject *object;
    QDeclarativeType *type;
    QDeclarativeTypeNameCache *typeNamespace;
    QDeclarativeTypeNameScriptClass::TypeNameMode mode;
};

QDeclarativeTypeNameScriptClass::QDeclarativeTypeNameScriptClass(QDeclarativeEngine *bindEngine)
: QScriptDeclarativeClass(QDeclarativeEnginePrivate::getScriptEngine(bindEngine)), 
  engine(bindEngine), object(0), type(0), api(0)
{
}

QDeclarativeTypeNameScriptClass::~QDeclarativeTypeNameScriptClass()
{
}

QScriptValue QDeclarativeTypeNameScriptClass::newObject(QObject *object, QDeclarativeType *type, TypeNameMode mode)
{
    QScriptEngine *scriptEngine = QDeclarativeEnginePrivate::getScriptEngine(engine);

    return QScriptDeclarativeClass::newObject(scriptEngine, this, new TypeNameData(object, type, mode));
}

QScriptValue QDeclarativeTypeNameScriptClass::newObject(QObject *object, QDeclarativeTypeNameCache *ns, TypeNameMode mode)
{
    QScriptEngine *scriptEngine = QDeclarativeEnginePrivate::getScriptEngine(engine);

    return QScriptDeclarativeClass::newObject(scriptEngine, this, new TypeNameData(object, ns, mode));
}

QScriptClass::QueryFlags 
QDeclarativeTypeNameScriptClass::queryProperty(Object *obj, const Identifier &name, 
                                      QScriptClass::QueryFlags flags)
{
    Q_UNUSED(flags);

    TypeNameData *data = (TypeNameData *)obj;

    object = 0;
    type = 0;
    api = 0;
    QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(engine);

    if (data->typeNamespace) {
        QDeclarativeTypeNameCache::Data *d = data->typeNamespace->data(name);
        if (d && d->type) {
            type = d->type;
            return QScriptClass::HandlesReadAccess;
        } else if (QDeclarativeMetaType::ModuleApiInstance *moduleApi = data->typeNamespace->moduleApi()) {
            if (moduleApi->scriptCallback) {
                moduleApi->scriptApi = moduleApi->scriptCallback(engine, &ep->scriptEngine);
                moduleApi->scriptCallback = 0;
                moduleApi->qobjectCallback = 0;
            } else if (moduleApi->qobjectCallback) {
                moduleApi->qobjectApi = moduleApi->qobjectCallback(engine, &ep->scriptEngine);
                moduleApi->scriptCallback = 0;
                moduleApi->qobjectCallback = 0;
            }

            api = moduleApi;
            if (api->qobjectApi) {
                return ep->objectClass->queryProperty(api->qobjectApi, name, flags, 0,
                                                      QDeclarativeObjectScriptClass::SkipAttachedProperties);
            } else {
                return QScriptClass::HandlesReadAccess;
            }

            return 0;

        } else {
            return 0;
        }

    } else if (data->type) {

        if (startsWithUpper(name)) {
            QString strName = toString(name);
            // Must be an enum
            if (data->mode == IncludeEnums) {
                // ### Optimize
                QByteArray enumName = strName.toUtf8();
                const QMetaObject *metaObject = data->type->baseMetaObject();
                for (int ii = metaObject->enumeratorCount() - 1; ii >= 0; --ii) {
                    QMetaEnum e = metaObject->enumerator(ii);
                    int value = e.keyToValue(enumName.constData());
                    if (value != -1) {
                        enumValue = value;
                        return QScriptClass::HandlesReadAccess;
                    }
                }
            }
            return 0;
        } else if (data->object) {
            // Must be an attached property
            object = qmlAttachedPropertiesObjectById(data->type->attachedPropertiesId(), data->object);
            if (!object) return 0;
            return ep->objectClass->queryProperty(object, name, flags, 0);
        }

    }

    return 0;
}

QDeclarativeTypeNameScriptClass::Value 
QDeclarativeTypeNameScriptClass::property(Object *obj, const Identifier &name)
{
    QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(engine);
    QScriptEngine *scriptEngine = QDeclarativeEnginePrivate::getScriptEngine(engine);
    if (type) {
        return Value(scriptEngine, newObject(((TypeNameData *)obj)->object, type, ((TypeNameData *)obj)->mode));
    } else if (object) {
        return ep->objectClass->property(object, name);
    } else if (api && api->qobjectApi) {
        return ep->objectClass->property(api->qobjectApi, name);
    } else if (api) {
        return propertyValue(api->scriptApi, name);
    } else {
        return Value(scriptEngine, enumValue);
    }
}

void QDeclarativeTypeNameScriptClass::setProperty(Object *, const Identifier &n, const QScriptValue &v)
{
    Q_ASSERT(!type);

    QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(engine);
    if (api) {
        Q_ASSERT(api->qobjectApi);
        ep->objectClass->setProperty(api->qobjectApi, n, v, context());
    } else {
        Q_ASSERT(object);
        ep->objectClass->setProperty(object, n, v, context());
    }
}

QT_END_NAMESPACE

