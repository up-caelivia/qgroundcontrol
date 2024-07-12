/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.11
import QtQuick.Layouts  1.11

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

import Constants  1.0


//-------------------------------------------------------------------------
//-- GPS Indicator
Item {
    id:             _root
    width:          (gpsValuesColumn.x + gpsValuesColumn.width) * 1.1
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property bool showIndicator: true
    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    Component {
        id: gpsInfo

        Rectangle {
            width:  gpsCol.width   + ScreenTools.defaultFontPixelWidth  * 3
            height: gpsCol.height  + ScreenTools.defaultFontPixelHeight * 2
            radius: ScreenTools.defaultFontPixelHeight * 0.5
            color:  qgcPal.window
            border.color:   qgcPal.text

            Column {
                id:                 gpsCol
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                width:              Math.max(gpsGrid.width, gpsLabel.width)
                anchors.margins:    ScreenTools.defaultFontPixelHeight
                anchors.centerIn:   parent

                QGCLabel {
                    id:             gpsLabel
                    text:           (_activeVehicle && _activeVehicle.gps.count.value >= 0) ? qsTr("GPS Status") : qsTr("GPS Data Unavailable")
                    font.family:    ScreenTools.demiboldFontFamily
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                GridLayout {
                    id:                 gpsGrid
                    visible:            (_activeVehicle && _activeVehicle.gps.count.value >= 0)
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    columnSpacing:      ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    columns: 2

                    QGCLabel { text: qsTr("GPS Count:") }
                    QGCLabel { text: _activeVehicle ? _activeVehicle.gps.count.valueString : qsTr("N/A", "No data to display") }
                    QGCLabel { text: qsTr("GPS Lock:") }
                    QGCLabel { text: _activeVehicle ? _activeVehicle.gps.lock.enumStringValue : qsTr("N/A", "No data to display") }
                    QGCLabel { text: qsTr("HDOP:") }
                    QGCLabel { text: _activeVehicle ? _activeVehicle.gps.hdop.valueString : qsTr("--.--", "No data to display") }
                    QGCLabel { text: qsTr("VDOP:") }
                    QGCLabel { text: _activeVehicle ? _activeVehicle.gps.vdop.valueString : qsTr("--.--", "No data to display") }
                    QGCLabel { text: qsTr("Course Over Ground:") }
                    QGCLabel { text: _activeVehicle ? _activeVehicle.gps.courseOverGround.valueString : qsTr("--.--", "No data to display") }
                }
            }
        }
    }

    QGCColoredImage {
        id:                 gpsIcon
        width:              height
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        source:             "/qmlimages/Gps.svg"
        fillMode:           Image.PreserveAspectFit
        sourceSize.height:  height
        opacity:            (_activeVehicle && _activeVehicle.gps.count.value >= 0) ? 1 : 0.5
        color:              getColorImage()
    }


    function getColorImage() {

        if( _activeVehicle && _activeVehicle.gps.lock.rawValue === 0)  // No GPS https://mavlink.io/en/messages/common.html
            return qgcPal.colorRed;

        if( _activeVehicle && _activeVehicle.gps.lock.rawValue === 1)  // No fix
            return qgcPal.colorOrange;

        if ( _activeVehicle && (_activeVehicle.gps.lock.rawValue === 6 || _activeVehicle.gps.lock.rawValue === 5) )  // RTK fixed / RTK Float
            return qgcPal.colorBlue;

        return qgcPal.colorGreen;  // 3d/2d fix etc...

    }

    Column {
        id:                     gpsValuesColumn
        anchors.verticalCenter: parent.verticalCenter
        anchors.left:           gpsIcon.right
        // anchors.leftMargin:
        leftPadding: ScreenTools.defaultFontPixelHeight
        spacing: ScreenTools.defaultFontPixelHeight * 0.1

        QGCLabel {
            anchors.horizontalCenter:   gpsValuesColumn.horizontalCenter
            visible:                    _activeVehicle
            color:                      qgcPal.buttonText
            text:                       _activeVehicle ? _activeVehicle.gps.count.valueString : ""
        }

        Rectangle {
            id: statusButton
            anchors.horizontalCenter:   gpsValuesColumn.horizontalCenter
            width: 8
            height: 8
            radius: 4
            visible: Constants.ntripEnabled
            color: Constants.ntripReceiving ? "green" : "red"

        }

    }

    MouseArea {
        anchors.fill:   parent
        onClicked: {
            mainWindow.showIndicatorPopup(_root, gpsInfo)
        }
    }
}
