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

#ifndef QDECLARATIVETYPENAMECACHE_P_H
#define QDECLARATIVETYPENAMECACHE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "private/qdeclarativerefcount_p.h"
#include "private/qdeclarativecleanup_p.h"
#include "private/qdeclarativemetatype_p.h"

#include <private/qscriptdeclarativeclass_p.h>

QT_BEGIN_NAMESPACE

class QDeclarativeType;
class QDeclarativeEngine;
class QDeclarativeTypeNameCache : public QDeclarativeRefCount, public QDeclarativeCleanup
{
public:
    QDeclarativeTypeNameCache(QDeclarativeEngine *);
    virtual ~QDeclarativeTypeNameCache();

    struct Data {
        inline Data();
        inline ~Data();
        QDeclarativeType *type;
        QDeclarativeTypeNameCache *typeNamespace;
        int importedScriptIndex;
    };

    void add(const QString &, int);
    void add(const QString &, QDeclarativeType *);
    void add(const QString &, QDeclarativeTypeNameCache *);

    Data *data(const QString &) const;
    inline Data *data(const QScriptDeclarativeClass::Identifier &id) const;
    inline bool isEmpty() const;

    inline QDeclarativeMetaType::ModuleApiInstance *moduleApi() const;
    void setModuleApi(QDeclarativeMetaType::ModuleApiInstance *);

protected:
    virtual void clear();

private:
    struct RData : public Data { 
        QScriptDeclarativeClass::PersistentIdentifier identifier;
    };
    typedef QHash<QString, RData *> StringCache;
    typedef QHash<QScriptDeclarativeClass::Identifier, RData *> IdentifierCache;

    StringCache stringCache;
    IdentifierCache identifierCache;
    QDeclarativeEngine *engine;
    QDeclarativeMetaType::ModuleApiInstance *m_moduleApi;
};

QDeclarativeTypeNameCache::Data::Data()
: type(0), typeNamespace(0), importedScriptIndex(-1)
{
}

QDeclarativeTypeNameCache::Data::~Data()
{
    if (typeNamespace) typeNamespace->release();
}

QDeclarativeTypeNameCache::Data *QDeclarativeTypeNameCache::data(const QScriptDeclarativeClass::Identifier &id) const
{
    return identifierCache.value(id);
}

bool QDeclarativeTypeNameCache::isEmpty() const
{
    return identifierCache.isEmpty();
}

QDeclarativeMetaType::ModuleApiInstance *QDeclarativeTypeNameCache::moduleApi() const
{
    return m_moduleApi;
}

QT_END_NAMESPACE

#endif // QDECLARATIVETYPENAMECACHE_P_H

