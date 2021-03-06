/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import Qt.labs.particles 2.0

Rectangle{
    color: "black"
    width: 360
    height: 540
    ParticleSystem{ id: sys }
    ColoredParticle{
        system: sys
        id: cp
        image: "content/particle.png"
        color: "#00FFFFFF"
        colorVariation: 0.4
    }
    TrailEmitter{
    //burst on click
        id: bursty
        system: sys
        emitting: false
        particlesPerSecond: 2000
        particleDuration: 500
        acceleration: AngleVector{ angle: 90; angleVariation: 360; magnitude: 640; }
        particleSize: 8
        particleEndSize: 16
        particleSizeVariation: 4
    }
    TrailEmitter{
        system: sys
        speedFromMovement: 4.0
        emitting: ma.pressed
        x: ma.mouseX
        y: ma.mouseY
        particlesPerSecond: 400
        particleDuration: 2000
        acceleration: AngleVector{ angle: 90; angleVariation: 22; magnitude: 32; }
        particleSize: 8
        particleEndSize: 16
        particleSizeVariation: 8
    }
    MouseArea{
        id: ma
        anchors.fill: parent
        onPressed: {bursty.x = mouse.x; bursty.y = mouse.y; bursty.pulse(0.1);}//uses both for comparison
        onReleased: {bursty.x = mouse.x; bursty.y = mouse.y; bursty.burst(200);}
    }
}
