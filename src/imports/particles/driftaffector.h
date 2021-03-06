/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Declarative module of the Qt Toolkit.
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

#ifndef DRIFTAFFECTOR_H
#define DRIFTAFFECTOR_H
#include "particleaffector.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)


class DriftAffector : public ParticleAffector
{
    Q_OBJECT
    Q_PROPERTY(qreal xDrift READ xDrift WRITE setXDrift NOTIFY xDriftChanged)
    Q_PROPERTY(qreal yDrift READ yDrift WRITE setYDrift NOTIFY yDriftChanged)
public:
    explicit DriftAffector(QSGItem *parent = 0);
    ~DriftAffector();
    qreal yDrift() const
    {
        return m_yDrift;
    }

    qreal xDrift() const
    {
        return m_xDrift;
    }
protected:
    virtual bool affectParticle(ParticleData *d, qreal dt);

signals:

    void yDriftChanged(qreal arg);

    void xDriftChanged(qreal arg);

public slots:

void setYDrift(qreal arg)
{
    if (m_yDrift != arg) {
        m_yDrift = arg;
        emit yDriftChanged(arg);
    }
}

void setXDrift(qreal arg)
{
    if (m_xDrift != arg) {
        m_xDrift = arg;
        emit xDriftChanged(arg);
    }
}

private:
    qreal m_yDrift;
    qreal m_xDrift;
};

QT_END_NAMESPACE
QT_END_HEADER
#endif // DRIFTAFFECTOR_H
