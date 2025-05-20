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
        qgcPal.toolbarBackground = Qt.rgba(0, 0, 0, 0.7) // 50% trasparente nero
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

    ColumnLayout {
        anchors.fill: parent
        spacing: ScreenTools.defaultFontPixelHeight

        QGCLabel {
            text: "SAV"
            font.pointSize: ScreenTools.defaultFontPointSize * 3
            color: qgcPal.text
            horizontalAlignment: Text.AlignHCenter
            Layout.alignment: Qt.AlignHCenter
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: ScreenTools.defaultFontPixelWidth
            Layout.rightMargin: ScreenTools.defaultFontPixelWidth
            Layout.bottomMargin: ScreenTools.defaultFontPixelWidth
            columns: 5
            rowSpacing: ScreenTools.defaultFontPixelHeight
            columnSpacing: ScreenTools.defaultFontPixelHeight

            Repeater {
                model: ["1", "A", "2", "3", "4", "5", "6", "8", "10", "11"]

                delegate: ToolButton {
                    text: modelData
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 100
                    Layout.preferredHeight: 100

                    background: Rectangle {
                        color: Qt.rgba(1, 1, 1, 0.2)  // Bianco al 20% di opacità
                        radius: ScreenTools.defaultFontPixelHeight
                        border.color: Qt.rgba(1, 1, 1, 0.6)
                        border.width: 1
                    }

                    contentItem: Column {
                        anchors.fill: parent
                        spacing: 0

                        QGCColoredImage {
                            color: "white"
                            width: Math.min(parent.width, parent.height - textTorretta.height) * 0.5
                            height: width
                            mipmap: true
                            fillMode: Image.PreserveAspectFit
                            source: "/res/QGCLogoWhite" //"/qmlimages/MapAddMission.svg"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        Rectangle {
                            width: 1
                            height: ScreenTools.defaultFontPixelHeight * 0.5
                            color: "transparent"
                        }

                        QGCLabel {
                            id: textTorretta
                            text: "TORRETTA"
                            font.pointSize: ScreenTools.defaultFontPointSize
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        QGCLabel {
                            id: textId
                            text: modelData
                            font.pointSize: ScreenTools.defaultFontPointSize * 4
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }

                    onClicked: {
                        const coord = findCoordByLabel(modelData)
                        if (coord) {
                            console.log("Clicked:", modelData, coord.latitude, coord.longitude)
                        } else {
                            console.log("Clicked:", modelData)
                        }
                    }
                }
            }
        }
    }
}
