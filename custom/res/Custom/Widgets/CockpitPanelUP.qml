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
    // The containing layer scales the complete cockpit group to fit the view.
    property real   instrumentDiameter: ScreenTools.defaultFontPixelWidth * 24

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

    implicitWidth: padding * 3 + cellWidth * 2 + ScreenTools.defaultFontPixelWidth * 0.75 + instrumentDiameter
    implicitHeight: padding * 2 + Math.max(instrumentColumn.implicitHeight, readings.implicitHeight)

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
        anchors.fill: parent
        anchors.margins: root.padding
        spacing: ScreenTools.defaultFontPixelWidth * 0.75

        GridLayout {
            id: readings
            columns: 2
            rowSpacing: ScreenTools.defaultFontPixelHeight * 0.6
            columnSpacing: ScreenTools.defaultFontPixelWidth * 0.85

            Layout.preferredWidth: root.cellWidth * 2 + columnSpacing
            Layout.alignment: Qt.AlignVCenter

            CockpitCellUP {
                Layout.preferredWidth: root.cellWidth
                label: "ALT"
                value: root.formatFact(root.vehicle ? root.vehicle.altitudeRelative : null, 1)
                prominent: true
                colSpan: 2
            }

            CockpitCellUP {
                Layout.preferredWidth: root.cellWidth
                label: "GS"
                value: root.formatFact(root.vehicle ? root.vehicle.groundSpeed : null, 1)
                prominent: true
                colSpan: 2
            }

            CockpitCellUP {
                Layout.preferredWidth: root.cellWidth
                label: "V/S"
                value: root.formatFact(root.vehicle ? root.vehicle.climbRate : null, 1)
                prominent: !forwardRangeCell.visible
                colSpan: forwardRangeCell.visible ? 1 : 2
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
            }
        }

        ColumnLayout {
            id: instrumentColumn

            Layout.preferredWidth: root.instrumentDiameter
            Layout.alignment: Qt.AlignVCenter

            QGCLabel {
                text: root.headingText() 
                color: qgcPal.text
                font.family: ScreenTools.demiboldFontFamily
                font.pointSize: ScreenTools.mediumFontPointSize
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
                    size: root.instrumentDiameter * 0.58
                    vehicle: root.vehicle
                    showHeading: false
                    z: 1
                }

                Canvas {
                    id: headingMarker
                    width: 12
                    height: 8
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
