TEMPLATE = subdirs
!symbian: {
SUBDIRS += \
           qdeclarativemetatype \
           qmetaobjectbuilder
}

SUBDIRS += \
           examples \
           parserstress \
           qdeclarativecomponent \
           qdeclarativecontext \
           qdeclarativeengine \
           qdeclarativeerror \
           qdeclarativefolderlistmodel \
           qdeclarativeinfo \
           qdeclarativelayoutitem \
           qdeclarativelistreference \
           qdeclarativemoduleplugin \
           qdeclarativeparticles \
           qdeclarativepixmapcache \
           qdeclarativeqt \
           qdeclarativeview \
           qdeclarativeviewer \
           qdeclarativexmlhttprequest \
           qmlvisual \
           moduleqt47

SUBDIRS += \
           qsgitem \

contains(QT_CONFIG, private_tests) {
    SUBDIRS += \
           qdeclarativeanchors \
           qdeclarativeanimatedimage \
           qdeclarativeanimations \
           qdeclarativeapplication \
           qdeclarativebehaviors \
           qdeclarativebinding \
           qdeclarativeborderimage \
           qdeclarativeconnection \
           qdeclarativedebug \
           qdeclarativedebugclient \
           qdeclarativedebugservice \
           qdeclarativeecmascript \
           qdeclarativeflickable \
           qdeclarativeflipable \
           qdeclarativefocusscope \
           qdeclarativefontloader \
           qdeclarativegridview \
           qdeclarativeimage \
           qdeclarativeimageprovider \
           qdeclarativeinstruction \
           qdeclarativeitem \
           qdeclarativelanguage \
           qdeclarativelistmodel \
           qdeclarativelistview \
           qdeclarativeloader \
           qdeclarativemousearea \
           qdeclarativepathview \
           qdeclarativepincharea \
           qdeclarativepositioners \
           qdeclarativeproperty \
           qdeclarativepropertymap \
           qdeclarativerepeater \
           qdeclarativesmoothedanimation \
           qdeclarativespringanimation \
           qdeclarativestyledtext \
           qdeclarativesqldatabase \
           qdeclarativestates \
           qdeclarativesystempalette \
           qdeclarativetext \
           qdeclarativetextedit \
           qdeclarativetextinput \
           qdeclarativetimer \
           qdeclarativevaluetypes \
           qdeclarativevisualdatamodel \
           qdeclarativeworkerscript \
           qdeclarativexmllistmodel \
           qpacketprotocol \
           qdeclarativev4 \
           qsgcanvas \
           qsgflickable \
           qsgflipable \
           qsgfocusscope \
           qsggridview \
           qsglistview \
           qsgloader \
           qsgmousearea \
           qsgpathview \
           qsgpincharea \
           qsgpositioners \
           qsgrepeater \
           qsgtext \
           qsgtextedit \
           qsgtextinput \
           qsgvisualdatamodel

}

# Tests which should run in Pulse
PULSE_TESTS = $$SUBDIRS
