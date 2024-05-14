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



    function showCriticalVehicleMessage(message) {
        mainWindow.hideIndicatorPopup()

        if (criticalVehicleMessagePopup.visible || QGroundControl.videoManager.fullScreen) {
            // We received additional wanring message while an older warning message was still displayed.
            // When the user close the older one drop the message indicator tool so they can see the rest of them.
            criticalVehicleMessagePopup.dropMessageIndicatorOnClose = true
        } else {
            criticalVehicleMessagePopup.criticalVehicleMessage      = message
            criticalVehicleMessagePopup.dropMessageIndicatorOnClose = false
            criticalVehicleMessagePopup.open()
        }
    }

    Popup {
        id:                 criticalVehicleMessagePopup
        y:                  ScreenTools.defaultFontPixelHeight
        x:                  Math.round((mainWindow.width - width) * 0.5)
        width:              mainWindow.width  * 0.55
        height:             criticalVehicleMessageText.contentHeight + ScreenTools.defaultFontPixelHeight * 2
        modal:              false
        focus:              true
        closePolicy:        Popup.CloseOnEscape

        property alias  criticalVehicleMessage:        criticalVehicleMessageText.text
        property bool   dropMessageIndicatorOnClose:   false

        background: Rectangle {
            anchors.fill:   parent
            color:          qgcPal.alertBackground
            radius:         ScreenTools.defaultFontPixelHeight * 0.5
            border.color:   qgcPal.alertBorder
            border.width:   2

            Rectangle {
                anchors.horizontalCenter:   parent.horizontalCenter
                anchors.top:                parent.top
                anchors.topMargin:          -(height / 2)
                color:                      qgcPal.alertBackground
                radius:                     ScreenTools.defaultFontPixelHeight * 0.25
                border.color:               qgcPal.alertBorder
                border.width:               1
                width:                      vehicleWarningLabel.contentWidth + _margins
                height:                     vehicleWarningLabel.contentHeight + _margins

                property real _margins: ScreenTools.defaultFontPixelHeight * 0.25

                QGCLabel {
                    id:                 vehicleWarningLabel
                    anchors.centerIn:   parent
                    text:               qsTr("Vehicle Error")
                    font.pointSize:     ScreenTools.smallFontPointSize
                    color:              qgcPal.alertText
                }
            }

            Rectangle {
                id:                         additionalErrorsIndicator
                anchors.horizontalCenter:   parent.horizontalCenter
                anchors.bottom:             parent.bottom
                anchors.bottomMargin:       -(height / 2)
                color:                      qgcPal.alertBackground
                radius:                     ScreenTools.defaultFontPixelHeight * 0.25
                border.color:               qgcPal.alertBorder
                border.width:               1
                width:                      additionalErrorsLabel.contentWidth + _margins
                height:                     additionalErrorsLabel.contentHeight + _margins
                visible:                    criticalVehicleMessagePopup.dropMessageIndicatorOnClose

                property real _margins: ScreenTools.defaultFontPixelHeight * 0.25

                QGCLabel {
                    id:                 additionalErrorsLabel
                    anchors.centerIn:   parent
                    text:               qsTr("Additional errors received")
                    font.pointSize:     ScreenTools.smallFontPointSize
                    color:              qgcPal.alertText
                }
            }
        }

        QGCLabel {
            id:                 criticalVehicleMessageText
            width:              criticalVehicleMessagePopup.width - ScreenTools.defaultFontPixelHeight
            anchors.centerIn:   parent
            wrapMode:           Text.WordWrap
            color:              qgcPal.alertText
            textFormat:         TextEdit.RichText
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                criticalVehicleMessagePopup.close()
                if (criticalVehicleMessagePopup.dropMessageIndicatorOnClose) {
                    criticalVehicleMessagePopup.dropMessageIndicatorOnClose = false;
                    QGroundControl.multiVehicleManager.activeVehicle.resetErrorLevelMessages();
                    toolbar.dropMessageIndicatorTool();
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


    Rectangle {
        property double value: _activeVehicle ? (isNaN(_activeVehicle.altitudeRelative.value) ? 0.0 : _activeVehicle.altitudeRelative.value) : 0.0
        onValueChanged: {

            var alt = _activeVehicle.altitudeRelative.rawValue;

            showCriticalVehicleMessage("Above 120m")
            console.log("CHIAMATO");

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
