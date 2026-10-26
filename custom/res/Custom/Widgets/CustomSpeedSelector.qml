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
import Custom.Controllers 1.0


Rectangle {
    height:     mainLayout.height + (_margins * 2)
    width:     height

    color:      Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.5)
    radius:     _margins

    property var controller: null
    readonly property var vehicle: controller ? controller.vehicle : null
    property var modality: vehicle ? vehicle.flightMode : ""

    enabled: controller ? controller.ready : false

    property real   _margins:                                   ScreenTools.defaultFontPixelHeight / 2
    property bool   _isHighSpeed:                    false
    property bool   _isLowSpeed:                     false

    Timer {
        id: speedMessageClearTimer
        interval: 2000    // 2 secondi
        repeat: false
        running: false
        onTriggered: {
            CustomPlugin.speedMessage = ""
        }
    }


    function setModality() {
        if (!controller || !controller.ready) {
            return false
        }
        if (_isLowSpeed && modality !== "Loiter") {
            return false
        }
        var mode = _isLowSpeed ? SpeedSelectorController.LowSpeed
            : _isHighSpeed ? SpeedSelectorController.HighSpeed
            : SpeedSelectorController.NormalSpeed
        return controller.requestSpeedMode(mode)
    }

    onVisibleChanged: {
        if (!visible) {
            _isHighSpeed = false
            _isLowSpeed = false
            speedMessageClearTimer.stop()
            CustomPlugin.speedMessage = ""
            }
        }

    onModalityChanged: {
        if(modality != "Loiter" && _isLowSpeed) {
            CustomPlugin.sendLogMessage("Low speed mode not available", "", "Warning")
            CustomPlugin.speedMessage = "Low speed mode not available"
            speedMessageClearTimer.stop()
        }
        if(modality == "Loiter" && _isLowSpeed) {
            if (setModality()) {
                CustomPlugin.sendLogMessage("Low speed mode activated")
                CustomPlugin.speedMessage = "Low speed mode activated"
            } else {
                CustomPlugin.sendLogMessage("Could not apply low speed settings", "", "Warning")
                CustomPlugin.speedMessage = "Could not apply low speed settings"
            }
            speedMessageClearTimer.stop()
        }
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

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
                        var wasHighSpeed = _isHighSpeed
                        _isHighSpeed = false
                        if (!setModality()) {
                            _isHighSpeed = wasHighSpeed
                            CustomPlugin.sendLogMessage("Could not apply normal speed settings", "", "Warning")
                            CustomPlugin.speedMessage = "Could not apply normal speed settings"
                            speedMessageClearTimer.restart()
                            return
                        }
                        CustomPlugin.sendLogMessage("Normal speed mode enabled")
                        CustomPlugin.speedMessage = "Normal Speed mode enabled"
                        speedMessageClearTimer.restart()
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

                        var wasHighSpeed = _isHighSpeed
                        _isHighSpeed = true
                        if (!setModality()) {
                            _isHighSpeed = wasHighSpeed
                            CustomPlugin.sendLogMessage("Could not apply high speed settings", "", "Warning")
                            CustomPlugin.speedMessage = "Could not apply high speed settings"
                            speedMessageClearTimer.restart()
                            return
                        }
                        CustomPlugin.sendLogMessage("High speed mode enabled")
                        CustomPlugin.speedMessage = "High Speed mode enabled"
                        speedMessageClearTimer.restart()

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
                    var wasLowSpeed = _isLowSpeed
                    _isLowSpeed = ! _isLowSpeed
                    if (!_isLowSpeed) {
                        if (!setModality()) {
                            _isLowSpeed = wasLowSpeed
                            CustomPlugin.sendLogMessage("Could not restore speed settings", "", "Warning")
                            CustomPlugin.speedMessage = "Could not restore speed settings"
                            speedMessageClearTimer.restart()
                            return
                        }
                        CustomPlugin.sendLogMessage("Low speed mode disabled")
                        CustomPlugin.speedMessage = "Low speed mode disabled"
                        speedMessageClearTimer.restart()
                        return
                    }
                    if(modality != "Loiter") {
                        CustomPlugin.sendLogMessage("Low speed mode not available", "", "Warning")
                        CustomPlugin.speedMessage = "Low speed mode not available"
                    } else {
                        if (!setModality()) {
                            _isLowSpeed = wasLowSpeed
                            CustomPlugin.sendLogMessage("Could not apply low speed settings", "", "Warning")
                            CustomPlugin.speedMessage = "Could not apply low speed settings"
                            speedMessageClearTimer.restart()
                            return
                        }
                        CustomPlugin.sendLogMessage("Low speed mode activated")
                        CustomPlugin.speedMessage = "Low speed mode activated"
                    }
                    speedMessageClearTimer.stop()
                }
            }
        }
    }
}
}
