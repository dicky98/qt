// Commit: 1bcddaaf318fc37c71c5191913f3487c49444ec6
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

#ifndef QSGFLICKABLE_P_H
#define QSGFLICKABLE_P_H

#include "qsgitem.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QSGFlickablePrivate;
class QSGFlickableVisibleArea;
class Q_AUTOTEST_EXPORT QSGFlickable : public QSGItem
{
    Q_OBJECT

    Q_PROPERTY(qreal contentWidth READ contentWidth WRITE setContentWidth NOTIFY contentWidthChanged)
    Q_PROPERTY(qreal contentHeight READ contentHeight WRITE setContentHeight NOTIFY contentHeightChanged)
    Q_PROPERTY(qreal contentX READ contentX WRITE setContentX NOTIFY contentXChanged)
    Q_PROPERTY(qreal contentY READ contentY WRITE setContentY NOTIFY contentYChanged)
    Q_PROPERTY(QSGItem *contentItem READ contentItem CONSTANT)

    Q_PROPERTY(qreal horizontalVelocity READ horizontalVelocity NOTIFY horizontalVelocityChanged)
    Q_PROPERTY(qreal verticalVelocity READ verticalVelocity NOTIFY verticalVelocityChanged)

    Q_PROPERTY(BoundsBehavior boundsBehavior READ boundsBehavior WRITE setBoundsBehavior NOTIFY boundsBehaviorChanged)
    Q_PROPERTY(qreal maximumFlickVelocity READ maximumFlickVelocity WRITE setMaximumFlickVelocity NOTIFY maximumFlickVelocityChanged)
    Q_PROPERTY(qreal flickDeceleration READ flickDeceleration WRITE setFlickDeceleration NOTIFY flickDecelerationChanged)
    Q_PROPERTY(bool moving READ isMoving NOTIFY movingChanged)
    Q_PROPERTY(bool movingHorizontally READ isMovingHorizontally NOTIFY movingHorizontallyChanged)
    Q_PROPERTY(bool movingVertically READ isMovingVertically NOTIFY movingVerticallyChanged)
    Q_PROPERTY(bool flicking READ isFlicking NOTIFY flickingChanged)
    Q_PROPERTY(bool flickingHorizontally READ isFlickingHorizontally NOTIFY flickingHorizontallyChanged)
    Q_PROPERTY(bool flickingVertically READ isFlickingVertically NOTIFY flickingVerticallyChanged)
    Q_PROPERTY(FlickableDirection flickableDirection READ flickableDirection WRITE setFlickableDirection NOTIFY flickableDirectionChanged)

    Q_PROPERTY(bool interactive READ isInteractive WRITE setInteractive NOTIFY interactiveChanged)
    Q_PROPERTY(int pressDelay READ pressDelay WRITE setPressDelay NOTIFY pressDelayChanged)

    Q_PROPERTY(bool atXEnd READ isAtXEnd NOTIFY isAtBoundaryChanged)
    Q_PROPERTY(bool atYEnd READ isAtYEnd NOTIFY isAtBoundaryChanged)
    Q_PROPERTY(bool atXBeginning READ isAtXBeginning NOTIFY isAtBoundaryChanged)
    Q_PROPERTY(bool atYBeginning READ isAtYBeginning NOTIFY isAtBoundaryChanged)

    Q_PROPERTY(QSGFlickableVisibleArea *visibleArea READ visibleArea CONSTANT)

    Q_PROPERTY(QDeclarativeListProperty<QObject> flickableData READ flickableData)
    Q_PROPERTY(QDeclarativeListProperty<QSGItem> flickableChildren READ flickableChildren)
    Q_CLASSINFO("DefaultProperty", "flickableData")

    Q_ENUMS(FlickableDirection)
    Q_ENUMS(BoundsBehavior)

public:
    QSGFlickable(QSGItem *parent=0);
    ~QSGFlickable();

    QDeclarativeListProperty<QObject> flickableData();
    QDeclarativeListProperty<QSGItem> flickableChildren();

    enum BoundsBehavior { StopAtBounds, DragOverBounds, DragAndOvershootBounds };
    BoundsBehavior boundsBehavior() const;
    void setBoundsBehavior(BoundsBehavior);

    qreal contentWidth() const;
    void setContentWidth(qreal);

    qreal contentHeight() const;
    void setContentHeight(qreal);

    qreal contentX() const;
    virtual void setContentX(qreal pos);

    qreal contentY() const;
    virtual void setContentY(qreal pos);

    bool isMoving() const;
    bool isMovingHorizontally() const;
    bool isMovingVertically() const;
    bool isFlicking() const;
    bool isFlickingHorizontally() const;
    bool isFlickingVertically() const;

    int pressDelay() const;
    void setPressDelay(int delay);

    qreal maximumFlickVelocity() const;
    void setMaximumFlickVelocity(qreal);

    qreal flickDeceleration() const;
    void setFlickDeceleration(qreal);

    bool isInteractive() const;
    void setInteractive(bool);

    qreal horizontalVelocity() const;
    qreal verticalVelocity() const;

    bool isAtXEnd() const;
    bool isAtXBeginning() const;
    bool isAtYEnd() const;
    bool isAtYBeginning() const;

    QSGItem *contentItem();

    enum FlickableDirection { AutoFlickDirection=0x00, HorizontalFlick=0x01, VerticalFlick=0x02, HorizontalAndVerticalFlick=0x03 };
    FlickableDirection flickableDirection() const;
    void setFlickableDirection(FlickableDirection);

    Q_INVOKABLE void resizeContent(qreal w, qreal h, QPointF center);
    Q_INVOKABLE void returnToBounds();

Q_SIGNALS:
    void contentWidthChanged();
    void contentHeightChanged();
    void contentXChanged();
    void contentYChanged();
    void movingChanged();
    void movingHorizontallyChanged();
    void movingVerticallyChanged();
    void flickingChanged();
    void flickingHorizontallyChanged();
    void flickingVerticallyChanged();
    void horizontalVelocityChanged();
    void verticalVelocityChanged();
    void isAtBoundaryChanged();
    void flickableDirectionChanged();
    void interactiveChanged();
    void boundsBehaviorChanged();
    void maximumFlickVelocityChanged();
    void flickDecelerationChanged();
    void pressDelayChanged();
    void movementStarted();
    void movementEnded();
    void flickStarted();
    void flickEnded();

protected:
    virtual bool childMouseEventFilter(QSGItem *, QEvent *);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    virtual void wheelEvent(QGraphicsSceneWheelEvent *event);
    virtual void timerEvent(QTimerEvent *event);

    QSGFlickableVisibleArea *visibleArea();

protected Q_SLOTS:
    virtual void ticked();
    void movementStarting();
    void movementEnding();

protected:
    void movementXEnding();
    void movementYEnding();
    virtual qreal minXExtent() const;
    virtual qreal minYExtent() const;
    virtual qreal maxXExtent() const;
    virtual qreal maxYExtent() const;
    qreal vWidth() const;
    qreal vHeight() const;
    virtual void viewportMoved();
    virtual void geometryChanged(const QRectF &newGeometry,
                                 const QRectF &oldGeometry);
    void mouseUngrabEvent();
    bool sendMouseEvent(QGraphicsSceneMouseEvent *event);

    bool xflick() const;
    bool yflick() const;
    void cancelFlick();

protected:
    QSGFlickable(QSGFlickablePrivate &dd, QSGItem *parent);

private:
    Q_DISABLE_COPY(QSGFlickable)
    Q_DECLARE_PRIVATE(QSGFlickable)
    friend class QSGFlickableVisibleArea;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QSGFlickable)

QT_END_HEADER

#endif // QSGFLICKABLE_P_H
