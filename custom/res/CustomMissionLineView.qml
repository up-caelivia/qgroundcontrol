/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

// spessore linee inside

import QtQuick          2.3
import QtLocation       5.3
import QtPositioning    5.3

import QGroundControl           1.0
import QGroundControl.Palette   1.0

/// The MissionLineView control is used to add lines between mission items
MapItemView {
    property bool showSpecialVisual: false
    delegate: MapPolyline {

        line.width: 4
        // Note: Special visuals for ROI are hacked out for now since they are not working correctly
        line.color: _terrainCollision ?
                        "#FF0000" :
                        (false/*showSpecialVisual*/ ? "green" : "#FFA500")//QGroundControl.globalPalette.mapMissionTrajectory)
        z:          QGroundControl.zOrderWaypointLines
        path:       object && object.coordinate1.isValid && object.coordinate2.isValid ? [ object.coordinate1, object.coordinate2 ] : []

        property bool _terrainCollision:    object && object.terrainCollision
        property bool _showSpecialVisual:   object && showSpecialVisual && object.specialVisual
    }
}
