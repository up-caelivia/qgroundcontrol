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
    // anchors.fill: parent
    implicitWidth: indicatorRow.width + (2 * _toolIndicatorMargins)

    property var  _activeVehicle:           QGroundControl.multiVehicleManager.activeVehicle
    property real _toolIndicatorMargins:    ScreenTools.defaultFontPixelHeight * 0.66

    QGCPalette { id: qgcPal }

    function dropMessageIndicatorTool() {
        toolIndicatorsRepeater.dropMessageIndicatorTool();
    }

    Row {
        id:                 indicatorRow
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        anchors.right:      parent.right
        anchors.margins:    _toolIndicatorMargins
        spacing:            ScreenTools.defaultFontPixelWidth * 1.5

        // TODO: remove from here
        FlightTimeWidget {
            id: tf
            //anchors.right:       mainIndicator.left
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            // anchors.leftMargin: 10
            //anchors.margins: ScreenTools.defaultFontPixelHeight * 0.66
            visible:            _activeVehicle
        }

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
            id:     toolIndicatorsRepeater
            model:  _activeVehicle ? _activeVehicle.toolIndicators : []

            function dropMessageIndicatorTool() {
                for (var i=0; i<count; i++) {
                    var thisTool = itemAt(i);
                    if (thisTool.item.dropMessageIndicator) {
                        thisTool.item.dropMessageIndicator();
                    }
                }
            }

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

        Repeater {
            model: _activeVehicle ? _activeVehicle.modeIndicators : []
            Loader {
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom
                source:             modelData
                visible:            item.showIndicator
            }
        }
    }
}

