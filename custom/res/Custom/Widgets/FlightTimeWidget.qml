/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.11
import QtQuick.Layouts  1.11

import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

import Custom.Controllers 1.0

Rectangle {
    id: _root

    property var vehicle: null
    property double flightTimeSeconds: NaN

    readonly property real _padding: ScreenTools.defaultFontPixelWidth
    readonly property real _sectionWidth: Math.max(
        timeMetrics.advanceWidth,
        flightTimeLabel.implicitWidth,
        flightTimeValue.implicitWidth,
        remainingTimeLabel.implicitWidth,
        remainingTimeValue.implicitWidth
    )

    implicitWidth: contentRow.implicitWidth + 2 * _padding
    implicitHeight: contentRow.implicitHeight + 2 * _padding

    radius: ScreenTools.defaultFontPixelWidth / 2
    color: Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.65)

    QGCPalette {
        id: qgcPal
    }

    BatteryTimeController {
        id: batteryTimeController
        vehicle: _root.vehicle
    }

    TextMetrics {
        id: timeMetrics
        font.family: ScreenTools.fixedFontFamily
        font.pointSize: ScreenTools.defaultFontPointSize
        text: "00:00:00"
    }

    RowLayout {
        id: contentRow

        anchors.fill: parent
        anchors.margins: _root._padding
        spacing: ScreenTools.defaultFontPixelWidth

        ColumnLayout {
            Layout.alignment: Qt.AlignVCenter
            Layout.minimumWidth: _root._sectionWidth
            Layout.preferredWidth: _root._sectionWidth
            Layout.fillWidth: true
            spacing: ScreenTools.defaultFontPixelHeight / 4

            QGCLabel {
                id: flightTimeLabel

                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter

                text: qsTr("Flight time")
                font.pointSize: ScreenTools.smallFontPointSize
                color: qgcPal.text
            }

            QGCLabel {
                id: flightTimeValue

                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter

                text: _root._formatDuration(_root.flightTimeSeconds)
                font.family: ScreenTools.fixedFontFamily
                color: qgcPal.text
            }
        }

        QGCColoredImage {
            Layout.preferredWidth: ScreenTools.defaultFontPixelHeight * 1.5
            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5

            source: "/InstrumentValueIcons/timer.svg"
            fillMode: Image.PreserveAspectFit
            color: qgcPal.text
        }

        ColumnLayout {
            Layout.alignment: Qt.AlignVCenter
            Layout.minimumWidth: _root._sectionWidth
            Layout.preferredWidth: _root._sectionWidth
            Layout.fillWidth: true
            spacing: ScreenTools.defaultFontPixelHeight / 4

            QGCLabel {
                id: remainingTimeLabel

                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter

                text: qsTr("Time remaining")
                font.pointSize: ScreenTools.smallFontPointSize
                color: qgcPal.text
            }

            QGCLabel {
                id: remainingTimeValue

                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter

                text: batteryTimeController.remainingTimeAvailable
                      ? _root._formatDuration(batteryTimeController.remainingSeconds)
                      : "--:--:--"
                font.family: ScreenTools.fixedFontFamily
                color: _root._statusColor(batteryTimeController.status)
            }
        }
    }

    function _formatDuration(seconds) {
        if (!isFinite(seconds) || seconds < 0) {
            return "--:--:--"
        }

        var totalSeconds = Math.floor(seconds)
        var hours = Math.floor(totalSeconds / 3600)
        var minutes = Math.floor(totalSeconds / 60) % 60
        var remainingSeconds = totalSeconds % 60

        return (hours < 10 ? "0" + hours : hours)
                + ":" + ("0" + minutes).slice(-2)
                + ":" + ("0" + remainingSeconds).slice(-2)
    }

    function _statusColor(status) {
        switch (status) {
        case BatteryTimeController.Low:
            return qgcPal.colorOrange
        case BatteryTimeController.Critical:
            return qgcPal.colorRed
        default:
            return qgcPal.text
        }
    }
}
