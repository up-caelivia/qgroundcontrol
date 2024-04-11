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
import QtQuick.Controls 2.4
import QtQuick.Layouts  1.11

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.FlightDisplay 1.0
import Custom.Widgets 1.0

Item {
    property var parentToolInsets                       // These insets tell you what screen real estate is available for positioning the controls in your overlay
    property var totalToolInsets:   _totalToolInsets    // The insets updated for the custom overlay additions
    property var mapControl

    readonly property string noGPS:         qsTr("NO GPS")
    readonly property real   indicatorValueWidth:   ScreenTools.defaultFontPixelWidth * 7

    property var    _activeVehicle:         QGroundControl.multiVehicleManager.activeVehicle
    property real   _indicatorDiameter:     ScreenTools.defaultFontPixelWidth * 18
    property real   _indicatorsHeight:      ScreenTools.defaultFontPixelHeight
    property var    _sepColor:              qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.5) : Qt.rgba(1,1,1,0.5)
    property color  _indicatorsColor:       qgcPal.text
    property bool   _isVehicleGps:          _activeVehicle ? _activeVehicle.gps.count.rawValue > 1 && _activeVehicle.gps.hdop.rawValue < 1.4 : false
    property string _altitude:              _activeVehicle ? (isNaN(_activeVehicle.altitudeRelative.value) ? "0.0" : _activeVehicle.altitudeRelative.value.toFixed(1)) + ' ' + _activeVehicle.altitudeRelative.units : "0.0"
    property string _distanceStr:           isNaN(_distance) ? "0" : _distance.toFixed(0) + ' ' + QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString
    property real   _heading:               _activeVehicle   ? _activeVehicle.heading.rawValue : 0
    property real   _distance:              _activeVehicle ? _activeVehicle.distanceToHome.rawValue : 0
    property string _messageTitle:          ""
    property string _messageText:           ""
    property real   _toolsMargin:           ScreenTools.defaultFontPixelWidth * 0.75


    QGCToolInsets {
        id:                     _totalToolInsets
        leftEdgeTopInset:       parentToolInsets.leftEdgeTopInset
        //leftEdgeCenterInset:    exampleRectangle.leftEdgeCenterInset
        leftEdgeBottomInset:    parentToolInsets.leftEdgeBottomInset
        rightEdgeTopInset:      parentToolInsets.rightEdgeTopInset
        rightEdgeCenterInset:   parentToolInsets.rightEdgeCenterInset
        //rightEdgeBottomInset:   parent.width - compassBackground.x
        topEdgeLeftInset:       parentToolInsets.topEdgeLeftInset
        //topEdgeCenterInset:     compassArrowIndicator.y + compassArrowIndicator.height
        topEdgeRightInset:      parentToolInsets.topEdgeRightInset
        bottomEdgeLeftInset:    parentToolInsets.bottomEdgeLeftInset
        bottomEdgeCenterInset:  parentToolInsets.bottomEdgeCenterInset
        bottomEdgeRightInset:   parent.height - attitudeIndicator.y
    }

    Rectangle {
        id:                     fig1
        anchors.verticalCenter: attitudeIndicator.verticalCenter
        anchors.right:          attitudeIndicator.left
        anchors.rightMargin:    -attitudeIndicator.width / 3
        width: 240
        height:                 attitudeIndicator.height * 0.85
        radius:                 10
        color:                  qgcPal.window

        GridLayout {
                    id:                     vehicleStatusGrid1
                    columnSpacing:          ScreenTools.defaultFontPixelWidth  * 2
                    rowSpacing:             ScreenTools.defaultFontPixelHeight * 0.5
                    columns:                2

            property real   _indicatorsHeight:      ScreenTools.defaultFontPixelHeight *2
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 12

            QGCColoredImage {
               height:                 _indicatorsHeight
               width:                  height
               source:                 "/custom/img/altitude.svg"
               fillMode:               Image.PreserveAspectFit
               sourceSize.height:      height
               Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
               color:                  qgcPal.text

                       }

           QGCLabel {
               text:                   _altitude
               color:                  _indicatorsColor
               font.pointSize:         ScreenTools.mediumFontPointSize
               Layout.fillWidth:       true
               Layout.minimumWidth:    indicatorValueWidth
           }

           QGCColoredImage {
               height:                 _indicatorsHeight
               width:                  height
               source:                   "/custom/img/distance.svg"
               fillMode:               Image.PreserveAspectFit
               sourceSize.height:      height
               Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
               color:                  qgcPal.text

           }

           QGCLabel {
               text:                   _activeVehicle ? ('0000' + _activeVehicle.distanceToHome.value.toFixed(0)).slice(-5) + ' ' + _activeVehicle.distanceToHome.units : "0000"
               color:                  _indicatorsColor
               font.pointSize:         ScreenTools.mediumFontPointSize
               Layout.fillWidth:       true
               Layout.minimumWidth:    indicatorValueWidth
           }

        }


    }

    Rectangle {
        id:                     fig2
        anchors.verticalCenter: attitudeIndicator.verticalCenter
        anchors.left:          attitudeIndicator.right
        anchors.leftMargin:    -attitudeIndicator.width / 3
        width: 240
        height:                 attitudeIndicator.height * 0.85
        radius:                 10
        color:                  qgcPal.window


        GridLayout {
                    id:                     vehicleStatusGrid2
                    columnSpacing:          ScreenTools.defaultFontPixelWidth  * 2
                    rowSpacing:             ScreenTools.defaultFontPixelHeight * 0.5
                    columns:                2

            property real   _indicatorsHeight:      ScreenTools.defaultFontPixelHeight *2
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 12

            QGCColoredImage {
                height:                 _indicatorsHeight
                width:                  height
                source:                 "/custom/img/horizontal_speed.svg"
                fillMode:               Image.PreserveAspectFit
                sourceSize.height:      height
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
                color:                  qgcPal.text
            }

            QGCLabel {
                text:                   _activeVehicle ? _activeVehicle.groundSpeed.value.toFixed(1) + ' ' + _activeVehicle.groundSpeed.units : "0.0"
                color:                  _indicatorsColor
                font.pointSize:         ScreenTools.mediumFontPointSize
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
            }

            QGCColoredImage {
                height:                 _indicatorsHeight
                width:                  height
                source:                 "/custom/img/vertical_speed.svg"
                fillMode:               Image.PreserveAspectFit
                sourceSize.height:      height
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
                color:                  qgcPal.text
            }

            QGCLabel {
                text:                   _activeVehicle ? _activeVehicle.climbRate.value.toFixed(1) + ' ' + _activeVehicle.climbRate.units : " 0.0"
                color:                  _indicatorsColor
                font.pointSize:         ScreenTools.mediumFontPointSize
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
            }


        }

    }

    Rectangle {
        id:                     attitudeIndicator
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin:   _toolsMargin + parentToolInsets.bottomEdgeRightInset
        anchors.rightMargin:    _toolsMargin
        anchors.bottom:         parent.bottom
        height:                 ScreenTools.defaultFontPixelHeight * 10
        width:                  height * 1.98
        radius:                 height * 0.5
        color:                  qgcPal.windowShade

        CustomAttitudeWidget {
            id:                 attitude
            anchors.leftMargin: 10
            size:               parent.height * 0.90
            vehicle:            _activeVehicle
            showHeading:        false
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
        }

        CustomCompassWidget {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 10
            size: parent.height * 0.90
            vehicle: globals.activeVehicle
        }

    }

}
