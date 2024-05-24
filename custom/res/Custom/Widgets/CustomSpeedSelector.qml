/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.4
import QtPositioning            5.2
import QtQuick.Layouts          1.2
import QtQuick.Controls         1.4
import QtQuick.Dialogs          1.2
import QtGraphicalEffects       1.0

import QGroundControl                   1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Controls          1.0
import QGroundControl.Palette           1.0
import QGroundControl.Vehicle           1.0
import QGroundControl.Controllers       1.0
import QGroundControl.FactSystem        1.0
import QGroundControl.FactControls      1.0

//TODO: check values

Rectangle {
    height:     mainLayout.height + (_margins * 2)
    color:      Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.5)
    radius:     _margins

    property real   _margins:                                   ScreenTools.defaultFontPixelHeight / 2
    property var    _activeVehicle:                             QGroundControl.multiVehicleManager.activeVehicle
    property bool   _secondButton:                    false
    property bool   _isLowSpeed: false
    property var _controller
    property var modality: _activeVehicle ? _activeVehicle.flightMode : ""

    onModalityChanged: {

        if(modality != "Loiter" && _isLowSpeed) {
            showCriticalVehicleMessage("LOW SPEED MODE NOT AVAILABLE")
        }

        if(modality == "Loiter" && _isLowSpeed) {
            showCriticalVehicleMessage("LOW SPEED MODE ACTIVATED")
        }

    }

    property double defaultLoitSpeed: 0.0
    // property double defaultWPNavSpeed: 0.0

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    function showCriticalVehicleMessage(message) {
        mainWindow.closeCriticalVehicleMessage()
        mainWindow.showCriticalVehicleMessage(message)
    }

    function setValueSpeed(loit, lowSpeedMode) {

        var par_found = false
        var loit_speed
        // var wpnav_speed

        _controller.object.searchText = "LOIT_SPEED"

        for( var i = 0; i < _controller.object.parameters.rowCount(); i++ ) {

            if ( _controller.object.parameters.get(i).name == "LOIT_SPEED")  {
                loit_speed = _controller.object.parameters.get(i)
                par_found = true

                if(lowSpeedMode)
                {defaultLoitSpeed = loit_speed.value}

                loit_speed.value = loit
                loit_speed.valueChanged(loit_speed.value)

                break
            }
        }


        // _controller.object.searchText = "WPNAV_SPEED"

        // for( var i = 0; i < _controller.object.parameters.rowCount(); i++ ) {
        //     if ( _controller.object.parameters.get(i).name == "WPNAV_SPEED")  {
        //         wpnav_speed = _controller.object.parameters.get(i)
        //         par_found = par_found && true

        //         if(lowSpeedMode){
        //             defaultWPNavSpeed = wpnav_speed.value
        //         }

        //         wpnav_speed.value = wpnav
        //         wpnav_speed.valueChanged(wpnav_speed.value)

        //         break
        //     }
        // }

        _controller.object.parametersChanged()

        if (!par_found)
            console.log("Not found the parameters for low speed mode")
    }



    ColumnLayout {
        id:                         mainLayout
        anchors.margins:            _margins
        anchors.top:                parent.top
        anchors.horizontalCenter:   parent.horizontalCenter
        spacing:                    ScreenTools.defaultFontPixelHeight / 2

        // Speed Selector
        Rectangle {
            Layout.alignment:   Qt.AlignHCenter
            width:              ScreenTools.defaultFontPixelWidth * 10
            height:             width / 2
            color:              qgcPal.windowShadeLight
            radius:             height * 0.5
            opacity: _isLowSpeed ? 0.5 : 1

            // First Button
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width:                  parent.height
                height:                 parent.height
                color:                  _secondButton ? qgcPal.windowShadeLight : qgcPal.window
                radius:                 height * 0.5
                anchors.left:           parent.left
                border.color:           qgcPal.text
                border.width:           _secondButton ? 0 : 1

                QGCColoredImage {
                    height:             parent.height * 0.5
                    width:              height
                    anchors.centerIn:   parent
                    source:             "/custom/img/n.svg"
                    fillMode:           Image.PreserveAspectFit
                    sourceSize.height:  height
                    color:              _secondButton ? qgcPal.text : qgcPal.colorGreen
                }

                MouseArea {
                    anchors.fill:   parent
                    enabled:        !_isLowSpeed
                    onClicked:      {
                        _secondButton = false
                        setValueSpeed(500, false)
                    }
                }
            }
            // Second Button
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width:                  parent.height
                height:                 parent.height
                color:                  _secondButton ? qgcPal.window : qgcPal.windowShadeLight
                radius:                 height * 0.5
                anchors.right:          parent.right
                border.color:           qgcPal.text
                border.width:           _secondButton ? 1 : 0

                QGCColoredImage {
                    height:             parent.height * 0.5
                    width:              height
                    anchors.centerIn:   parent
                    source:             "/custom/img/h.svg"
                    fillMode:           Image.PreserveAspectFit
                    sourceSize.height:  height
                    color:              _secondButton ? qgcPal.colorGreen : qgcPal.text
                }

                MouseArea {
                    anchors.fill:   parent
                    enabled:        !_isLowSpeed
                    onClicked:      {

                        _secondButton = true
                        setValueSpeed(1000, false)

                    }
                }
            }
        }


        // Central Button
        Rectangle {
            Layout.alignment:   Qt.AlignHCenter
            color:              Qt.rgba(0,0,0,0)
            width:              ScreenTools.defaultFontPixelWidth * 6
            height:             width
            radius:             width * 0.5
            border.color:       qgcPal.buttonText
            border.width:       3

            // Rectangle {
            //     anchors.centerIn:   parent
            //     width:              parent.width * 0.7
            //     height:             width
            //     radius:             width * 0.5
            //     // color:              _isLowSpeed ? qgcPal.colorRed : qgcPal.colorGrey

                QGCColoredImage {
                    height:             parent.height * 0.5
                    width:              height
                    anchors.centerIn:   parent
                    source:             "/custom/img/alert.svg"
                    fillMode:           Image.PreserveAspectFit
                    sourceSize.height:  height
                    color:              _secondButton ? qgcPal.colorGreen : qgcPal.text
                }



            // }

            MouseArea {
                anchors.fill:   parent
                        onClicked: {

                                           _isLowSpeed = ! _isLowSpeed

                                           if (!_isLowSpeed) {
                                               showCriticalVehicleMessage("LOW SPEED MODE DISABLED")
                                               setValueSpeed(defaultLoitSpeed, _isLowSpeed)
                                               return
                                           }

                                           if(modality != "Loiter") {
                                               showCriticalVehicleMessage("LOW SPEED MODE NOT AVAILABLE")
                                               // TODO: change icon
                                           } else {
                                               showCriticalVehicleMessage("LOW SPEED MODE ACTIVATED")
                                           }


                                           setValueSpeed(260, _isLowSpeed)

                                       }
            }
        }




    }

}
