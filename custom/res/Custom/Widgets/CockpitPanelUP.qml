import QtQuick 2.12
import QtQuick.Layouts 1.11

import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0
import Custom.Widgets 1.0

Rectangle {
    id: root

    property var    vehicle: null
    property real   padding: ScreenTools.defaultFontPixelWidth
    property real   cellWidth: ScreenTools.defaultFontPixelWidth * 8
    property real   cellFontScale: 1.0
    // Adapt the dimension of the external dial to the UI scaling dimensions
    TextMetrics {
        id: referenceFontMetrics
        text: "X"
        font.family: ScreenTools.normalFontFamily
        font.pointSize: ScreenTools.platformFontPointSize
    }

    readonly property real referenceFontPixelWidth:
        Math.round(referenceFontMetrics.advanceWidth / 2.0) * 2

    property real instrumentDiameter:
        referenceFontPixelWidth * 25
        * ScreenTools.defaultFontPointSize
        / ScreenTools.platformFontPointSize

    readonly property var forwardRangeFact: vehicle ? vehicle.distanceSensors.rotationNone : null

    readonly property real forwardRange:
        forwardRangeFact && vehicle.distanceSensors.telemetryAvailable
        ? forwardRangeFact.rawValue
        : NaN

    // Formatting fact
    function formatFact(fact, decimals) {
        if (!fact || isNaN(Number(fact.value)))
            return "-"
        
        var units = fact.units ? " " + fact.units : ""
        return Number(fact.value).toFixed(decimals) + units
    }

    function headingText() {
        if (!vehicle || isNaN(vehicle.heading.rawValue)) {
            return "-"
        }

        return ("" + Math.round(vehicle.heading.rawValue)).slice(-3) + "°"
    }

    implicitWidth: padding * 2 + cockpitRow.implicitWidth
    implicitHeight: padding * 2 + cockpitRow.implicitHeight

    width: implicitWidth
    height: implicitHeight

    radius: ScreenTools.defaultFontPixelWidth
    color: Qt.rgba(
        qgcPal.windowShade.r,
        qgcPal.windowShade.g,
        qgcPal.windowShade.b,
        0.05)

    QGCPalette {
        id: qgcPal
        colorGroupEnabled: root.enabled
    }

    RowLayout {
        id: cockpitRow
        anchors.fill: parent
        anchors.margins: root.padding
        spacing: ScreenTools.defaultFontPixelWidth * 0.75

        GridLayout {
            id: readings
            columns: 2
            rowSpacing: ScreenTools.defaultFontPixelHeight * 0.6
            columnSpacing: ScreenTools.defaultFontPixelWidth * 0.85

            Layout.minimumWidth: root.cellWidth * 2 + columnSpacing
            Layout.preferredWidth: Layout.minimumWidth
            Layout.maximumWidth: Layout.minimumWidth
            Layout.alignment: Qt.AlignVCenter
            Layout.fillWidth: false

            CockpitCellUP {
                Layout.preferredWidth: root.cellWidth
                label: "ALT"
                value: root.formatFact(root.vehicle ? root.vehicle.altitudeRelative : null, 1)
                prominent: true
                colSpan: 2
                fontScale: root.cellFontScale
            }

            CockpitCellUP {
                Layout.preferredWidth: root.cellWidth
                label: "GS"
                value: root.formatFact(root.vehicle ? root.vehicle.groundSpeed : null, 1)
                prominent: true
                colSpan: 2
                fontScale: root.cellFontScale
            }

            CockpitCellUP {
                Layout.preferredWidth: root.cellWidth
                label: "V/S"
                value: root.formatFact(root.vehicle ? root.vehicle.climbRate : null, 1)
                prominent: !forwardRangeCell.visible
                colSpan: forwardRangeCell.visible ? 1 : 2
                fontScale: root.cellFontScale
            }

            // TODO: move this value near the toolbar
            /* CockpitCellUP {
                Layout.preferredWidth: root.cellWidth
                label: "HOME"
                value: root.formatFact(root.vehicle ? root.vehicle.distanceToHome : null, 0)
            } */

            CockpitCellUP {
                id: forwardRangeCell
                Layout.preferredWidth: root.cellWidth
                label: "FWD"
                value: isNaN(root.forwardRange) ? "—" : root.formatFact(root.forwardRangeFact, 1)
                valueColor: isNaN(root.forwardRange) ? qgcPal.text
                          : root.forwardRange < 5 ? qgcPal.colorRed
                          : root.forwardRange < 10 ? qgcPal.colorOrange
                          : qgcPal.text
                visible: value !== "—"
                fontScale: root.cellFontScale
            }
        }

        ColumnLayout {
            id: instrumentColumn

            Layout.minimumWidth: root.instrumentDiameter
            Layout.preferredWidth: root.instrumentDiameter
            Layout.maximumWidth: root.instrumentDiameter
            Layout.alignment: Qt.AlignVCenter
            Layout.fillWidth: false

            QGCLabel {
                text: root.headingText() 
                color: qgcPal.text
                font.family: ScreenTools.demiboldFontFamily
                font.pointSize: ScreenTools.mediumFontPointSize * root.cellFontScale
                Layout.alignment: Qt.AlignHCenter
                style: Text.Outline
                styleColor: qgcPal.window
            }

            Item {
                id: instrument
                Layout.preferredWidth: root.instrumentDiameter
                Layout.preferredHeight: root.instrumentDiameter

                CustomCompassWidget {
                    id: compass
                    anchors.fill: parent
                    size: root.instrumentDiameter
                    vehicle: root.vehicle
                    showCenterPointer: false
                    showHeadingReadout: true 
                }
                

                CustomAttitudeWidget {
                    id: attitude
                    anchors.centerIn: compass
                    size: root.instrumentDiameter * 0.54
                    vehicle: root.vehicle
                    showHeading: false
                    z: 1
                }

                Canvas {
                    id: headingMarker
                    width: 14
                    height: 10
                    visible: root.vehicle !== null 
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.topMargin: -height / 2

                    onPaint: {
                        var ctx = getContext("2d");
                        ctx.clearRect(0, 0, width, height);
                        ctx.fillStyle = qgcPal.text;
                        ctx.beginPath();
                        ctx.moveTo(0, 0);
                        ctx.lineTo(width, 0);
                        ctx.lineTo(width / 2, height);
                        ctx.closePath();
                        ctx.fill();
                    }
                }
            }
        }
    }
}
