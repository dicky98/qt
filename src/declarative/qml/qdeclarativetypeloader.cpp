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

#include "qdeclarativetypeloader_p.h"

#include <private/qdeclarativeengine_p.h>
#include <private/qdeclarativecompiler_p.h>
#include <private/qdeclarativecomponent_p.h>
#include <private/qdeclarativeglobal_p.h>
#include <private/qdeclarativedebugtrace_p.h>

#include <QtDeclarative/qdeclarativecomponent.h>
#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qfile.h>

QT_BEGIN_NAMESPACE

/*!
\class QDeclarativeDataBlob
\brief The QDeclarativeDataBlob encapsulates a data request that can be issued to a QDeclarativeDataLoader.
\internal

QDeclarativeDataBlob's are loaded by a QDeclarativeDataLoader.  The user creates the QDeclarativeDataBlob
and then calls QDeclarativeDataLoader::load() or QDeclarativeDataLoader::loadWithStaticData() to load it.
The QDeclarativeDataLoader invokes callbacks on the QDeclarativeDataBlob as data becomes available.
*/

/*!
\enum QDeclarativeDataBlob::Status

This enum describes the status of the data blob.

\list
\o Null The blob has not yet been loaded by a QDeclarativeDataLoader
\o Loading The blob is loading network data.  The QDeclarativeDataBlob::setData() callback has not yet been
invoked or has not yet returned.
\o WaitingForDependencies The blob is waiting for dependencies to be done before continueing.  This status
only occurs after the QDeclarativeDataBlob::setData() callback has been made, and when the blob has outstanding
dependencies.
\o Complete The blob's data has been loaded and all dependencies are done.
\o Error An error has been set on this blob.
\endlist
*/

/*!
\enum QDeclarativeDataBlob::Type

This enum describes the type of the data blob.

\list
\o QmlFile This is a QDeclarativeTypeData
\o JavaScriptFile This is a QDeclarativeScriptData
\o QmldirFile This is a QDeclarativeQmldirData
\endlist
*/

/*!
Create a new QDeclarativeDataBlob for \a url and of the provided \a type.
*/
QDeclarativeDataBlob::QDeclarativeDataBlob(const QUrl &url, Type type)
: m_type(type), m_status(Null), m_progress(0), m_url(url), m_finalUrl(url), m_manager(0),
  m_redirectCount(0), m_inCallback(false), m_isDone(false)
{
}

/*!  \internal */
QDeclarativeDataBlob::~QDeclarativeDataBlob()
{
    Q_ASSERT(m_waitingOnMe.isEmpty());

    cancelAllWaitingFor();
}

/*!
Returns the type provided to the constructor.
*/
QDeclarativeDataBlob::Type QDeclarativeDataBlob::type() const
{
    return m_type;
}

/*!
Returns the blob's status.
*/
QDeclarativeDataBlob::Status QDeclarativeDataBlob::status() const
{
    return m_status;
}

/*!
Returns true if the status is Null.
*/
bool QDeclarativeDataBlob::isNull() const
{
    return m_status == Null;
}

/*!
Returns true if the status is Loading.
*/
bool QDeclarativeDataBlob::isLoading() const
{
    return m_status == Loading;
}

/*!
Returns true if the status is WaitingForDependencies.
*/
bool QDeclarativeDataBlob::isWaiting() const
{
    return m_status == WaitingForDependencies;
}

/*!
Returns true if the status is Complete.
*/
bool QDeclarativeDataBlob::isComplete() const
{
    return m_status == Complete;
}

/*!
Returns true if the status is Error.
*/
bool QDeclarativeDataBlob::isError() const
{
    return m_status == Error;
}

/*!
Returns true if the status is Complete or Error.
*/
bool QDeclarativeDataBlob::isCompleteOrError() const
{
    return isComplete() || isError();
}

/*!
Returns the data download progress from 0 to 1.
*/
qreal QDeclarativeDataBlob::progress() const
{
    return m_progress;
}

/*!
Returns the blob url passed to the constructor.  If a network redirect
happens while fetching the data, this url remains the same.

\sa finalUrl()
*/
QUrl QDeclarativeDataBlob::url() const
{
    return m_url;
}

/*!
Returns the final url of the data.  Initially this is the same as
url(), but if a network redirect happens while fetching the data, this url
is updated to reflect the new location.
*/
QUrl QDeclarativeDataBlob::finalUrl() const
{
    return m_finalUrl;
}

/*!
Return the errors on this blob.
*/
QList<QDeclarativeError> QDeclarativeDataBlob::errors() const
{
    return m_errors;
}

/*!
Mark this blob as having \a errors.

All outstanding dependencies will be cancelled.  Requests to add new dependencies 
will be ignored.  Entry into the Error state is irreversable, although you can change the 
specific errors by additional calls to setError.
*/
void QDeclarativeDataBlob::setError(const QDeclarativeError &errors)
{
    QList<QDeclarativeError> l;
    l << errors;
    setError(l);
}

/*!
\overload
*/
void QDeclarativeDataBlob::setError(const QList<QDeclarativeError> &errors)
{
    m_status = Error;
    m_errors = errors;

    cancelAllWaitingFor();

    if (!m_inCallback)
        tryDone();
}

/*! 
Wait for \a blob to become complete or to error.  If \a blob is already 
complete or in error, or this blob is already complete, this has no effect.
*/
void QDeclarativeDataBlob::addDependency(QDeclarativeDataBlob *blob)
{
    Q_ASSERT(status() != Null);

    if (!blob ||
        blob->status() == Error || blob->status() == Complete ||
        status() == Error || status() == Complete ||
        m_waitingFor.contains(blob))
        return;

    blob->addref();
    m_status = WaitingForDependencies;
    m_waitingFor.append(blob);
    blob->m_waitingOnMe.append(this);
}

/*!
\fn void QDeclarativeDataBlob::dataReceived(const QByteArray &data)

Invoked when data for the blob is received.  Implementors should use this callback
to determine a blob's dependencies.  Within this callback you may call setError()
or addDependency().
*/

/*!
Invoked once data has either been received or a network error occurred, and all 
dependencies are complete.

You can set an error in this method, but you cannot add new dependencies.  Implementors
should use this callback to finalize processing of data.

The default implementation does nothing.
*/
void QDeclarativeDataBlob::done()
{
}

/*!
Invoked if there is a network error while fetching this blob.

The default implementation sets an appropriate QDeclarativeError.
*/
void QDeclarativeDataBlob::networkError(QNetworkReply::NetworkError networkError)
{
    Q_UNUSED(networkError);

    QDeclarativeError error;
    error.setUrl(m_finalUrl);

    const char *errorString = 0;
    switch (networkError) {
        default:
            errorString = "Network error";
            break;
        case QNetworkReply::ConnectionRefusedError:
            errorString = "Connection refused";
            break;
        case QNetworkReply::RemoteHostClosedError:
            errorString = "Remote host closed the connection";
            break;
        case QNetworkReply::HostNotFoundError:
            errorString = "Host not found";
            break;
        case QNetworkReply::TimeoutError:
            errorString = "Timeout";
            break;
        case QNetworkReply::ProxyConnectionRefusedError:
        case QNetworkReply::ProxyConnectionClosedError:
        case QNetworkReply::ProxyNotFoundError:
        case QNetworkReply::ProxyTimeoutError:
        case QNetworkReply::ProxyAuthenticationRequiredError:
        case QNetworkReply::UnknownProxyError:
            errorString = "Proxy error";
            break;
        case QNetworkReply::ContentAccessDenied:
            errorString = "Access denied";
            break;
        case QNetworkReply::ContentNotFoundError:
            errorString = "File not found";
            break;
        case QNetworkReply::AuthenticationRequiredError:
            errorString = "Authentication required";
            break;
    };

    error.setDescription(QLatin1String(errorString));

    setError(error);
}

/*! 
Called if \a blob, which was previously waited for, has an error.

The default implementation does nothing.
*/
void QDeclarativeDataBlob::dependencyError(QDeclarativeDataBlob *blob)
{
    Q_UNUSED(blob);
}

/*!
Called if \a blob, which was previously waited for, has completed.

The default implementation does nothing.
*/
void QDeclarativeDataBlob::dependencyComplete(QDeclarativeDataBlob *blob)
{
    Q_UNUSED(blob);
}

/*! 
Called when all blobs waited for have completed.  This occurs regardless of 
whether they are in error, or complete state.  

The default implementation does nothing.
*/
void QDeclarativeDataBlob::allDependenciesDone()
{
}

/*!
Called when the download progress of this blob changes.  \a progress goes
from 0 to 1.
*/
void QDeclarativeDataBlob::downloadProgressChanged(qreal progress)
{
    Q_UNUSED(progress);
}

void QDeclarativeDataBlob::tryDone()
{
    if (status() != Loading && m_waitingFor.isEmpty() && !m_isDone) {
        if (status() != Error)
            m_status = Complete;

        m_isDone = true;
        done();
        notifyAllWaitingOnMe();
    }
}

void QDeclarativeDataBlob::cancelAllWaitingFor()
{
    while (m_waitingFor.count()) {
        QDeclarativeDataBlob *blob = m_waitingFor.takeLast();

        Q_ASSERT(blob->m_waitingOnMe.contains(this));

        blob->m_waitingOnMe.removeOne(this);

        blob->release();
    }
}

void QDeclarativeDataBlob::notifyAllWaitingOnMe()
{
    while (m_waitingOnMe.count()) {
        QDeclarativeDataBlob *blob = m_waitingOnMe.takeLast();

        Q_ASSERT(blob->m_waitingFor.contains(this));

        blob->notifyComplete(this);
    }
}

void QDeclarativeDataBlob::notifyComplete(QDeclarativeDataBlob *blob)
{
    Q_ASSERT(m_waitingFor.contains(blob));
    Q_ASSERT(blob->status() == Error || blob->status() == Complete);

    m_inCallback = true;

    if (blob->status() == Error) {
        dependencyError(blob);
    } else if (blob->status() == Complete) {
        dependencyComplete(blob);
    }

    m_waitingFor.removeOne(blob);
    blob->release();

    if (!isError() && m_waitingFor.isEmpty()) 
        allDependenciesDone();

    m_inCallback = false;

    tryDone();
}

/*!
\class QDeclarativeDataLoader
\brief The QDeclarativeDataLoader class abstracts loading files and their dependecies over the network.
\internal

The QDeclarativeDataLoader class is provided for the exclusive use of the QDeclarativeTypeLoader class.

Clients create QDeclarativeDataBlob instances and submit them to the QDeclarativeDataLoader class
through the QDeclarativeDataLoader::load() or QDeclarativeDataLoader::loadWithStaticData() methods.
The loader then fetches the data over the network or from the local file system in an efficient way.
QDeclarativeDataBlob is an abstract class, so should always be specialized.

Once data is received, the QDeclarativeDataBlob::dataReceived() method is invoked on the blob.  The
derived class should use this callback to process the received data.  Processing of the data can 
result in an error being set (QDeclarativeDataBlob::setError()), or one or more dependencies being
created (QDeclarativeDataBlob::addDependency()).  Dependencies are other QDeclarativeDataBlob's that
are required before processing can fully complete.

To complete processing, the QDeclarativeDataBlob::done() callback is invoked.  done() is called when
one of these three preconditions are met.

1.  The QDeclarativeDataBlob has no dependencies.
2.  The QDeclarativeDataBlob has an error set.
3.  All the QDeclarativeDataBlob's dependencies are themselves "done()".

Thus QDeclarativeDataBlob::done() will always eventually be called, even if the blob has an error set.
*/

/*!
Create a new QDeclarativeDataLoader for \a engine.
*/
QDeclarativeDataLoader::QDeclarativeDataLoader(QDeclarativeEngine *engine)
: m_engine(engine)
{
}

/*! \internal */
QDeclarativeDataLoader::~QDeclarativeDataLoader()
{
    for (NetworkReplies::Iterator iter = m_networkReplies.begin(); iter != m_networkReplies.end(); ++iter) 
        (*iter)->release();
}

/*!
Load the provided \a blob from the network or filesystem.
*/
void QDeclarativeDataLoader::load(QDeclarativeDataBlob *blob)
{
    Q_ASSERT(blob->status() == QDeclarativeDataBlob::Null);
    Q_ASSERT(blob->m_manager == 0);

    blob->m_status = QDeclarativeDataBlob::Loading;

    if (blob->m_url.isEmpty()) {
        QDeclarativeError error;
        error.setDescription(QLatin1String("Invalid null URL"));
        blob->setError(error);
        return;
    }

    QString lf = QDeclarativeEnginePrivate::urlToLocalFileOrQrc(blob->m_url);

    if (!lf.isEmpty()) {
        if (!QDeclarative_isFileCaseCorrect(lf)) {
            QDeclarativeError error;
            error.setUrl(blob->m_url);
            error.setDescription(QLatin1String("File name case mismatch"));
            blob->setError(error);
            return;
        }
        QFile file(lf);
        if (file.open(QFile::ReadOnly)) {
            QByteArray data = file.readAll();

            blob->m_progress = 1.;
            blob->downloadProgressChanged(1.);

            setData(blob, data);
        } else {
            blob->networkError(QNetworkReply::ContentNotFoundError);
        }

    } else {

        blob->m_manager = this;
        QNetworkReply *reply = m_engine->networkAccessManager()->get(QNetworkRequest(blob->m_url));
        QObject::connect(reply, SIGNAL(downloadProgress(qint64,qint64)), 
                         this, SLOT(networkReplyProgress(qint64,qint64)));
        QObject::connect(reply, SIGNAL(finished()), 
                         this, SLOT(networkReplyFinished()));
        m_networkReplies.insert(reply, blob);

        blob->addref();
    }
}

#define DATALOADER_MAXIMUM_REDIRECT_RECURSION 16

void QDeclarativeDataLoader::networkReplyFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    QDeclarativeDataBlob *blob = m_networkReplies.take(reply);

    Q_ASSERT(blob);

    blob->m_redirectCount++;

    if (blob->m_redirectCount < DATALOADER_MAXIMUM_REDIRECT_RECURSION) {
        QVariant redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if (redirect.isValid()) {
            QUrl url = reply->url().resolved(redirect.toUrl());
            blob->m_finalUrl = url;

            QNetworkReply *reply = m_engine->networkAccessManager()->get(QNetworkRequest(url));
            QObject::connect(reply, SIGNAL(finished()), this, SLOT(networkReplyFinished()));
            m_networkReplies.insert(reply, blob);
            return;
        }
    }

    if (reply->error()) {
        blob->networkError(reply->error());
    } else {
        QByteArray data = reply->readAll();
        setData(blob, data);
    }

    blob->release();
}

void QDeclarativeDataLoader::networkReplyProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    QDeclarativeDataBlob *blob = m_networkReplies.value(reply);

    Q_ASSERT(blob);

    if (bytesTotal != 0) {
        blob->m_progress = bytesReceived / bytesTotal;
        blob->downloadProgressChanged(blob->m_progress);
    }
}

/*!
Load the provided \a blob with \a data.  The blob's URL is not used by the data loader in this case.
*/
void QDeclarativeDataLoader::loadWithStaticData(QDeclarativeDataBlob *blob, const QByteArray &data)
{
    Q_ASSERT(blob->status() == QDeclarativeDataBlob::Null);
    Q_ASSERT(blob->m_manager == 0);
    
    blob->m_status = QDeclarativeDataBlob::Loading;

    setData(blob, data);
}

/*!
Return the QDeclarativeEngine associated with this loader
*/
QDeclarativeEngine *QDeclarativeDataLoader::engine() const
{
    return m_engine;
}

void QDeclarativeDataLoader::setData(QDeclarativeDataBlob *blob, const QByteArray &data)
{
    blob->m_inCallback = true;

    blob->dataReceived(data);

    if (!blob->isError() && !blob->isWaiting())
        blob->allDependenciesDone();

    if (blob->status() != QDeclarativeDataBlob::Error)
        blob->m_status = QDeclarativeDataBlob::WaitingForDependencies;

    blob->m_inCallback = false;

    blob->tryDone();
}

/*!
\class QDeclarativeTypeLoader
*/
QDeclarativeTypeLoader::QDeclarativeTypeLoader(QDeclarativeEngine *engine)
: QDeclarativeDataLoader(engine)
{
}

QDeclarativeTypeLoader::~QDeclarativeTypeLoader()
{
    clearCache();
}

/*!
\enum QDeclarativeTypeLoader::Option

This enum defines the options that control the way type data is handled.

\value None             The default value, indicating that no other options
                        are enabled.
\value PreserveParser   The parser used to handle the type data is preserved
                        after the data has been parsed.
*/

/*!
Returns a QDeclarativeTypeData for the specified \a url.  The QDeclarativeTypeData may be cached.
*/
QDeclarativeTypeData *QDeclarativeTypeLoader::get(const QUrl &url)
{
    Q_ASSERT(!url.isRelative() && 
            (QDeclarativeEnginePrivate::urlToLocalFileOrQrc(url).isEmpty() || 
             !QDir::isRelativePath(QDeclarativeEnginePrivate::urlToLocalFileOrQrc(url))));

    QDeclarativeTypeData *typeData = m_typeCache.value(url);

    if (!typeData) {
        typeData = new QDeclarativeTypeData(url, None, this);
        m_typeCache.insert(url, typeData);
        QDeclarativeDataLoader::load(typeData);
    }

    typeData->addref();
    return typeData;
}

/*!
Returns a QDeclarativeTypeData for the given \a data with the provided base \a url.  The 
QDeclarativeTypeData will not be cached.

The specified \a options control how the loader handles type data.
*/
QDeclarativeTypeData *QDeclarativeTypeLoader::get(const QByteArray &data, const QUrl &url, Options options)
{
    QDeclarativeTypeData *typeData = new QDeclarativeTypeData(url, options, this);
    QDeclarativeDataLoader::loadWithStaticData(typeData, data);
    return typeData;
}

/*!
Return a QDeclarativeScriptBlob for \a url.  The QDeclarativeScriptData may be cached.
*/
QDeclarativeScriptBlob *QDeclarativeTypeLoader::getScript(const QUrl &url)
{
    Q_ASSERT(!url.isRelative() && 
            (QDeclarativeEnginePrivate::urlToLocalFileOrQrc(url).isEmpty() || 
             !QDir::isRelativePath(QDeclarativeEnginePrivate::urlToLocalFileOrQrc(url))));

    QDeclarativeScriptBlob *scriptBlob = m_scriptCache.value(url);

    if (!scriptBlob) {
        scriptBlob = new QDeclarativeScriptBlob(url, this);
        m_scriptCache.insert(url, scriptBlob);
        QDeclarativeDataLoader::load(scriptBlob);
    }

    return scriptBlob;
}

/*!
Return a QDeclarativeQmldirData for \a url.  The QDeclarativeQmldirData may be cached.
*/
QDeclarativeQmldirData *QDeclarativeTypeLoader::getQmldir(const QUrl &url)
{
    Q_ASSERT(!url.isRelative() && 
            (QDeclarativeEnginePrivate::urlToLocalFileOrQrc(url).isEmpty() || 
             !QDir::isRelativePath(QDeclarativeEnginePrivate::urlToLocalFileOrQrc(url))));

    QDeclarativeQmldirData *qmldirData = m_qmldirCache.value(url);

    if (!qmldirData) {
        qmldirData = new QDeclarativeQmldirData(url);
        m_qmldirCache.insert(url, qmldirData);
        QDeclarativeDataLoader::load(qmldirData);
    }

    qmldirData->addref();
    return qmldirData;
}

void QDeclarativeTypeLoader::clearCache()
{
    for (TypeCache::Iterator iter = m_typeCache.begin(); iter != m_typeCache.end(); ++iter) 
        (*iter)->release();
    for (ScriptCache::Iterator iter = m_scriptCache.begin(); iter != m_scriptCache.end(); ++iter) 
        (*iter)->release();
    for (QmldirCache::Iterator iter = m_qmldirCache.begin(); iter != m_qmldirCache.end(); ++iter) 
        (*iter)->release();

    m_typeCache.clear();
    m_scriptCache.clear();
    m_qmldirCache.clear();
}


QDeclarativeTypeData::QDeclarativeTypeData(const QUrl &url, QDeclarativeTypeLoader::Options options, 
                                           QDeclarativeTypeLoader *manager)
: QDeclarativeDataBlob(url, QmlFile), m_options(options), m_typesResolved(false), 
  m_compiledData(0), m_typeLoader(manager)
{
}

QDeclarativeTypeData::~QDeclarativeTypeData()
{
    for (int ii = 0; ii < m_scripts.count(); ++ii) 
        m_scripts.at(ii).script->release();
    for (int ii = 0; ii < m_qmldirs.count(); ++ii) 
        m_qmldirs.at(ii)->release();
    for (int ii = 0; ii < m_types.count(); ++ii) 
        if (m_types.at(ii).typeData) m_types.at(ii).typeData->release();
    if (m_compiledData)
        m_compiledData->release();
}

QDeclarativeTypeLoader *QDeclarativeTypeData::typeLoader() const
{
    return m_typeLoader;
}

const QDeclarativeImports &QDeclarativeTypeData::imports() const
{
    return m_imports;
}

const QDeclarativeScriptParser &QDeclarativeTypeData::parser() const
{
    return scriptParser;
}

const QList<QDeclarativeTypeData::TypeReference> &QDeclarativeTypeData::resolvedTypes() const
{
    return m_types;
}

const QList<QDeclarativeTypeData::ScriptReference> &QDeclarativeTypeData::resolvedScripts() const
{
    return m_scripts;
}

QDeclarativeCompiledData *QDeclarativeTypeData::compiledData() const
{
    if (m_compiledData) 
        m_compiledData->addref();

    return m_compiledData;
}

void QDeclarativeTypeData::registerCallback(TypeDataCallback *callback)
{
    Q_ASSERT(!m_callbacks.contains(callback));
    m_callbacks.append(callback);
}

void QDeclarativeTypeData::unregisterCallback(TypeDataCallback *callback)
{
    Q_ASSERT(m_callbacks.contains(callback));
    m_callbacks.removeOne(callback);
    Q_ASSERT(!m_callbacks.contains(callback));
}

void QDeclarativeTypeData::done()
{
    addref();

    // Check all script dependencies for errors
    for (int ii = 0; !isError() && ii < m_scripts.count(); ++ii) {
        const ScriptReference &script = m_scripts.at(ii);
        Q_ASSERT(script.script->isCompleteOrError());
        if (script.script->isError()) {
            QList<QDeclarativeError> errors = script.script->errors();
            QDeclarativeError error;
            error.setUrl(finalUrl());
            error.setLine(script.location.line);
            error.setColumn(script.location.column);
            error.setDescription(QDeclarativeTypeLoader::tr("Script %1 unavailable").arg(script.script->url().toString()));
            errors.prepend(error);
            setError(errors);
        }
    }

    // Check all type dependencies for errors
    for (int ii = 0; !isError() && ii < m_types.count(); ++ii) {
        const TypeReference &type = m_types.at(ii);
        Q_ASSERT(!type.typeData || type.typeData->isCompleteOrError());
        if (type.typeData && type.typeData->isError()) {
            QString typeName = scriptParser.referencedTypes().at(ii)->name;

            QList<QDeclarativeError> errors = type.typeData->errors();
            QDeclarativeError error;
            error.setUrl(finalUrl());
            error.setLine(type.location.line);
            error.setColumn(type.location.column);
            error.setDescription(QDeclarativeTypeLoader::tr("Type %1 unavailable").arg(typeName));
            errors.prepend(error);
            setError(errors);
        }
    }

    // Compile component
    if (!isError()) 
        compile();

    if (!(m_options & QDeclarativeTypeLoader::PreserveParser))
        scriptParser.clear();

    // Notify callbacks
    while (!m_callbacks.isEmpty()) {
        TypeDataCallback *callback = m_callbacks.takeFirst();
        callback->typeDataReady(this);
    }

    release();
}

void QDeclarativeTypeData::dataReceived(const QByteArray &data)
{
    if (!scriptParser.parse(data, finalUrl())) {
        setError(scriptParser.errors());
        return;
    }

    m_imports.setBaseUrl(finalUrl());

    foreach (const QDeclarativeScriptParser::Import &import, scriptParser.imports()) {
        if (import.type == QDeclarativeScriptParser::Import::File && import.qualifier.isEmpty()) {
            QUrl importUrl = finalUrl().resolved(QUrl(import.uri + QLatin1String("/qmldir")));
            if (QDeclarativeEnginePrivate::urlToLocalFileOrQrc(importUrl).isEmpty()) {
                QDeclarativeQmldirData *data = typeLoader()->getQmldir(importUrl);
                addDependency(data);
                m_qmldirs << data;
            }
        } else if (import.type == QDeclarativeScriptParser::Import::Script) {
            QUrl scriptUrl = finalUrl().resolved(QUrl(import.uri));
            QDeclarativeScriptBlob *blob = typeLoader()->getScript(scriptUrl);
            addDependency(blob);

            ScriptReference ref;
            ref.location = import.location.start;
            ref.qualifier = import.qualifier;
            ref.script = blob;
            blob->addref();
            m_scripts << ref;

        }
    }

    if (!finalUrl().scheme().isEmpty()) {
        QUrl importUrl = finalUrl().resolved(QUrl(QLatin1String("qmldir")));
        if (QDeclarativeEnginePrivate::urlToLocalFileOrQrc(importUrl).isEmpty()) {
            QDeclarativeQmldirData *data = typeLoader()->getQmldir(importUrl);
            addDependency(data);
            m_qmldirs << data;
        }
    }
}

void QDeclarativeTypeData::allDependenciesDone()
{
    if (!m_typesResolved) {
        resolveTypes();
        m_typesResolved = true;
    }
}

void QDeclarativeTypeData::downloadProgressChanged(qreal p)
{
    for (int ii = 0; ii < m_callbacks.count(); ++ii) {
        TypeDataCallback *callback = m_callbacks.at(ii);
        callback->typeDataProgress(this, p);
    }
}

void QDeclarativeTypeData::compile()
{
    Q_ASSERT(m_compiledData == 0);
    QDeclarativeDebugTrace::startRange(QDeclarativeDebugTrace::Compiling);

    m_compiledData = new QDeclarativeCompiledData(typeLoader()->engine());
    m_compiledData->url = m_imports.baseUrl();
    m_compiledData->name = m_compiledData->url.toString();
    QDeclarativeDebugTrace::rangeData(QDeclarativeDebugTrace::Compiling, m_compiledData->name);

    QDeclarativeCompiler compiler;
    if (!compiler.compile(typeLoader()->engine(), this, m_compiledData)) {
        setError(compiler.errors());
        m_compiledData->release();
        m_compiledData = 0;
    }
    QDeclarativeDebugTrace::endRange(QDeclarativeDebugTrace::Compiling);
}

void QDeclarativeTypeData::resolveTypes()
{
    QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(m_typeLoader->engine());
    QDeclarativeImportDatabase *importDatabase = &ep->importDatabase;

    // For local urls, add an implicit import "." as first (most overridden) lookup. 
    // This will also trigger the loading of the qmldir and the import of any native 
    // types from available plugins.
    QList<QDeclarativeError> errors;
    if (QDeclarativeQmldirData *qmldir = qmldirForUrl(finalUrl().resolved(QUrl(QLatin1String("./qmldir"))))) {
        m_imports.addImport(importDatabase, QLatin1String("."),
                            QString(), -1, -1, QDeclarativeScriptParser::Import::File, 
                            qmldir->dirComponents(), &errors);
    } else {
        m_imports.addImport(importDatabase, QLatin1String("."), 
                            QString(), -1, -1, QDeclarativeScriptParser::Import::File, 
                            QDeclarativeDirComponents(), &errors);
    }

    // remove any errors which are due to the implicit import which aren't real errors.
    // for example, if the implicitly included qmldir file doesn't exist, that is not an error.
    QList<QDeclarativeError> realErrors;
    for (int i = 0; i < errors.size(); ++i) {
        if (errors.at(i).description() != QDeclarativeImportDatabase::tr("import \".\" has no qmldir and no namespace")
                && errors.at(i).description() != QDeclarativeImportDatabase::tr("\".\": no such directory")) {
            realErrors.prepend(errors.at(i)); // this is a real error.
        }
    }

    // report any real errors which occurred during plugin loading or qmldir parsing.
    if (!realErrors.isEmpty()) {
        setError(realErrors);
        return;
    }

    foreach (const QDeclarativeScriptParser::Import &import, scriptParser.imports()) {
        QDeclarativeDirComponents qmldircomponentsnetwork;
        if (import.type == QDeclarativeScriptParser::Import::Script)
            continue;

        if (import.type == QDeclarativeScriptParser::Import::File && import.qualifier.isEmpty()) {
            QUrl qmldirUrl = finalUrl().resolved(QUrl(import.uri + QLatin1String("/qmldir")));
            if (QDeclarativeQmldirData *qmldir = qmldirForUrl(qmldirUrl))
                qmldircomponentsnetwork = qmldir->dirComponents();
        }

        int vmaj = -1;
        int vmin = -1;
        import.extractVersion(&vmaj, &vmin);

        QList<QDeclarativeError> errors;
        if (!m_imports.addImport(importDatabase, import.uri, import.qualifier,
                                 vmaj, vmin, import.type, qmldircomponentsnetwork, &errors)) {
            QDeclarativeError error;
            if (errors.size()) {
                error = errors.takeFirst();
            } else {
                // this should not be possible!
                // Description should come from error provided by addImport() function.
                error.setDescription(QDeclarativeTypeLoader::tr("Unreported error adding script import to import database"));
            }
            error.setUrl(m_imports.baseUrl());
            error.setLine(import.location.start.line);
            error.setColumn(import.location.start.column);
            errors.prepend(error); // put it back on the list after filling out information.

            setError(errors);
            return;
        }
    }

    foreach (QDeclarativeScriptParser::TypeReference *parserRef, scriptParser.referencedTypes()) {
        QByteArray typeName = parserRef->name.toUtf8();

        TypeReference ref;

        QUrl url;
        int majorVersion;
        int minorVersion;
        QDeclarativeImportedNamespace *typeNamespace = 0;
        QList<QDeclarativeError> errors;

        if (!m_imports.resolveType(typeName, &ref.type, &url, &majorVersion, &minorVersion,
                                   &typeNamespace, &errors) || typeNamespace) {
            // Known to not be a type:
            //  - known to be a namespace (Namespace {})
            //  - type with unknown namespace (UnknownNamespace.SomeType {})
            QDeclarativeError error;
            QString userTypeName = parserRef->name;
            userTypeName.replace(QLatin1Char('/'),QLatin1Char('.'));
            if (typeNamespace) {
                error.setDescription(QDeclarativeTypeLoader::tr("Namespace %1 cannot be used as a type").arg(userTypeName));
            } else {
                if (errors.size()) {
                    error = errors.takeFirst();
                } else {
                    // this should not be possible!
                    // Description should come from error provided by addImport() function.
                    error.setDescription(QDeclarativeTypeLoader::tr("Unreported error adding script import to import database"));
                }
                error.setUrl(m_imports.baseUrl());
                error.setDescription(QDeclarativeTypeLoader::tr("%1 %2").arg(userTypeName).arg(error.description()));
            }

            if (!parserRef->refObjects.isEmpty()) {
                QDeclarativeParser::Object *obj = parserRef->refObjects.first();
                error.setLine(obj->location.start.line);
                error.setColumn(obj->location.start.column);
            }

            errors.prepend(error);
            setError(errors);
            return;
        }

        if (ref.type) {
            ref.majorVersion = majorVersion;
            ref.minorVersion = minorVersion;
            foreach (QDeclarativeParser::Object *obj, parserRef->refObjects) {
               // store namespace for DOM
               obj->majorVersion = majorVersion;
               obj->minorVersion = minorVersion;
            }
        } else {
            ref.typeData = typeLoader()->get(url);
            addDependency(ref.typeData);
        }

        if (parserRef->refObjects.count())
            ref.location = parserRef->refObjects.first()->location.start;

        m_types << ref;
    }
}

QDeclarativeQmldirData *QDeclarativeTypeData::qmldirForUrl(const QUrl &url)
{
    for (int ii = 0; ii < m_qmldirs.count(); ++ii) {
        if (m_qmldirs.at(ii)->url() == url)
            return m_qmldirs.at(ii);
    }
    return 0;
}

QDeclarativeScriptData::QDeclarativeScriptData(QDeclarativeEngine *engine)
: QDeclarativeCleanup(engine), importCache(0), pragmas(QDeclarativeParser::Object::ScriptBlock::None),
  m_loaded(false)
{
}

QDeclarativeScriptData::~QDeclarativeScriptData()
{
    clear();
}

void QDeclarativeScriptData::clear()
{
    if (importCache) {
        importCache->release();
        importCache = 0;
    }

    for (int ii = 0; ii < scripts.count(); ++ii)
        scripts.at(ii)->release();
    scripts.clear();
}

QDeclarativeScriptBlob::QDeclarativeScriptBlob(const QUrl &url, QDeclarativeTypeLoader *loader)
: QDeclarativeDataBlob(url, JavaScriptFile), m_pragmas(QDeclarativeParser::Object::ScriptBlock::None),
  m_scriptData(0), m_typeLoader(loader)
{
}

QDeclarativeScriptBlob::~QDeclarativeScriptBlob()
{
    if (m_scriptData) {
        m_scriptData->release();
        m_scriptData = 0;
    }
}

QDeclarativeParser::Object::ScriptBlock::Pragmas QDeclarativeScriptBlob::pragmas() const
{
    return m_pragmas;
}

QString QDeclarativeScriptBlob::scriptSource() const
{
    return m_source;
}

QDeclarativeTypeLoader *QDeclarativeScriptBlob::typeLoader() const
{
    return m_typeLoader;
}

const QDeclarativeImports &QDeclarativeScriptBlob::imports() const
{
    return m_imports;
}

QDeclarativeScriptData *QDeclarativeScriptBlob::scriptData() const
{
    return m_scriptData;
}

void QDeclarativeScriptBlob::dataReceived(const QByteArray &data)
{
    QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(m_typeLoader->engine());
    QDeclarativeImportDatabase *importDatabase = &ep->importDatabase;

    m_source = QString::fromUtf8(data);

    QDeclarativeScriptParser::JavaScriptMetaData metadata =
        QDeclarativeScriptParser::extractMetaData(m_source);

    m_imports.setBaseUrl(finalUrl());

    m_pragmas = metadata.pragmas;

    foreach (const QDeclarativeScriptParser::Import &import, metadata.imports) {
        Q_ASSERT(import.type != QDeclarativeScriptParser::Import::File);

        if (import.type == QDeclarativeScriptParser::Import::Script) {
            QUrl scriptUrl = finalUrl().resolved(QUrl(import.uri));
            QDeclarativeScriptBlob *blob = typeLoader()->getScript(scriptUrl);
            addDependency(blob);

            ScriptReference ref;
            ref.location = import.location.start;
            ref.qualifier = import.qualifier;
            ref.script = blob;
            blob->addref();
            m_scripts << ref;
        } else {
            Q_ASSERT(import.type == QDeclarativeScriptParser::Import::Library);
            int vmaj = -1;
            int vmin = -1;
            import.extractVersion(&vmaj, &vmin);

            QList<QDeclarativeError> errors;
            if (!m_imports.addImport(importDatabase, import.uri, import.qualifier, vmaj, vmin,
                                     import.type, QDeclarativeDirComponents(), &errors)) {
                QDeclarativeError error = errors.takeFirst();
                // description should be set by addImport().
                error.setUrl(m_imports.baseUrl());
                error.setLine(import.location.start.line);
                error.setColumn(import.location.start.column);
                errors.prepend(error);

                setError(errors);
                return;
            }
        }
    }
}

void QDeclarativeScriptBlob::done()
{
    // Check all script dependencies for errors
    for (int ii = 0; !isError() && ii < m_scripts.count(); ++ii) {
        const ScriptReference &script = m_scripts.at(ii);
        Q_ASSERT(script.script->isCompleteOrError());
        if (script.script->isError()) {
            QList<QDeclarativeError> errors = script.script->errors();
            QDeclarativeError error;
            error.setUrl(finalUrl());
            error.setLine(script.location.line);
            error.setColumn(script.location.column);
            error.setDescription(typeLoader()->tr("Script %1 unavailable").arg(script.script->url().toString()));
            errors.prepend(error);
            setError(errors);
        }
    }

    if (isError())
        return;

    QDeclarativeEngine *engine = typeLoader()->engine();
    m_scriptData = new QDeclarativeScriptData(engine);
    m_scriptData->url = finalUrl();
    m_scriptData->importCache = new QDeclarativeTypeNameCache(engine);

    for (int ii = 0; !isError() && ii < m_scripts.count(); ++ii) {
        const ScriptReference &script = m_scripts.at(ii);

        m_scriptData->scripts.append(script.script);
        m_scriptData->importCache->add(script.qualifier, ii);
    }

    m_imports.populateCache(m_scriptData->importCache, engine);

    m_scriptData->pragmas = m_pragmas;
    m_scriptData->m_program = QScriptProgram(m_source, finalUrl().toString());
}

QDeclarativeQmldirData::QDeclarativeQmldirData(const QUrl &url)
: QDeclarativeDataBlob(url, QmldirFile)
{
}

const QDeclarativeDirComponents &QDeclarativeQmldirData::dirComponents() const
{
    return m_components;
}

void QDeclarativeQmldirData::dataReceived(const QByteArray &data)
{
    QDeclarativeDirParser parser;
    parser.setSource(QString::fromUtf8(data));
    parser.parse();
    m_components = parser.components();
}

QT_END_NAMESPACE

