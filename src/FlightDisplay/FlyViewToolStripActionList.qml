/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQml.Models 2.12

import QGroundControl           1.0
import QGroundControl.Controls  1.0

import Constants 1.0

ToolStripActionList {
    id: _root

    signal displayPreFlightChecklist

    model: [
        ToolStripAction {
            text:           qsTr("Plan")
            iconSource:     "/qmlimages/Plan.svg"
            onTriggered:    mainWindow.showPlanView()
        },
        PreFlightCheckListShowAction { onTriggered: displayPreFlightChecklist() },
        GuidedActionTakeoff { },
        GuidedActionLand { },
        GuidedActionRTL { },
        GuidedActionPause { },
        GuidedActionActionList { },
        GuidedActionGripper { },
        ToolStripAction {
            text: "SAV Btns"
            iconSource: "/res/QGCLogoWhite" //"/qmlimages/MapAddMission.svg"
            enabled:    true
            visible:    QGroundControl.parameterManager.parameterExists(vehicle, "SAV_NUM_TORR") &&
                        QGroundControl.parameterManager.getParameter(vehicle, "SAV_NUM_TORR").value > 0
            onTriggered: {
                Constants.showSavButtons = !Constants.showSavButtons
            }
        }
    ]
}
