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

Rectangle {
    height:     mainLayout.height + (_margins * 2)
    color:      Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.5)
    radius:     _margins

    property real   _margins:                                   ScreenTools.defaultFontPixelHeight / 2
    property var    _activeVehicle:                             QGroundControl.multiVehicleManager.activeVehicle
    property bool   _secondButton:                    false
    property bool _lowSpeedActive: false

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }


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
                    source:             "/qmlimages/camera_video.svg"
                    fillMode:           Image.PreserveAspectFit
                    sourceSize.height:  height
                    color:              _secondButton ? qgcPal.text : qgcPal.colorGreen
                }

                MouseArea {
                    anchors.fill:   parent
                    enabled:        true
                    onClicked:      {_secondButton = false}
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
                    source:             "/qmlimages/camera_photo.svg"
                    fillMode:           Image.PreserveAspectFit
                    sourceSize.height:  height
                    color:              _secondButton ? qgcPal.colorGreen : qgcPal.text
                }

                MouseArea {
                    anchors.fill:   parent
                    enabled:        true
                    onClicked:      {_secondButton = true}
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

            Rectangle {
                anchors.centerIn:   parent
                width:              parent.width * 0.7
                height:             width
                radius:             width * 0.5
                color:              _lowSpeedActive ? qgcPal.colorRed : qgcPal.colorGrey
            }

            MouseArea {
                anchors.fill:   parent
                onClicked:      {_lowSpeedActive = !_lowSpeedActive}//toggleShooting()
            }
        }


    }

}
