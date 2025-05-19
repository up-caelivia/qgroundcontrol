import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQml 2.15
import QGroundControl 1.0
import QGroundControl.Controls 1.0
import QGroundControl.ScreenTools 1.0
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15

ToolStrip {
    id: savToolStrip
    width: Screen.width * 0.6
    height: Screen.height * 0.4

    property var _savParamCoords: []

    Connections {
        target: CustomPlugin
        onSavParamCoordinatesChanged: {
            _savParamCoords = CustomPlugin.savParamCoordinates
        }
    }

    Component.onCompleted: {
        qgcPal.toolbarBackground = Qt.rgba(0, 0, 0, 0.5) // 50% trasparente nero
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

    Column {
        width: parent.width
        height: parent.height

        QGCLabel {
            text: "SAV"
            font.pointSize: ScreenTools.defaultFontPointSize * 1.2
            color: qgcPal.text
            horizontalAlignment: Text.AlignHCenter
            anchors.horizontalCenter: parent.horizontalCenter
            //height: implicitHeight + ScreenTools.defaultFontPixelHeight
        }

        GridLayout {
            anchors.fill: parent
            columns: 4
            rowSpacing: 4
            columnSpacing: 4

            Repeater {
                model: ["A", "B", "C", "D", "E", "F", "G", "H"]

                delegate: ToolButton {
                    text: modelData
                    icon.source: "/qmlimages/MapAddMission.svg"
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    background: Item {} 
                    contentItem: Column {
                        anchors.fill: parent
                        spacing: ScreenTools.defaultFontPixelWidth

                        QGCColoredImage {
                            color: "white"
                            width: Math.min(parent.width, parent.height - textId.height)  * 0.9
                            height: width
                            mipmap:                     true
                            fillMode: Image.PreserveAspectFit
                            source: "/qmlimages/MapAddMission.svg"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }     

                        QGCLabel {
                            id: textId
                            text: modelData
                            font.pointSize: ScreenTools.defaultFontPointSize
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }

                    onClicked: {
                        console.log("Clicked:", modelData)
                    }
                }
            }
        }
    }
/*
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
*/
}
