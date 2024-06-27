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
    property string _altitude:              _activeVehicle ? (isNaN(_activeVehicle.altitudeRelative.value) ? "0.0" : _activeVehicle.altitudeRelative.value.toFixed(1)) + ' ' + _activeVehicle.altitudeRelative.units : "--/--"
    property string _distanceStr:           isNaN(_distance) ? "0" : _distance.toFixed(0) + ' ' + QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString
    property real   _heading:               _activeVehicle   ? _activeVehicle.heading.rawValue : 0
    property real   _distance:              _activeVehicle ? _activeVehicle.distanceToHome.rawValue : 0
    property string _messageTitle:          ""
    property string _messageText:           ""
    property real   _toolsMargin:           ScreenTools.defaultFontPixelWidth * 0.75

    function showCriticalVehicleMessage(message) {
        mainWindow.closeCriticalVehicleMessage()
        mainWindow.showCriticalVehicleMessage(message)
    }

    Rectangle {

        property double value: _activeVehicle ? (isNaN(_activeVehicle.altitudeRelative.value) ? 0.0 : _activeVehicle.altitudeRelative.value) : 0.0
        //property string value:              _activeVehicle ? (isNaN(_activeVehicle.altitudeRelative.value) ? "0.0" : _activeVehicle.altitudeRelative.value.toFixed(1)) + ' ' + _activeVehicle.altitudeRelative.units : "0.0"

        property bool above120: false

        onValueChanged: {

            if (_activeVehicle == null || _activeVehicle.altitudeRelative == null)
                return;

            if ((value > 120 && _activeVehicle.altitudeRelative.units == "m") || ( value > 393.7 && _activeVehicle.altitudeRelative.units == "ft")) {

                if(!above) {
                    showCriticalVehicleMessage("WARNING : Above 120m")
                    _activeVehicle.announceAltitude();
                }

                above = true
                console.log("INFO: Vehicle above 120m");
            } else {
                if(above && mainWindows.getCriticalVehicleMessage() == "WARNING : Above 120m") {
                    mainWindow.closeCriticalVehicleMessage()
                    above = false
                }
            }
        }
    }



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


    ProximityRadarValues {
        id:                     proximityValues
        vehicle:                _activeVehicle
    }

    // TOP BOX !
    Rectangle {
        anchors.horizontalCenter: attitudeIndicator.horizontalCenter
        anchors.bottom: attitudeIndicator.top
        anchors.bottomMargin: ScreenTools.defaultFontPixelHeight * 0.1
        visible: proximityValues.telemetryAvailable && !isNaN(proximityValues.rotationNoneValue)

        width: attitudeIndicator.width * 0.6
        height: attitudeIndicator.height * 0.3
        radius: ScreenTools.defaultFontPixelHeight
        color: qgcPal.window

        GridLayout {
            columnSpacing: ScreenTools.defaultFontPixelWidth * 0.3
            rowSpacing: ScreenTools.defaultFontPixelHeight * 0.3
            columns: 2

            property real _indicatorsHeight: ScreenTools.defaultFontPixelHeight
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter


            QGCColoredImage {
                height: _indicatorsHeight
                width: height
                source: "/custom/img/vertical_speed.svg"
                fillMode: Image.PreserveAspectFit
                sourceSize.height: height
                Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
                color: qgcPal.text
            }

            QGCLabel {
                id: distanceLabel
                text: distance.toFixed(1) + " m"
                font.pointSize: ScreenTools.mediumFontPointSize
                Layout.fillWidth: true
                Layout.minimumWidth: indicatorValueWidth
                Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter

                property real distance: proximityValues.rotationNoneValue


                property color normalColor: {
                    if (distance < 10)
                        return "orange"
                    return _indicatorsColor
                }

                color: normalColor // Default color setting based on distance

                SequentialAnimation on color {
                    id: pulseAnimation
                    loops: Animation.Infinite
                    ColorAnimation {
                        from: "white"
                        to: "red"
                        duration: 200
                    }
                    ColorAnimation {
                        from: "red"
                        to: "white"
                        duration: 200
                    }
                }

                Component.onCompleted: {
                    // Start or stop the animation based on the initial distance
                    pulseAnimation.running = distance < 5
                }

                onDistanceChanged: {
                    // Restart animation when distance changes
                    pulseAnimation.running = distance < 5
                    if (distance < 5) {
                        color = "white"
                    } else {
                        color = normalColor
                    }
                }
            }
        }
    }







    // UPPER LEFT BOX !
    Rectangle {
        anchors.horizontalCenter: left_box.horizontalCenter
        anchors.bottom:           left_box.top
        anchors.rightMargin:  ScreenTools.defaultFontPixelHeight * 0.4  // -attitudeIndicator.width / 3
        anchors.bottomMargin:   ScreenTools.defaultFontPixelHeight * 0.1  // -attitudeIndicator.width / 3



        width:  attitudeIndicator.width*0.6
        height: attitudeIndicator.height * 0.3
        radius:                 ScreenTools.defaultFontPixelHeight
        color:                  qgcPal.window

        GridLayout {
                    columnSpacing:          ScreenTools.defaultFontPixelWidth  * 0.3
                    rowSpacing:             ScreenTools.defaultFontPixelHeight * 0.3
                    columns:                2

            property real   _indicatorsHeight:      ScreenTools.defaultFontPixelHeight
            //anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            //anchors.rightMargin: 12

            QGCColoredImage {
                height:                 _indicatorsHeight
                width:                  height
                source:                "/custom/img/distance.svg"
                fillMode:               Image.PreserveAspectFit
                sourceSize.height:      height
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
                color:                  qgcPal.text
            }

            QGCLabel {
                text:                   _activeVehicle && !isNaN(_activeVehicle.distanceToHome.value) ? (('0000' + _activeVehicle.distanceToHome.value.toFixed(0)).slice(-4) + ' ' + _activeVehicle.distanceToHome.units) : "--/--"
                color:                  _indicatorsColor
                font.pointSize:         ScreenTools.mediumFontPointSize
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter

            }
        }
    }




    // LEFT BOX !
    Rectangle {

        id: left_box

        anchors.right:          attitudeIndicator.left
        anchors.rightMargin:  ScreenTools.defaultFontPixelHeight * 0.4  // -attitudeIndicator.width / 3
        width:  attitudeIndicator.width*0.6
        height: attitudeIndicator.height * 0.65
       // anchors.verticalCenter: attitudeIndicator.verticalCenter
        anchors.bottom : attitudeIndicator.bottom

        radius:                 ScreenTools.defaultFontPixelHeight
        color:                  qgcPal.window

        GridLayout {
                    columnSpacing:          ScreenTools.defaultFontPixelWidth  * 2
                    rowSpacing:             ScreenTools.defaultFontPixelHeight * 0.3
                    columns:                1

            property real   _indicatorsHeight:      ScreenTools.defaultFontPixelHeight *2
            //anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            //anchors.rightMargin: 12

            QGCColoredImage {                
                height:                 _indicatorsHeight * 1.4
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
                font.pointSize:         ScreenTools.largeFontPointSize * 1.4
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
                font.weight: Font.Bold

            }
        }
    }

    // UPPER RIGHT BOX !
    Rectangle {
        anchors.horizontalCenter: right_box.horizontalCenter
        anchors.bottom:           right_box.top
        anchors.rightMargin:  ScreenTools.defaultFontPixelHeight * 0.4  // -attitudeIndicator.width / 3
        anchors.bottomMargin:   ScreenTools.defaultFontPixelHeight * 0.1  // -attitudeIndicator.width / 3



        width:  attitudeIndicator.width*0.6
        height: attitudeIndicator.height * 0.3
        radius:                 ScreenTools.defaultFontPixelHeight
        color:                  qgcPal.window

        GridLayout {
                    columnSpacing:          ScreenTools.defaultFontPixelWidth  * 0.3
                    rowSpacing:             ScreenTools.defaultFontPixelHeight * 0.3
                    columns:                2

            property real   _indicatorsHeight:      ScreenTools.defaultFontPixelHeight
            //anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            //anchors.rightMargin: 12

            QGCColoredImage {
                height:                 _indicatorsHeight
                width:                  height
                source:                "/custom/img/vertical_speed.svg"
                fillMode:               Image.PreserveAspectFit
                sourceSize.height:      height
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
                color:                  qgcPal.text
            }

            QGCLabel {
                text:                  _activeVehicle ? _activeVehicle.climbRate.value.toFixed(1) + ' ' + _activeVehicle.climbRate.units : " --/--"
                color:                  _indicatorsColor
                font.pointSize:         ScreenTools.mediumFontPointSize
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter

            }
        }
    }




    // RIGHT BOX !
    Rectangle {

        id: right_box

        anchors.left:          attitudeIndicator.right
        anchors.bottom : attitudeIndicator.bottom

        anchors.leftMargin:  ScreenTools.defaultFontPixelHeight * 0.4  // -attitudeIndicator.width / 3
        width:  attitudeIndicator.width*0.6
        height: attitudeIndicator.height * 0.65

        radius:                 ScreenTools.defaultFontPixelHeight
        color:                  qgcPal.window

        GridLayout {
                    columnSpacing:          ScreenTools.defaultFontPixelWidth  * 2
                    rowSpacing:             ScreenTools.defaultFontPixelHeight * 0.3
                    columns:                1

            property real   _indicatorsHeight:      ScreenTools.defaultFontPixelHeight *2
            //anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            //anchors.rightMargin: 12

            QGCColoredImage {
                height:                 _indicatorsHeight * 1.4
                width:                  height
                source:                 "/custom/img/horizontal_speed.svg"
                fillMode:               Image.PreserveAspectFit
                sourceSize.height:      height
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
                color:                  qgcPal.text
            }

            QGCLabel {
                text:                   _activeVehicle ? _activeVehicle.groundSpeed.value.toFixed(1) + ' ' + _activeVehicle.groundSpeed.units : "--/--"
                color:                  _indicatorsColor
                font.pointSize:         ScreenTools.largeFontPointSize * 1.4
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
                font.weight: Font.Bold

            }
        }
    }



    Rectangle {
        id:                     attitudeIndicator
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin:   _toolsMargin + parentToolInsets.bottomEdgeRightInset
        anchors.rightMargin:    _toolsMargin
        anchors.bottom:         parent.bottom
       // height:                 ScreenTools.defaultFontPixelHeight * 10
        height: parent.height / 4
        width:                  height * 1.98
        radius:                 height * 0.5
        color:                  qgcPal.window   //qgcPal.windowShade

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
