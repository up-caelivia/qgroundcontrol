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
    property bool _selected1: false


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


    // UPPER BOX !
    // Rectangle {
    //     anchors.horizontalCenter: attitudeIndicator.horizontalCenter
    //     anchors.bottom: attitudeIndicator.top
    //     anchors.bottomMargin: - attitudeIndicator.height / 3
    //     width: attitudeIndicator.width*0.65
    //     height: attitudeIndicator.height * 0.55
    //     radius: ScreenTools.defaultFontPixelWidth
    //     color: qgcPal.window

    //     property real _indicatorsHeight: ScreenTools.defaultFontPixelHeight * 2

    //     QGCColoredImage {
    //                    id: img1
    //                    height:                 _indicatorsHeight
    //                    width:                  height
    //                    source:                 "/custom/img/vertical_speed.svg"
    //                    fillMode:               Image.PreserveAspectFit
    //                    sourceSize.height:      height
    //                    //Layout.alignment:       Qt.AlignVCenter | Qt.AlignHCenter
    //                    color:                  qgcPal.text
    //                    anchors.left: parent.left
    //                    anchors.leftMargin: ScreenTools.defaultFontPixelWidth  * 4

    //                    anchors.bottomMargin: + attitudeIndicator.height / 3 + ScreenTools.defaultFontPixelWidth  * 2
    //                    anchors.bottom: parent.bottom
    //                    }

    //                QGCLabel {
    //                    text:                   _activeVehicle ? _activeVehicle.climbRate.value.toFixed(1) + ' ' + _activeVehicle.climbRate.units : " 0.0"
    //                    color:                  _indicatorsColor
    //                    font.pointSize:         ScreenTools.mediumFontPointSize
    //                    //Layout.fillWidth:       true
    //                    //Layout.minimumWidth:    indicatorValueWidth
    //                    anchors.left: img1.right
    //                    anchors.leftMargin: ScreenTools.defaultFontPixelWidth
    //                    //anchors.leftMargin: 5
    //                    anchors.bottomMargin: + attitudeIndicator.height / 3 + ScreenTools.defaultFontPixelWidth  * 2
    //                    anchors.bottom: parent.bottom

    //                }

    //                QGCColoredImage {
    //                    height:                 _indicatorsHeight
    //                    width:                  height
    //                    source:                   "/custom/img/distance.svg"
    //                    fillMode:               Image.PreserveAspectFit
    //                    sourceSize.height:      height
    //                    color:                  qgcPal.text
    //                    anchors.right: txt2.left
    //                    anchors.rightMargin: ScreenTools.defaultFontPixelWidth
    //                    anchors.bottomMargin: + attitudeIndicator.height / 3 + ScreenTools.defaultFontPixelWidth  * 2
    //                    anchors.bottom: parent.bottom

    //                }

    //                QGCLabel {
    //                    id: txt2
    //                    text:                   _activeVehicle && !isNaN(_activeVehicle.distanceToHome.value) ? (('0000' + _activeVehicle.distanceToHome.value.toFixed(0)).slice(-4) + ' ' + _activeVehicle.distanceToHome.units) : "--/--"
    //                    color:                  _indicatorsColor
    //                    font.pointSize:         ScreenTools.mediumFontPointSize
    //                    anchors.right: parent.right
    //                    anchors.rightMargin: ScreenTools.defaultFontPixelWidth  * 4
    //                    anchors.bottomMargin: + attitudeIndicator.height / 3 + ScreenTools.defaultFontPixelWidth  * 2
    //                    anchors.bottom: parent.bottom

    //                }
    // }



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
                text:                  _activeVehicle ? _activeVehicle.climbRate.value.toFixed(1) + ' ' + _activeVehicle.climbRate.units : " 0.0"
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
        //anchors.verticalCenter: attitudeIndicator.verticalCenter
        anchors.left:          attitudeIndicator.right
        anchors.leftMargin:  ScreenTools.defaultFontPixelHeight * 0.4  // -attitudeIndicator.width / 3
        width:  attitudeIndicator.width*0.6
        height: attitudeIndicator.height * 0.65
        radius:                 ScreenTools.defaultFontPixelWidth
        color:                  qgcPal.window
        anchors.bottom : attitudeIndicator.bottom


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

        Column {

            anchors.top: parent.top
            anchors.horizontalCenter:   parent.horizontalCenter


            QGCRadioButton {
                id:                     yoyo1
                leftPadding:            0
                text:                   "Camera 1"
                checked:                !!_selected1
                onClicked:              _selected1 = true
            }

            QGCRadioButton {
                id:                     yoyo2
                leftPadding:            0
                text:                   "Camera 2"
                checked:                !_selected1
                onClicked:              _selected1 = false
            }
        }

    }

}
