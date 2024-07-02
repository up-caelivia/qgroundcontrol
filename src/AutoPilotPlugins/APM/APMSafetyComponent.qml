/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick              2.3
import QtQuick.Controls     1.2
import QtGraphicalEffects   1.0
import QtQuick.Layouts      1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

import Constants 1.0
import Custom.Widgets 1.0


import QtQuick 2.15


Loader {
    id: loader
    property bool developer: Constants.developer
    sourceComponent: developer ? safetyFull : safety

    Component {
        id: safety
        Safety {}
    }

    Component {
        id: safetyFull
        SafetyFull {}
    }

}



