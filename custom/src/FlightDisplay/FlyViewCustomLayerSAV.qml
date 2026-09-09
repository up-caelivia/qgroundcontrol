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
import CustomAnnouncer 1.0

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
    property string _latitude:              _activeVehicle ? (isNaN(_activeVehicle.latitude) ? "0.0" : _activeVehicle.latitude.toFixed(6)) : "--/--"
    property string _longitude:              _activeVehicle ? (isNaN(_activeVehicle.longitude) ? "0.0" : _activeVehicle.longitude.toFixed(6)) : "--/--"
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
        property bool above120: false

        onValueChanged: {

            if (_activeVehicle == null || _activeVehicle.altitudeRelative == null)
                return;

            if ((value > 120 && _activeVehicle.altitudeRelative.units == "m") || ( value > 393.7 && _activeVehicle.altitudeRelative.units == "ft")) {

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
        anchors.right: attitudeIndicator.right
        anchors.bottom: attitudeIndicator.top
        anchors.bottomMargin: ScreenTools.defaultFontPixelHeight * 0.1
        visible: proximityValues.telemetryAvailable && !isNaN(proximityValues.rotationNoneValue)

        width: attitudeIndicator.width * 0.6
        height: attitudeIndicator.height * 0.3
        radius: ScreenTools.defaultFontPixelHeight
        color: Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.7)

        GridLayout {
            columnSpacing: ScreenTools.defaultFontPixelWidth * 0.3
            rowSpacing: ScreenTools.defaultFontPixelHeight * 0.3
            columns: 2

            property real _indicatorsHeight: ScreenTools.defaultFontPixelHeight
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter


            QGCColoredImage {
                height: _indicatorsHeight * 0.8
                width: height
                source: "/custom/img/vertical_speed.svg"
                fillMode: Image.PreserveAspectFit
                sourceSize.height: height
                Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
                color: qgcPal.text
            }

            QGCLabel {
                id: distanceLabel
                text: "  " + distance.toFixed(1) + " m"
                font.pointSize: ScreenTools.mediumFontPointSize * 0.8
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
        id: upperLeftBox
        anchors.top: attitudeIndicator.bottom
        anchors.right: upperRightBox.left
        anchors.topMargin: ScreenTools.defaultFontPixelHeight * 0.1
        anchors.rightMargin: ScreenTools.defaultFontPixelHeight * 0.4
        width:  attitudeIndicator.width*0.5
        height: attitudeIndicator.height * 0.3
        radius:                 ScreenTools.defaultFontPixelHeight
        color:                  Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.7)

        GridLayout {
                    columnSpacing:          ScreenTools.defaultFontPixelWidth  * 0.5
                    rowSpacing:             ScreenTools.defaultFontPixelHeight * 0.3
                    columns:                2

            property real   _indicatorsHeight:      ScreenTools.defaultFontPixelHeight
            //anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            //anchors.rightMargin: 12

            QGCColoredImage {
                height:                 _indicatorsHeight * 0.8
                width:                  height
                source:                "/custom/img/distance.svg"
                fillMode:               Image.PreserveAspectFit
                sourceSize.height:      height
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
                color:                  qgcPal.text
            }

            QGCLabel {
                text:                   _activeVehicle && !isNaN(_activeVehicle.distanceToHome.value) ? (('  0000' + _activeVehicle.distanceToHome.value.toFixed(0)).slice(-4) + ' ' + _activeVehicle.distanceToHome.units) : "   --/--"
                color:                  _indicatorsColor
                font.pointSize:         ScreenTools.mediumFontPointSize * 0.8
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter

            }
        }
    }

    // LEFT BOX !
    Rectangle {
            id: leftBox
            anchors.top: upperLeftBox.bottom
            anchors.right: rightBox.left
            anchors.topMargin: ScreenTools.defaultFontPixelHeight * 0.1
            anchors.rightMargin: ScreenTools.defaultFontPixelHeight * 0.4
            width:  attitudeIndicator.width*0.5
            height: attitudeIndicator.height * 0.3
            radius:                 ScreenTools.defaultFontPixelHeight
            color:                  Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.7)

            GridLayout {
                        columnSpacing:          ScreenTools.defaultFontPixelWidth  * 0.5
                        rowSpacing:             ScreenTools.defaultFontPixelHeight * 0.3
                        columns:                2

                property real   _indicatorsHeight:      ScreenTools.defaultFontPixelHeight
                //anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                //anchors.rightMargin: 12
                

                QGCColoredImage {
                    source: "/custom/img/altitude.svg"
                    height:                 _indicatorsHeight * 0.8
                    width:                  height
                    fillMode:               Image.PreserveAspectFit
                    sourceSize.height:      height
                    Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
                    color:                  qgcPal.text
                }

                QGCLabel {
                    text:                   _altitude
                    color:                  _indicatorsColor
                    font.pointSize:         ScreenTools.mediumFontPointSize * 0.8
                    Layout.fillWidth:       true
                    Layout.minimumWidth:    indicatorValueWidth
                    Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
                }
            }
    }

    //DOWNER LEFT BOX !
    Rectangle {
            id: downerLeftBox
            anchors.top: leftBox.bottom
            anchors.right: rightBox.left
            anchors.topMargin: ScreenTools.defaultFontPixelHeight * 0.1
            anchors.rightMargin: ScreenTools.defaultFontPixelHeight * 0.4
            width:  attitudeIndicator.width*0.5
            height: attitudeIndicator.height * 0.3
            radius:                 ScreenTools.defaultFontPixelHeight
            color:                  Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.7)

            GridLayout {
                        columnSpacing:          ScreenTools.defaultFontPixelWidth  * 0.5
                        rowSpacing:             ScreenTools.defaultFontPixelHeight * 0.3
                        columns:                1

                property real   _indicatorsHeight:      ScreenTools.defaultFontPixelHeight
                //anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                //anchors.rightMargin: 12

                QGCLabel {
                    text:                   "LAT: " + _latitude
                    color:                  _indicatorsColor
                    font.pointSize:         ScreenTools.mediumFontPointSize * 0.6
                    Layout.fillWidth:       true
                    Layout.minimumWidth:    indicatorValueWidth
                    Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
                }
            }
    }

    // UPPER RIGHT BOX !
    Rectangle {
        id: upperRightBox
        anchors.top: attitudeIndicator.bottom
        anchors.right: attitudeIndicator.right
        anchors.topMargin: ScreenTools.defaultFontPixelHeight * 0.1
        anchors.leftMargin: ScreenTools.defaultFontPixelHeight * 0.4
        width:  attitudeIndicator.width*0.5
        height: attitudeIndicator.height * 0.3
        radius:                 ScreenTools.defaultFontPixelHeight
        color:                  Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.7)

        GridLayout {
                    columnSpacing:          ScreenTools.defaultFontPixelWidth  * 0.5
                    rowSpacing:             ScreenTools.defaultFontPixelHeight * 0.3
                    columns:                2

            property real   _indicatorsHeight:      ScreenTools.defaultFontPixelHeight
            //anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            //anchors.rightMargin: 12

            QGCColoredImage {
                height:                 _indicatorsHeight * 1.3
                width:                  height
                source:                "/custom/img/MovimentoY.svg"
                fillMode:               Image.PreserveAspectFit
                sourceSize.height:      height
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
                color:                  qgcPal.text
            }

            QGCLabel {
                text:                  _activeVehicle ? "  "+_activeVehicle.climbRate.value.toFixed(1) + ' ' + _activeVehicle.climbRate.units : "   --/--"
                color:                  _indicatorsColor
                font.pointSize:         ScreenTools.mediumFontPointSize * 0.8
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter

            }
        }
    }

    // RIGHT BOX!
    Rectangle {
        id: rightBox
        anchors.top: upperRightBox.bottom
        anchors.right: attitudeIndicator.right
        anchors.topMargin: ScreenTools.defaultFontPixelHeight * 0.1
        anchors.leftMargin: ScreenTools.defaultFontPixelHeight * 0.4
        width:  attitudeIndicator.width*0.5
        height: attitudeIndicator.height * 0.3
        radius:                 ScreenTools.defaultFontPixelHeight
        color:                  Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.7)

        GridLayout {
                    columnSpacing:          ScreenTools.defaultFontPixelWidth  * 0.5
                    rowSpacing:             ScreenTools.defaultFontPixelHeight * 0.3
                    columns:                2

            property real   _indicatorsHeight:      ScreenTools.defaultFontPixelHeight
            //anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            //anchors.rightMargin: 12
            

            QGCColoredImage {
                source: "/custom/img/MovimentoZ.svg"
                height:                 _indicatorsHeight * 1.3
                width:                  height
                fillMode:               Image.PreserveAspectFit
                sourceSize.height:      height
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
                color:                  qgcPal.text
            }

            QGCLabel {
                text:                   _activeVehicle ? _activeVehicle.groundSpeed.value.toFixed(1) + ' ' + _activeVehicle.groundSpeed.units : "--/--"
                color:                  _indicatorsColor
                font.pointSize:         ScreenTools.mediumFontPointSize * 0.8
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
            }
        }
    }

    // RIGHT BOX!
    Rectangle {
        id: donwerRightBox
        anchors.top: rightBox.bottom
        anchors.right: attitudeIndicator.right
        anchors.topMargin: ScreenTools.defaultFontPixelHeight * 0.1
        anchors.leftMargin: ScreenTools.defaultFontPixelHeight * 0.4
        width:  attitudeIndicator.width*0.5
        height: attitudeIndicator.height * 0.3
        radius:                 ScreenTools.defaultFontPixelHeight
        color:                  Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.7)

        GridLayout {
                    columnSpacing:          ScreenTools.defaultFontPixelWidth  * 0.5
                    rowSpacing:             ScreenTools.defaultFontPixelHeight * 0.3
                    columns:                1

            property real   _indicatorsHeight:      ScreenTools.defaultFontPixelHeight
            //anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            //anchors.rightMargin: 12

            QGCLabel {
                text:                   "LON: " + _longitude
                color:                  _indicatorsColor
                font.pointSize:         ScreenTools.mediumFontPointSize * 0.6
                Layout.fillWidth:       true
                Layout.minimumWidth:    indicatorValueWidth
                Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
            }
        }
    }

    // NAVIGATION BOX!
    Rectangle {
        id:                     attitudeIndicator
        anchors.left: parent.left
        anchors.topMargin:   _toolsMargin + parentToolInsets.topEdgeRightInset
        anchors.leftMargin:    _toolsMargin
        anchors.top:         parent.top
       // height:                 ScreenTools.defaultFontPixelHeight * 10
        height: parent.height / 5
        width:                  height * 1.98
        radius:                 height * 0.5
        color:                  Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.7)

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
