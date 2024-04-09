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

//-------------------------------------------------------------------------
//-- Flight Time Indicator
Item {
    id:             _root
    width:          (ftValuesColumn.x + ftValuesColumn.width) * 1.1
    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle


    QGCColoredImage {
        id:                 ftIcon
        width:              height
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        // anchors.topMargin: 7
        // anchors.bottomMargin: 7
        source:             "/InstrumentValueIcons/timer.svg"
        fillMode:           Image.PreserveAspectFit
        sourceSize.height:  height
        opacity:            (_activeVehicle && _activeVehicle.getFact("flightTime").value > 0) ? 1 : 0.5
    }

    Column {
        id:                     ftValuesColumn
        anchors.verticalCenter: parent.verticalCenter
        anchors.left:           ftIcon.right
        leftPadding: 10

        QGCLabel {
            color:                      qgcPal.buttonText
            text: _activeVehicle ? _activeVehicle.getFact("flightTime").valueString : ""
        }
    }

}
