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

    function secondsToHHMMSS(timeS) {
        var sec_num = parseInt(timeS, 10);
        var hours   = Math.floor(sec_num / 3600);
        var minutes = Math.floor((sec_num - (hours * 3600)) / 60);
        var seconds = sec_num - (hours * 3600) - (minutes * 60);
        if (hours   < 10) {hours   = "0"+hours;}
        if (minutes < 10) {minutes = "0"+minutes;}
        if (seconds < 10) {seconds = "0"+seconds;}
        return hours+":"+ minutes+':'+seconds;
    }


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
            text: "Flight Time: " + ( _activeVehicle ?  _activeVehicle.getFact("flightTime").valueString : "--/--")
        }

        QGCLabel {
            color:                      qgcPal.buttonText
            text: "Time Remaining: " + (_activeVehicle ? getTimeRemaningEstimate() : "--/--")


            function getTimeRemaningEstimate() {

                var time_remaining_tot = 0;
                var i = 0;

                for (var battery in _activeVehicle.batteries)
                    if( !isNaN(battery.timeRemaining) ) {

                        time_remaining_tot += battery.timeRemaining.rawValue;
                        i++;

                    }

                if (i==0)
                    return "--/--";

                return _root.secondsToHHMMSS(time_remaining/i);

            }
        }



    }



}
