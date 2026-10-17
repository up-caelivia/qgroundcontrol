import QtQuick 2.12

import QGroundControl 1.0
import QtQuick.Layouts 1.11

import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

ColumnLayout {
    id: root

    property string label: ""
    property string value: ""
    property color valueColor: qgcPal.text
    property bool prominent: false
    property int colSpan: 1

    spacing: 0
    Layout.columnSpan: root.colSpan
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignVCenter

    QGCPalette {
        id: qgcPal
        colorGroupEnabled: root.enabled
    }

    QGCLabel {
        text: root.label
        color: qgcPal.text
        opacity: 0.85
        font.family: ScreenTools.demiboldFontFamily
        font.pointSize: ScreenTools.smallFontPointSize
        style: Text.Outline
        styleColor: qgcPal.window
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
    }

    QGCLabel {
        text: root.value
        color: root.valueColor
        font.family: ScreenTools.demiboldFontFamily
        font.pointSize: root.prominent ? ScreenTools.largeFontPointSize : ScreenTools.mediumFontPointSize
        style: Text.Outline
        styleColor: qgcPal.window
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
    }
}
