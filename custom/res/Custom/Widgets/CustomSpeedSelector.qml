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

import Constants  1.0


Rectangle {
    height:     mainLayout.height + (_margins * 2)
    color:      Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.5)
    radius:     _margins

    property real   _margins:                                   ScreenTools.defaultFontPixelHeight / 2
    property var    _activeVehicle:                             QGroundControl.multiVehicleManager.activeVehicle
    property bool   _isHighSpeed:                    false
    property bool   _isLowSpeed:                     false
    property var _controller
    property var modality: _activeVehicle ? _activeVehicle.flightMode : ""


    function setModality() {

        for( var j = 0; j < Constants.factSpeedNames.length; j++) {

            _controller.object.searchText = Constants.factSpeedNames[j];

            for( var i = 0; i < _controller.object.parameters.rowCount(); i++ ) {

                var fact = _controller.object.parameters.get(i)

                if ( fact.name == Constants.factSpeedNames[j])  {

                    if(_isLowSpeed) {
                        fact.value = Constants.lowSpeed[j]
                    }
                    else {

                        if(_isHighSpeed)
                            fact.value = Constants.highSpeed[j]
                        else
                            fact.value = Constants.normalSpeed[j]
                    }

                    fact.valueChanged(fact.value)
                    break
                }
            }
        }
         _controller.object.parametersChanged()
    }

    onVisibleChanged: {
        if (visible)
            setModality()
    }

    onModalityChanged: {

        if(modality != "Loiter" && _isLowSpeed) {
            showCriticalVehicleMessage("LOW SPEED MODE NOT AVAILABLE")
        }

        if(modality == "Loiter" && _isLowSpeed) {
            showCriticalVehicleMessage("LOW SPEED MODE ACTIVATED")
        }

    }

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    function showCriticalVehicleMessage(message) {
        mainWindow.closeCriticalVehicleMessage()
        mainWindow.showCriticalVehicleMessage(message)
    }

    MouseArea {
        anchors.fill:   parent

    ColumnLayout {
        id:                         mainLayout
        anchors.margins:            _margins
        anchors.top:                parent.top
        anchors.horizontalCenter:   parent.horizontalCenter
        spacing:                    ScreenTools.defaultFontPixelHeight / 2

        // Speed Selector
        Rectangle {
            Layout.alignment:   Qt.AlignHCenter
            width:              ScreenTools.defaultFontPixelWidth * 11
            height:             ScreenTools.defaultFontPixelWidth * 10 / 2
            color:              qgcPal.windowShadeLight
            radius:             height * 0.5

            // First Button
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width:                  parent.height
                height:                 parent.height
                color:                  _isHighSpeed ? qgcPal.windowShadeLight : qgcPal.window
                radius:                 height * 0.5
                anchors.left:           parent.left
                border.color:           qgcPal.text
                border.width:           _isHighSpeed ? 0 : 1
                opacity: _isLowSpeed ? 0.6 : 1


                QGCColoredImage {
                    height:             parent.height * 3 // * 0.5
                    width:              height
                    anchors.centerIn:   parent
                    source:             "/custom/img/n.svg"
                    fillMode:           Image.PreserveAspectFit
                    sourceSize.height:  height
                    color:              _isHighSpeed ? qgcPal.text : qgcPal.colorGreen
                }

                MouseArea {
                    anchors.fill:   parent
                    enabled:        !_isLowSpeed
                    onClicked:      {
                        _isHighSpeed = false
                        setModality()
                    }
                }
            }
            // Second Button
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width:                  parent.height
                height:                 parent.height
                color:                  _isHighSpeed ? qgcPal.window : qgcPal.windowShadeLight
                radius:                 height * 0.5
                anchors.right:          parent.right
                border.color:           qgcPal.text
                border.width:           _isHighSpeed ? 1 : 0
                opacity: _isLowSpeed ? 0.6 : 1



                QGCColoredImage {
                    height:             parent.height * 3
                    width:              height
                    anchors.centerIn:   parent
                    source:             "/custom/img/h.svg"
                    fillMode:           Image.PreserveAspectFit
                    sourceSize.height:  height
                    color:              _isHighSpeed ? qgcPal.colorGreen : qgcPal.text

                }

                MouseArea {
                    anchors.fill:   parent
                    enabled:        !_isLowSpeed

                    onClicked:      {

                        _isHighSpeed = true
                        setModality()

                    }
                }
            }
        }


        // Central Button
        // Rectangle {
        //     Layout.alignment:   Qt.AlignHCenter
        //     color:              Qt.rgba(0,0,0,0)
        //     width:              ScreenTools.defaultFontPixelWidth * 8
        //     height:             width
        //     radius:             width * 0.5
        //     border.color:       qgcPal.buttonText
        //     border.width:       3


            Rectangle {
                //anchors.centerIn:   parent
                Layout.alignment:   Qt.AlignHCenter
                color:              qgcPal.window
                width:              ScreenTools.defaultFontPixelWidth * 7 //parent.width * 0.85
                height:             width
                radius:             width * 0.5
                border.color:       qgcPal.buttonText
                border.width:       3

            QGCColoredImage {
                height:             _isLowSpeed ? (modality != "Loiter" ? parent.height * 0.5 : parent.height * 1.6 ) :  parent.height * 1.6
                width:              height
                anchors.centerIn:   parent
                source:             _isLowSpeed ? (modality != "Loiter" ? "/custom/img/alert.svg" : "/custom/img/slow.svg" ) :  "/custom/img/slow.svg"
                fillMode:           Image.PreserveAspectFit
                sourceSize.height:  height
                color:              _isLowSpeed ? (modality != "Loiter" ? qgcPal.alertBackground : qgcPal.colorGreen ) : qgcPal.text
            }

            // }

            MouseArea {
                anchors.fill:   parent
                        onClicked: {

                               _isLowSpeed = ! _isLowSpeed

                               if (!_isLowSpeed) {
                                   showCriticalVehicleMessage("LOW SPEED MODE DISABLED")
                                   setModality()
                                   return
                               }

                               if(modality != "Loiter") {
                                   showCriticalVehicleMessage("LOW SPEED MODE NOT AVAILABLE")
                                   // TODO: change icon
                               } else {
                                   showCriticalVehicleMessage("LOW SPEED MODE ACTIVATED")
                               }

                               setModality()
                           }
            }
        }
    }
}
}
