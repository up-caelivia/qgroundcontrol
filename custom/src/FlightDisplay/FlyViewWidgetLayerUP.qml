/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.12
import QtQuick.Window           2.2

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

import Custom.Widgets 1.0
import Custom.Controllers 1.0
import Constants 1.0


Item {
    id: _root

    property var    parentToolInsets
    property var    totalToolInsets:        _totalToolInsets
    property var    mapControl
    property real   cockpitCenterY
    property real   cockpitHeight
    readonly property bool instrumentPanelVisible: QGroundControl.corePlugin.options.flyView.showInstrumentPanel && multiVehiclePanelSelector.showSingleVehiclePanel

    property var    _activeVehicle:         QGroundControl.multiVehicleManager.activeVehicle
    property var    _planMasterController:  globals.planMasterControllerFlyView
    property var    _missionController:     _planMasterController.missionController
    property var    _geoFenceController:    _planMasterController.geoFenceController
    property var    _rallyPointController:  _planMasterController.rallyPointController
    property var    _guidedController:      globals.guidedControllerFlyView
    property real   _toolsMargin:           ScreenTools.defaultFontPixelWidth * 0.75
    property real   _rightPanelWidth:       ScreenTools.defaultFontPixelWidth * 30
    property alias  _gripperMenu:           gripperOptions

    readonly property real rightControlWidth: Math.max(
        ScreenTools.minTouchPixels,
        ScreenTools.defaultFontPixelWidth * 7,
        ScreenTools.defaultFontPixelHeight * 2
    )

    readonly property real _rightControlMargin:
        _toolsMargin + Math.max(
            parentToolInsets.rightEdgeCenterInset,
            parentToolInsets.rightEdgeBottomInset
        )


    QGCToolInsets {
        id:                     _totalToolInsets

        leftEdgeTopInset:       toolStrip.leftEdgeTopInset
        leftEdgeCenterInset:    parentToolInsets.leftEdgeCenterInset
        leftEdgeBottomInset:    parentToolInsets.leftEdgeBottomInset

        rightEdgeTopInset:      Math.max(
                                    parentToolInsets.rightEdgeTopInset,
                                    mapScale.rightEdgeTopInset,
                                    multiVehiclePanelSelector.rightEdgeTopInset,
                                    multiVehicleList.rightEdgeTopInset
                                )
        rightEdgeCenterInset: Math.max(
            parentToolInsets.rightEdgeCenterInset,
            photoVideoControl.visible
                ? _root._rightControlMargin + photoVideoControl.occupiedWidth
                : 0
        )
        rightEdgeBottomInset: Math.max(
            parentToolInsets.rightEdgeBottomInset,
            speedControl.visible && speedControl.enabled
                ? _root._rightControlMargin + _root.rightControlWidth
                : 0
        )

        topEdgeLeftInset:       toolStrip.topEdgeLeftInset
        topEdgeCenterInset:     Math.max(
                                    parentToolInsets.topEdgeCenterInset,
                                    flightTimePanel.visible
                                        ? flightTimePanel.y + flightTimePanel.height + _toolsMargin
                                        : 0,
                                    guidedActionConfirm.visible
                                        ? guidedActionConfirm.y + guidedActionConfirm.height + _toolsMargin
                                        : 0
                                )
        topEdgeRightInset:      Math.max(
                                    parentToolInsets.topEdgeRightInset,
                                    mapScale.topEdgeRightInset,
                                    multiVehiclePanelSelector.topEdgeRightInset,
                                    multiVehicleList.topEdgeRightInset
                                )

        bottomEdgeLeftInset:    parentToolInsets.bottomEdgeLeftInset
        bottomEdgeRightInset: Math.max(
            parentToolInsets.bottomEdgeRightInset,
            speedControl.visible && speedControl.enabled
                ? _root.height - speedControl.y : 0
        )
        bottomEdgeCenterInset: parentToolInsets.bottomEdgeCenterInset
    }


    FlyViewMissionCompleteDialog {
        missionController:      _missionController
        geoFenceController:     _geoFenceController
        rallyPointController:   _rallyPointController
    }

    Row {
        id:                 multiVehiclePanelSelector
        anchors.margins:    _toolsMargin
        anchors.topMargin:  _toolsMargin + Math.max(parentToolInsets.topEdgeRightInset, mapScale.topEdgeRightInset)
        anchors.top:        parent.top
        anchors.right:      parent.right
        width:              _rightPanelWidth
        spacing:            ScreenTools.defaultFontPixelWidth
        visible:            QGroundControl.multiVehicleManager.vehicles.count > 1 && QGroundControl.corePlugin.options.flyView.showMultiVehicleList

        property bool showSingleVehiclePanel:  !visible || singleVehicleRadio.checked
        property real topEdgeRightInset: visible ? y + height : 0
        property real rightEdgeTopInset: visible ? parent.width - x : 0

        QGCMapPalette { id: mapPal; lightColors: true }

        QGCRadioButton {
            id:             singleVehicleRadio
            text:           qsTr("Single")
            checked:        true
            textColor:      mapPal.text
        }

        QGCRadioButton {
            text:           qsTr("Multi-Vehicle")
            textColor:      mapPal.text
        }
    }

    MultiVehicleList {
        id: multiVehicleList

        anchors.topMargin:  _toolsMargin
        anchors.top:        multiVehiclePanelSelector.bottom
        anchors.right:      parent.right
        width:              _rightPanelWidth
        height:             parent.height - y - _toolsMargin
        visible:            !multiVehiclePanelSelector.showSingleVehiclePanel

        property real topEdgeRightInset: visible ? y + height : 0
        property real rightEdgeTopInset: visible ? _root.width - x : 0
    }

    FlightTimeWidget {
        id: flightTimePanel

        anchors.top: parent.top
        anchors.topMargin: parentToolInsets.topEdgeCenterInset + _toolsMargin
        anchors.horizontalCenter: parent.horizontalCenter

        vehicle: _root._activeVehicle
        flightTimeSeconds: vehicle ? CustomPlugin.tripFlightTimeSec : NaN

        visible: !!vehicle
                    && !vehicle.vehicleLinkManager.communicationLost
                    && multiVehiclePanelSelector.showSingleVehiclePanel
                    && !guidedActionConfirm.visible
    }

    GuidedActionConfirm {
        id: guidedActionConfirm
        anchors.margins:            _toolsMargin
        anchors.top:                parent.top
        anchors.horizontalCenter:   parent.horizontalCenter
        z:                          QGroundControl.zOrderTopMost
        guidedController:           _guidedController
        guidedValueSlider:          _guidedValueSlider
    }

    //-- Virtual Joystick
    /* TBD: for the moment this is an unwanted functionality, commented for future development
    Loader {
        id:                         virtualJoystickMultiTouch
        z:                          QGroundControl.zOrderTopMost + 1
        width:                      parent.width  - (_pipOverlay.width / 2)
        height:                     Math.min(parent.height * 0.25, ScreenTools.defaultFontPixelWidth * 16)
        visible:                    _virtualJoystickEnabled && !QGroundControl.videoManager.fullScreen && !(_activeVehicle ? _activeVehicle.usingHighLatencyLink : false)
        anchors.bottom:             parent.bottom
        anchors.bottomMargin:       parentToolInsets.bottomEdgeLeftInset + ScreenTools.defaultFontPixelHeight * 2
        anchors.horizontalCenter:   parent.horizontalCenter
        source:                     "qrc:/qml/VirtualJoystick.qml"
        active:                     _virtualJoystickEnabled && !(_activeVehicle ? _activeVehicle.usingHighLatencyLink : false)

        property bool autoCenterThrottle: QGroundControl.settingsManager.appSettings.virtualJoystickAutoCenterThrottle.rawValue

        property bool _virtualJoystickEnabled: QGroundControl.settingsManager.appSettings.virtualJoystick.rawValue

        property real bottomEdgeLeftInset: parent.height-y
        property real bottomEdgeRightInset: parent.height-y

        // Width is difficult to access directly hence this hack which may not work in all circumstances
        property real leftEdgeBottomInset: visible ? bottomEdgeLeftInset + width/18 - ScreenTools.defaultFontPixelHeight*2 : 0
        property real rightEdgeBottomInset: visible ? bottomEdgeRightInset + width/18 - ScreenTools.defaultFontPixelHeight*2 : 0
    }
    */

    FlyViewToolStrip {
        id:                     toolStrip
        anchors.leftMargin:     _toolsMargin + parentToolInsets.leftEdgeCenterInset
        anchors.topMargin:      _toolsMargin + parentToolInsets.topEdgeLeftInset
        anchors.left:           parent.left
        anchors.top:            parent.top
        z:                      QGroundControl.zOrderWidgets
        maxHeight:              parent.height - y - parentToolInsets.bottomEdgeLeftInset - _toolsMargin
        visible:                CustomPlugin.isSAVenabled || !QGroundControl.videoManager.fullScreen
    

        onDisplayPreFlightChecklist: preFlightChecklistPopup.createObject(mainWindow).open()

        property real topEdgeLeftInset: visible ? y + height : 0
        property real leftEdgeTopInset: visible ? x + width : 0
    }

    GripperMenu {
        id: gripperOptions
    }

    VehicleWarnings {
        anchors.centerIn:   parent
        z:                  QGroundControl.zOrderTopMost
    }

    MapScale {
        id:                 mapScale
        anchors.margins:    _toolsMargin
        anchors.right:      parent.right
        anchors.top:        parent.top
        mapControl:         _root.mapControl
        buttonsOnLeft:      false
        visible:            !ScreenTools.isTinyScreen && QGroundControl.corePlugin.options.flyView.showMapScale && mapControl.pipState.state === mapControl.pipState.fullState
        navigationController: _root.mapControl ? _root.mapControl.navigationController : null

        property real rightEdgeTopInset: visible ? parent.width - mapScale.x : 0
        property real topEdgeRightInset: visible ? y + height : 0
    }


    Component {
        id: preFlightChecklistPopup
        FlyViewPreFlightChecklistPopup {
        }
    }

    SpeedSelectorController {
        id: speedController

        vehicle: QGroundControl.multiVehicleManager.activeVehicleAvailable
            ? _root._activeVehicle : null
    }

    SpeedSelector {
        id: speedControl

        anchors.right: parent.right
        anchors.rightMargin: _root._rightControlMargin

        height: _root.cockpitHeight * (ScreenTools.isMobile ? 1 : 0.75)
        y: _root.cockpitCenterY - height / 2

        controller: speedController

        visible: speedController.ready
            && multiVehiclePanelSelector.showSingleVehiclePanel
    }

    PhotoVideoControl {
        id: photoVideoControl

        anchors.right: parent.right
        anchors.rightMargin: _root._rightControlMargin
        buttonSize: _root.rightControlWidth

        readonly property real _availableTop: _toolsMargin + Math.max(
            _totalToolInsets.topEdgeRightInset,
            parentToolInsets.topEdgeCenterInset
        )

        readonly property real _availableBottom: speedControl.visible && speedControl.enabled
            ? speedControl.y - _toolsMargin
            : _root.height - parentToolInsets.bottomEdgeRightInset - _toolsMargin

        height: Math.min(
            implicitHeight,
            Math.max(0, _availableBottom, _availableTop)
        )
        y: Math.max(
            _availableTop,
            Math.min(
                mainWindow.height / 2 - mainWindow.contentItem.y - height / 2,
                _availableBottom - height
            )
        )
    }

    SavButtonWidgetUP {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin:      _toolsMargin + Screen.height * 0.2
        anchors.top:            parent.top
        z:                      QGroundControl.zOrderWidgets
        maxHeight:              parent.height - y - parentToolInsets.bottomEdgeLeftInset - _toolsMargin
        visible:                Constants.showSavButtons
    }   

    AbluoPathWidgetUP {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin:      _toolsMargin + Screen.height * 0.2
        anchors.top:            parent.top
        z:                      QGroundControl.zOrderWidgets
        maxHeight:              parent.height - y - parentToolInsets.bottomEdgeLeftInset - _toolsMargin
        visible:                CustomPlugin.isAbluoMapPlanEnabled
    }      

}
