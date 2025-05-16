import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQml 2.15
import QGroundControl 1.0
import QGroundControl.Controls 1.0
import QGroundControl.ScreenTools 1.0

ToolStrip {
    id: savToolStrip
    title: "SAV"

    property var _savParamCoords: []

    Connections {
        target: CustomPlugin
        onSavParamCoordinatesChanged: {
            _savParamCoords = CustomPlugin.savParamCoordinates
        }
    }

    Component.onCompleted: {
        _savParamCoords = CustomPlugin.savParamCoordinates
    }

    function findCoordByLabel(label) {
        for (var i = 0; i < _savParamCoords.length; i++) {
            if (_savParamCoords[i].label === label) {
                return _savParamCoords[i]
            }
        }
        return null
    }

    ToolStripActionList {
        id: savToolStripActionList

        model: [
            ToolStripAction {
                //text: "A"
                iconSource: "/qmlimages/MapAddMission.svg"
                onTriggered: {
                    const c = _findSavCoord("A")
                    if (c) console.log("A →", c.latitude, c.longitude)
                }
            },
            ToolStripAction {
                //text: "B"
                iconSource: "/qmlimages/MapAddMission.svg"
                onTriggered: {
                    const c = _findSavCoord("B")
                    if (c) console.log("B →", c.latitude, c.longitude)
                }
            },
            ToolStripAction {
                //text: "C"
                iconSource: "/qmlimages/MapAddMission.svg"
                onTriggered: {
                    const c = _findSavCoord("C")
                    if (c) console.log("C →", c.latitude, c.longitude)
                }
            },
            ToolStripAction {
                text: "D"
                iconSource: "/qmlimages/MapAddMission.svg"
                onTriggered: {
                    const c = _findSavCoord("D")
                    if (c) console.log("D →", c.latitude, c.longitude)
                }
            },
            ToolStripAction {
                text: "E"
                iconSource: "/qmlimages/MapAddMission.svg"
                onTriggered: {
                    const c = _findSavCoord("E")
                    if (c) console.log("E →", c.latitude, c.longitude)
                }
            },
            ToolStripAction {
                text: "F"
                iconSource: "/qmlimages/MapAddMission.svg"
                onTriggered: {
                    const c = _findSavCoord("F")
                    if (c) console.log("F →", c.latitude, c.longitude)
                }
            },
            ToolStripAction {
                text: "G"
                iconSource: "/qmlimages/MapAddMission.svg"
                onTriggered: {
                    const c = _findSavCoord("G")
                    if (c) console.log("G →", c.latitude, c.longitude)
                }
            },
            ToolStripAction {
                text: "H"
                iconSource: "/qmlimages/MapAddMission.svg"
                onTriggered: {
                    const c = _findSavCoord("H")
                    if (c) console.log("H →", c.latitude, c.longitude)
                }
            }
        ]
    }
    model: savToolStripActionList.model

}
