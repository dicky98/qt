/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <qtest.h>
#include <QDeclarativeEngine>
#include <QDeclarativeComponent>
#include <QDeclarativeProperty>
#include <QFile>
#include <QDebug>

//TESTED_FILES=

#ifdef Q_OS_SYMBIAN
// In Symbian OS test data is located in applications private dir
#define SRCDIR "."
#endif

class tst_qmlmetaproperty : public QObject
{
    Q_OBJECT

public:
    tst_qmlmetaproperty();
    virtual ~tst_qmlmetaproperty();

public slots:
    void init();
    void cleanup();

private slots:
    void lookup_data();
    void lookup();

private:
    QDeclarativeEngine engine;
};

tst_qmlmetaproperty::tst_qmlmetaproperty()
{
}

tst_qmlmetaproperty::~tst_qmlmetaproperty()
{
}

void tst_qmlmetaproperty::init()
{
}

void tst_qmlmetaproperty::cleanup()
{
}

void tst_qmlmetaproperty::lookup_data()
{
    QTest::addColumn<QString>("file");

    QTest::newRow("Simple Object") << SRCDIR "/data/object.qml";
    QTest::newRow("Synthesized Object") << SRCDIR "/data/synthesized_object.qml";
}

void tst_qmlmetaproperty::lookup()
{
    QFETCH(QString, file);

    QDeclarativeComponent c(&engine, file);
    QVERIFY(c.isReady());

    QObject *obj = c.create();

    QBENCHMARK {
        QDeclarativeProperty p(obj, "x");
    }

    delete obj;
}

QTEST_MAIN(tst_qmlmetaproperty)
#include "tst_qdeclarativemetaproperty.moc"
