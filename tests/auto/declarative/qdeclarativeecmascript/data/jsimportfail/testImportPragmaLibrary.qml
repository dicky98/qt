import QtQuick 1.0

import "importPragmaLibrary.js" as ImportPragmaLibrary

QtObject {
    id: testQtObject
    property int testValue: ImportPragmaLibrary.importValue()
}
