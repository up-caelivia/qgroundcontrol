import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQml 2.15
import QGroundControl 1.0
import QGroundControl.Controls 1.0
import QGroundControl.ScreenTools 1.0
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15

import Constants 1.0

ToolStrip {
    id: savToolStrip
    width: Screen.width * 0.6
    height: Screen.height * 0.4

    property var _savParamCoords: []
    property var _labels: []

    Connections {
        target: CustomPlugin
        onSavParamCoordinatesChanged: {
            _savParamCoords = CustomPlugin.savParamCoordinates
            updateLabels()
        }
    }

    Component.onCompleted: {
        qgcPal.toolbarBackground = Qt.rgba(0, 0, 0, 0.7) // 50% trasparente nero
        _savParamCoords = CustomPlugin.savParamCoordinates
        updateLabels()
    }

    function findCoordByLabel(label) {
        for (var i = 0; i < _savParamCoords.length; i++) {
            if (_savParamCoords[i].label === label) {
                return _savParamCoords[i]
            }
        }
        return null
    }

    function updateLabels() {
        const excluded = ["SG", "MAR", "HOM"]
        _labels = _savParamCoords
            .filter(coord => !excluded.includes(coord.label))
            .map(coord => coord.label)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: ScreenTools.defaultFontPixelHeight

        QGCLabel {
            text: "SAV points"
            font.pointSize: ScreenTools.isMobile ? ScreenTools.defaultFontPointSize *1.5 : ScreenTools.defaultFontPointSize * 3
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
            columns: Math.ceil(_labels.length/2)
            rowSpacing: ScreenTools.defaultFontPixelHeight
            columnSpacing: ScreenTools.defaultFontPixelHeight

            Repeater {
                model: _labels

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
                            visible: !ScreenTools.isMobile
                        }

                        Rectangle {
                            width: 1
                            height: ScreenTools.isMobile ? ScreenTools.defaultFontPixelHeight * 0.2 : ScreenTools.defaultFontPixelHeight * 0.5
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
                            font.pointSize: ScreenTools.isMobile ? ScreenTools.defaultFontPointSize *1.5 : ScreenTools.defaultFontPointSize * 4
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }

                    onClicked: {
                        const coord = findCoordByLabel(modelData)
                        CustomPlugin.savButtonPressed(modelData)
                        Constants.showSavButtons = false
                    }
                }
            }
        }
    }
}
