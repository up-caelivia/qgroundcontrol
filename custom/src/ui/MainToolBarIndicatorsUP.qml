/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick 2.12

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0
import Custom.Widgets               1.0

//-------------------------------------------------------------------------
//-- Toolbar Indicators
Item {
    id: indicatorArea
    implicitWidth:  leftIndicatorRow.width +
                    rightIndicatorRow.width + 
                    (2 * _toolIndicatorMargins) +
                    ScreenTools.defaultFontPixelWidth * 1.5

    property var  _activeVehicle:           QGroundControl.multiVehicleManager.activeVehicle
    property real _toolIndicatorMargins:    ScreenTools.defaultFontPixelHeight * 0.66
    property var _indicators:               _activeVehicle ? _activeVehicle.toolIndicators : []

    // Variable defined in order to split the list on a specified index (assume that the developer knows where he want to split it)
    // TODO: find a better solution
    readonly property int _splitIndex:      1
    property var _leftIndicators: _indicators.slice(0, _splitIndex)
    property var _rightIndicators: _indicators.slice(_splitIndex)


    QGCPalette { id: qgcPal }

    function dropMessageIndicatorTool() {
        var repeaters = [
            leftToolIndicatorsRepeater,
            rightToolIndicatorsRepeater
        ]

        for (var i = 0; i < repeaters.length; i++) {
            var repeater = repeaters[i]
            for (var j = 0; j < repeater.count; j++) {
                var loader = repeater.itemAt(j)
                var indicator = loader ? loader.item : null
                if (indicator && typeof indicator.dropMessageIndicator === "function") {
                    indicator.dropMessageIndicator()
                    return
                }
            }
        }
    }

    // Reusable component
    Component {
        id: indicatorDelegate

        Loader {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            source: modelData
            visible: item.showIndicator

            Rectangle {
                anchors.fill: parent
                anchors.margins: -(ScreenTools.defaultFontPixelWidth * 0.35)

                radius: ScreenTools.defaultFontPixelHeight * 0.35
                z: -1

                color: hoverArea.containsMouse
                        ? Qt.rgba(
                            qgcPal.text.r,
                            qgcPal.text.g,
                            qgcPal.text.b,
                            0.1
                        ) : "transparent"
                Behavior on color {
                    ColorAnimation { duration: 100 }
                }
            }

            MouseArea {
                id: hoverArea
                anchors.fill: parent
                z: 2

                hoverEnabled: !ScreenTools.isMobile
                acceptedButtons: Qt.NoButton
            }
        }
    }

    // Left side indicators
    Row {
        id:                 leftIndicatorRow
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        anchors.left:      parent.left
        anchors.margins:    _toolIndicatorMargins
        spacing:            ScreenTools.defaultFontPixelWidth * 1.5

        Repeater {
            id:     appRepeater
            model:  QGroundControl.corePlugin.toolBarIndicators
            Loader {
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom
                source:             modelData
                visible:            item.showIndicator
            }
        }

        Repeater {
            id:     leftToolIndicatorsRepeater
            model: _leftIndicators
            delegate: indicatorDelegate
        }

        Repeater {
            model: _activeVehicle ? _activeVehicle.modeIndicators : []
            Loader {
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom
                source:             modelData
                visible:            item.showIndicator
            }
        }


        // TODO: remove from here
        FlightTimeWidget {
            id: tf
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            visible:            _activeVehicle
        }
    }

    // Right side indicators
    Row {
        id: rightIndicatorRow
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        anchors.right:      parent.right
        anchors.margins:    _toolIndicatorMargins
        spacing:            ScreenTools.defaultFontPixelWidth * 1.5

        Repeater {
            id:     rightToolIndicatorsRepeater
            model:  _rightIndicators
            delegate: indicatorDelegate
        }
    }
}

