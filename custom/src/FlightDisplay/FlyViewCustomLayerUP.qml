/****************************************************************************
 *
 * (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 * @file
 *   @author Gus Grubba <gus@auterion.com>
 */

import QtQuick          2.12

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.FlightDisplay 1.0
import Custom.Widgets 1.0
import CustomAnnouncer 1.0
import Custom.Controllers 1.0

Item {
    id: root

    property var parentToolInsets                       // These insets tell you what screen real estate is available for positioning the controls in your overlay
    property var totalToolInsets: _totalToolInsets    // The insets updated for the custom overlay additions

    property var vehicle: null
    property real   _toolsMargin:           ScreenTools.defaultFontPixelWidth * 0.75

    readonly property real uiScale:
        ScreenTools.defaultFontPointSize / ScreenTools.platformFontPointSize

    readonly property real cockpitLeftClearance: Math.max(
        parentToolInsets.leftEdgeBottomInset,
        speedControl.visible ? speedControl.x + speedControl.width : 0
    )

    readonly property real cockpitRightClearance: parentToolInsets.rightEdgeBottomInset

    readonly property real cockpitBottomClearance: Math.max(
        parentToolInsets.bottomEdgeRightInset,
        parentToolInsets.bottomEdgeCenterInset
    )

    readonly property real cockpitAvailableWidth: Math.max(
        0, width - cockpitLeftClearance - cockpitRightClearance - 2 * _toolsMargin
    )

    readonly property real cockpitAvailableHeight: Math.max(
        0, height - parentToolInsets.topEdgeRightInset - cockpitBottomClearance - 2 * _toolsMargin
    )

    function showCriticalVehicleMessage(message) {
        mainWindow.closeCriticalVehicleMessage()
        mainWindow.showCriticalVehicleMessage(message)
    }

    Rectangle {

        property double value: root.vehicle ? (isNaN(root.vehicle.altitudeRelative.value) ? 0.0 : root.vehicle.altitudeRelative.value) : 0.0
        property bool above120: false

        onValueChanged: {

            if (root.vehicle == null || root.vehicle.altitudeRelative == null)
                return;

            if ((value > 120 && root.vehicle.altitudeRelative.units == "m") || ( value > 393.7 && root.vehicle.altitudeRelative.units == "ft")) {

                if(!above120) {
                    showCriticalVehicleMessage("WARNING : Above 120m")
                    console.log("INFO: Vehicle above 120m");
                    CustomAnnouncer.announceAltitude();
                }

                above120 = true

            } else {

                if(above120 && mainWindow.getCriticalVehicleMessage() == "WARNING : Above 120m") {
                    mainWindow.closeCriticalVehicleMessage()
                }

                above120 = false

            }
        }
    }

    QGCToolInsets {
        id:                     _totalToolInsets
        leftEdgeTopInset:       parentToolInsets.leftEdgeTopInset
        //leftEdgeCenterInset:    exampleRectangle.leftEdgeCenterInset
        leftEdgeCenterInset: parentToolInsets.leftEdgeCenterInset
        // leftEdgeBottomInset:    parentToolInsets.leftEdgeBottomInset
        leftEdgeBottomInset: Math.max(
                                    parentToolInsets.leftEdgeBottomInset,
                                    speedControl.visible ? speedControl.x + speedControl.width : 0
        )
        rightEdgeTopInset:      parentToolInsets.rightEdgeTopInset
        rightEdgeCenterInset:   parentToolInsets.rightEdgeCenterInset
        //rightEdgeBottomInset:   parent.width - compassBackground.x
        rightEdgeBottomInset:   Math.max(
            parentToolInsets.rightEdgeBottomInset,
            parent.width - cockpitGroup.x
        )
        topEdgeLeftInset:       parentToolInsets.topEdgeLeftInset
        //topEdgeCenterInset:     compassArrowIndicator.y + compassArrowIndicator.height
        topEdgeCenterInset: parentToolInsets.topEdgeCenterInset
        topEdgeRightInset:      parentToolInsets.topEdgeRightInset
        // bottomEdgeLeftInset:    parentToolInsets.bottomEdgeLeftInset
        bottomEdgeLeftInset: Math.max(
                                    parentToolInsets.bottomEdgeLeftInset,
                                    speedControl.visible ? parent.height - speedControl.y : 0
        )
        bottomEdgeCenterInset:  parentToolInsets.bottomEdgeCenterInset
        // bottomEdgeRightInset:   parent.height - attitudeIndicator.y
        bottomEdgeRightInset:   Math.max(
            parentToolInsets.bottomEdgeRightInset,
            parent.height - cockpitGroup.y
        )
    }

    SpeedSelectorController {
        id: speedController
        vehicle: root.vehicle
    }

    CustomSpeedSelector {
        id: speedControl
        anchors.left: parent.left
        anchors.leftMargin: _toolsMargin + parentToolInsets.leftEdgeBottomInset
        anchors.bottom: parent.bottom
        anchors.bottomMargin: _toolsMargin + parentToolInsets.bottomEdgeLeftInset
        controller: speedController
        visible: speedController.ready
    }

    Item {
        id: cockpitGroup

        readonly property real widthFraction: 0.38
        readonly property real heightFraction: 0.32
        readonly property real minimumCellFontScale: 1.3
        readonly property real maximumCellFontScale: 1.6
        readonly property real referenceViewHeight: 720

        readonly property real maximumWidth: root.width * widthFraction * root.uiScale
        readonly property real maximumHeight: root.height * heightFraction * root.uiScale
        readonly property real cellFontScale: Math.max(
            minimumCellFontScale,
            Math.min(
                maximumCellFontScale,
                referenceViewHeight / Math.max(1, root.height)
            )
        )

        readonly property real fitScale: width > 0 && height > 0
            ? Math.min(
                1,
                maximumWidth / width,
                maximumHeight / height,
                root.cockpitAvailableWidth / width,
                root.cockpitAvailableHeight / height
            )
            : 1

        width: cockpitPanel.width + (radarWidget.visible ? _toolsMargin + radarWidget.width : 0)
        height: Math.max(
            cockpitPanel.height,
            radarWidget.visible ? radarWidget.height + _toolsMargin : 0
        )
        scale: fitScale
        transformOrigin: Item.TopLeft
        x: root.width - root.cockpitRightClearance - _toolsMargin - width * fitScale
        y: root.height - root.cockpitBottomClearance - _toolsMargin - height * fitScale

        CockpitPanelUP {
            id: cockpitPanel
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            vehicle: root.vehicle
            cellFontScale: cockpitGroup.cellFontScale
        }

        CustomRadarWidget {
            id: radarWidget
            anchors.left: cockpitPanel.right
            anchors.leftMargin: _toolsMargin
            anchors.verticalCenter: cockpitPanel.verticalCenter
            anchors.bottomMargin: _toolsMargin
        }
    }

}
