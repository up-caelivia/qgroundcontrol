/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQml.Models                 2.12

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QtQuick                      2.12

import Constants                    1.0
import Custom.Widgets               1.0

ToolStripActionList {
    id: _root
    property bool _SAVenabled: CustomPlugin.isSAVenabled
    property bool _isABLUO: Constants.isABLUOApp
    property bool _SAVexist: CustomPlugin.isSAVexist
    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property var    _vehicleInAir:      _activeVehicle ? _activeVehicle.flying || _activeVehicle.landing : false
    property var    _planMasterController:              globals.planMasterControllerPlanView

    signal displayPreFlightChecklist

    model: [
        ToolStripAction {
            text:           qsTr("Plan")
            iconSource:     "/qmlimages/Plan.svg"
            onTriggered:    mainWindow.showPlanView()
            visible: !_SAVenabled
        },
        PreFlightCheckListShowAction { onTriggered: displayPreFlightChecklist() },
        GuidedActionTakeoff { visible: false && !_SAVenabled && (_guidedController.showTakeoff || !_guidedController.showLand)},
        GuidedActionLand {  visible: false && !_SAVenabled && (_guidedController.showLand && !_guidedController.showTakeoff)},
        GuidedActionRTL {  visible: !_SAVenabled && _guidedController.showRTL},
        GuidedActionPause {  visible: _isABLUO && !_SAVenabled && _guidedController.showPause},
        GuidedActionActionList {  visible: false && !_SAVenabled},
        GuidedActionGripper {  visible: false},
        ToolStripAction {
            text: "SAV GoTo"
            iconSource: "/res/Lifeguard.svg"
            enabled:    true
            visible:    _SAVenabled
            onTriggered: {
                Constants.showSavButtons = !Constants.showSavButtons
            }
        },
        ToolStripAction {
            text: "SAV"
            iconSource: _SAVenabled ? "/res/Enable_white.svg" : "/res/Disable_white.svg"
            enabled:    true
            visible:    _SAVexist && !_vehicleInAir
            onTriggered: {
                CustomPlugin.isSAVenabled = !CustomPlugin.isSAVenabled
                CustomPlugin.updateFence(_planMasterController.geoFenceController, !_SAVenabled);
            }
        },
        ToolStripAction {
            text: "ABLUO"
            iconSource:  "/res/QGCLogoWhite"
            visible:    _isABLUO
            onTriggered:  { CustomPlugin.isAbluoMapPlanEnabled = !CustomPlugin.isAbluoMapPlanEnabled }  
        }

    ]
}
