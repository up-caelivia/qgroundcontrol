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
import QGroundControl.FactSystem            1.0

RowLayout {
    id:         _root
    spacing:    0

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property var    _vehicleInAir:      _activeVehicle ? _activeVehicle.flying || _activeVehicle.landing : false
    property bool   _vtolInFWDFlight:   _activeVehicle ? _activeVehicle.vtolInFwdFlight : false
    property bool   _armed:             _activeVehicle ? _activeVehicle.armed : false
    property real   _margins:           ScreenTools.defaultFontPixelWidth
    property real   _spacing:           ScreenTools.defaultFontPixelWidth / 2
    property bool   _healthAndArmingChecksSupported: _activeVehicle ? _activeVehicle.healthAndArmingCheckReport.supported : false


    Item {
        id: statusButton
        Layout.preferredWidth:  statusContent.implicitWidth + (_spacing * 2)
        Layout.fillHeight:      true
    
        Rectangle {
            anchors.fill: parent
            radius: ScreenTools.defaultFontPixelHeight * 0.35
            color: statusMouseArea.containsMouse
                    ? Qt.rgba(
                        qgcPal.r,
                        qgcPal.g,
                        qgcPal.b,
                        0.14
                    ) : "transparent"
            Behavior on color {
                ColorAnimation { duration: 100 }
            }
        }

        RowLayout {
            id: statusContent
        
            anchors.fill: parent
            anchors.leftMargin: _spacing
            anchors.rightMargin: _spacing
            spacing: _spacing

            Rectangle {
                id: statusDot

                Layout.preferredWidth: ScreenTools.defaultFontPixelHeight * 0.55
                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 0.55
                Layout.alignment: Qt.AlignVCenter

                radius: width / 2
                color: _mainStatusBGColor

                border.width: 1
                border.color: Qt.rgba(
                    qgcPal.text.r,
                    qgcPal.text.g,
                    qgcPal.text.b,
                    0.35
                )
            }

            QGCLabel {
                id:             mainStatusLabel
                text:           mainStatusText()

                Layout.alignment: Qt.AlignVCenter
                font.family: ScreenTools.demiboldFontFamily
                font.pointSize: ScreenTools.defaultFontPointSize

                property string _commLostText:      qsTr("Link Lost")
                property string _readyToFlyText:    qsTr("Ready To Fly")
                property string _notReadyToFlyText: qsTr("Not Ready")
                property string _disconnectedText:  qsTr("Disconnected")
                property string _armedText:         qsTr("Armed")
                property string _flyingText:        qsTr("Flying")
                property string _landingText:       qsTr("Landing")

                function mainStatusText() {
                    var statusText
                    if (_activeVehicle) {
                        if (_communicationLost) {
                            _mainStatusBGColor = "red"
                            return mainStatusLabel._commLostText
                        }
                        if (_activeVehicle.armed) {
                            _mainStatusBGColor = "green"

                            if (_healthAndArmingChecksSupported) {
                                if (_activeVehicle.healthAndArmingCheckReport.canArm) {
                                    if (_activeVehicle.healthAndArmingCheckReport.hasWarningsOrErrors) {
                                        _mainStatusBGColor = "yellow"
                                    }
                                } else {
                                    _mainStatusBGColor = "red"
                                }
                            }

                            if (_activeVehicle.flying) {
                                return mainStatusLabel._flyingText
                            } else if (_activeVehicle.landing) {
                                return mainStatusLabel._landingText
                            } else {
                                return mainStatusLabel._armedText
                            }
                        } else {
                            if (_healthAndArmingChecksSupported) {
                                if (_activeVehicle.healthAndArmingCheckReport.canArm) {
                                    if (_activeVehicle.healthAndArmingCheckReport.hasWarningsOrErrors) {
                                        _mainStatusBGColor = "yellow"
                                    } else {
                                        _mainStatusBGColor = "green"
                                    }
                                    return mainStatusLabel._readyToFlyText
                                } else {
                                    _mainStatusBGColor = "red"
                                    return mainStatusLabel._notReadyToFlyText
                                }
                            } else if (_activeVehicle.readyToFlyAvailable) {
                                if (_activeVehicle.readyToFly) {
                                    _mainStatusBGColor = "green"
                                    return mainStatusLabel._readyToFlyText
                                } else {
                                    _mainStatusBGColor = "yellow"
                                    return mainStatusLabel._notReadyToFlyText
                                }
                            } else {
                                // Best we can do is determine readiness based on AutoPilot component setup and health indicators from SYS_STATUS
                                if (_activeVehicle.allSensorsHealthy && _activeVehicle.autopilot.setupComplete) {
                                    _mainStatusBGColor = "green"
                                    return mainStatusLabel._readyToFlyText
                                } else {
                                    _mainStatusBGColor = "yellow"
                                    return mainStatusLabel._notReadyToFlyText
                                }
                            }
                        }
                    } else {
                        _mainStatusBGColor = qgcPal.brandingPurple
                        return mainStatusLabel._disconnectedText
                    }
                }
            }

            // Graphical arrow - supported on herelink
            Canvas {
                id: graphicalArrow 
                property real _baseDimension: ScreenTools.defaultFontPointSize
                property bool _isMobile: ScreenTools.isMobile 
                width: _isMobile ? _baseDimension * 1.3 : _baseDimension * 0.7
                height: _isMobile ? _baseDimension * 1.1 : _baseDimension * 0.6
                Layout.alignment: Qt.AlignVCenter
                visible: _activeVehicle

                onPaint: {
                    var ctx = getContext("2d");
                    ctx.clearRect(0, 0, width, height);
                    ctx.fillStyle = qgcPal.text;
                    ctx.beginPath();
                    ctx.moveTo(0, 0);
                    ctx.lineTo(width, 0);
                    ctx.lineTo(width / 2, height);
                    ctx.closePath();
                    ctx.fill();
                }
            }
        }

        QGCMouseArea {
            id: statusMouseArea

            anchors.fill: parent
            enabled: _activeVehicle
            hoverEnabled: !ScreenTools.isMobile
            onClicked: {
                mainWindow.showIndicatorPopup(
                    statusButton,
                    sensorStatusInfoComponent
                )
            }
        }
    }

    // Separator
    Rectangle {
        Layout.preferredWidth: 1
        Layout.preferredHeight: _root.height * 0.6
        Layout.alignment: Qt.AlignVCenter
        Layout.leftMargin: _spacing
        Layout.rightMargin: _spacing
        visible: flightModeMenu.visible

        color: Qt.rgba(
            qgcPal.text.r,
            qgcPal.text.g,
            qgcPal.text.b,
            0.3
        )
    }

    FlightModeMenuIndicator {
        id:                     flightModeMenu
        Layout.preferredHeight: _root.height
        visible:                _activeVehicle
    }

    // Separator
    Item {
        Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * ScreenTools.largeFontPointRatio * 1.5
        height:                 1
        visible:                vtolModeLabel.visible
    }

    // Not our concern atm
    QGCLabel {
        id:                     vtolModeLabel
        Layout.preferredHeight: _root.height
        verticalAlignment:      Text.AlignVCenter
        text:                   _vtolInFWDFlight ? qsTr("FW(vtol)") : qsTr("MR(vtol)")
        font.pointSize:         ScreenTools.largeFontPointSize
        visible:                _activeVehicle ? _activeVehicle.vtol && _vehicleInAir : false

        QGCMouseArea {
            anchors.fill:   parent
            onClicked:      mainWindow.showIndicatorPopup(vtolModeLabel, vtolTransitionComponent)
        }
    }

    Component {
        id: sensorStatusInfoComponent

        Rectangle {
            width:          flickable.width + (_margins * 2)
            height:         flickable.height + (_margins * 2)
            radius:         ScreenTools.defaultFontPixelHeight * 0.5
            color:          qgcPal.window
            border.color:   qgcPal.text

            QGCFlickable {
                id:                 flickable
                anchors.margins:    _margins
                anchors.top:        parent.top
                anchors.left:       parent.left
                width:              mainLayout.width
                height:             mainWindow.contentItem.height - (indicatorPopup.padding * 2) - (_margins * 2)
                flickableDirection: Flickable.VerticalFlick
                contentHeight:      mainLayout.height
                contentWidth:       mainLayout.width

                ColumnLayout {
                    id:         mainLayout
                    spacing:    _spacing

                    QGCButton {
                        Layout.leftMargin:  _healthAndArmingChecksSupported ? width / 2 : 0
                        Layout.alignment:   _healthAndArmingChecksSupported ? Qt.AlignLeft : Qt.AlignHCenter
                        // FIXME: forceArm is not possible anymore if _healthAndArmingChecksSupported == true
                        enabled:            _armed || !_healthAndArmingChecksSupported || _activeVehicle.healthAndArmingCheckReport.canArm
                        text:               _armed ?  qsTr("Disarm") : (forceArm ? qsTr("Force Arm") : qsTr("Arm"))

                        property bool forceArm: false

                        onPressAndHold: forceArm = true

                        onClicked: {
                            if (_armed) {
                                mainWindow.disarmVehicleRequest()
                            } else {
                                if (forceArm) {
                                    mainWindow.forceArmVehicleRequest()
                                } else {
                                    mainWindow.armVehicleRequest()
                                }
                            }
                            forceArm = false
                            mainWindow.hideIndicatorPopup()
                        }
                    }

                    QGCLabel {
                        Layout.alignment:   Qt.AlignHCenter
                        text:               qsTr("Sensor Status")
                        visible:            !_healthAndArmingChecksSupported
                    }

                    GridLayout {
                        rowSpacing:     _spacing
                        columnSpacing:  _spacing
                        rows:           _activeVehicle.sysStatusSensorInfo.sensorNames.length
                        flow:           GridLayout.TopToBottom
                        visible:        !_healthAndArmingChecksSupported

                        Repeater {
                            model: _activeVehicle.sysStatusSensorInfo.sensorNames

                            QGCLabel {
                                text: modelData
                            }
                        }

                        Repeater {
                            model: _activeVehicle.sysStatusSensorInfo.sensorStatus

                            QGCLabel {
                                text: modelData
                            }
                        }
                    }


                    QGCLabel {
                        text:               qsTr("Arming Check Report:")
                        visible:            _healthAndArmingChecksSupported && _activeVehicle.healthAndArmingCheckReport.problemsForCurrentMode.count > 0
                    }
                    // List health and arming checks
                    QGCListView {
                        visible:            _healthAndArmingChecksSupported
                        anchors.margins:    ScreenTools.defaultFontPixelHeight
                        spacing:            ScreenTools.defaultFontPixelWidth
                        width:              mainWindow.width * 0.66666
                        height:             contentHeight
                        model:              _activeVehicle ? _activeVehicle.healthAndArmingCheckReport.problemsForCurrentMode : null
                        delegate:           listdelegate
                    }

                    FactPanelController {
                        id: controller
                    }

                    Component {
                        id: listdelegate

                        Column {
                            width:      parent ? parent.width : 0
                            Row {
                                width:  parent.width
                                QGCLabel {
                                    id:           message
                                    text:         object.message
                                    wrapMode:     Text.WordWrap
                                    textFormat:   TextEdit.RichText
                                    width:        parent.width - arrowDownIndicator.width
                                    color:        object.severity == 'error' ? qgcPal.colorRed : object.severity == 'warning' ? qgcPal.colorOrange : qgcPal.text
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            if (object.description != "")
                                                object.expanded = !object.expanded
                                        }
                                    }
                                }

                                QGCColoredImage {
                                    id:                     arrowDownIndicator
                                    height:                 1.5 * ScreenTools.defaultFontPixelWidth
                                    width:                  height
                                    source:                 "/qmlimages/arrow-down.png"
                                    color:                  qgcPal.text
                                    visible:                object.description != ""
                                    MouseArea {
                                        anchors.fill:       parent
                                        onClicked:          object.expanded = !object.expanded
                                    }
                                }
                            }
                            Rectangle {
                                property var margin:      ScreenTools.defaultFontPixelWidth
                                id:                       descriptionRect
                                width:                    parent.width
                                height:                   description.height + margin
                                color:                    qgcPal.windowShade
                                visible:                  false
                                Connections {
                                    target:               object
                                    function onExpandedChanged() {
                                        if (object.expanded) {
                                            description.height = description.preferredHeight
                                        } else {
                                            description.height = 0
                                        }
                                    }
                                }

                                Behavior on height {
                                    NumberAnimation {
                                        id: animation
                                        duration: 150
                                        onRunningChanged: {
                                            descriptionRect.visible = animation.running || object.expanded
                                        }
                                    }
                                }
                                QGCLabel {
                                    id:                 description
                                    anchors.centerIn:   parent
                                    width:              parent.width - parent.margin * 2
                                    height:             0
                                    text:               object.description
                                    textFormat:         TextEdit.RichText
                                    wrapMode:           Text.WordWrap
                                    clip:               true
                                    property var fact:  null
                                    onLinkActivated: {
                                        if (link.startsWith('param://')) {
                                            var paramName = link.substr(8);
                                            fact = controller.getParameterFact(-1, paramName, true)
                                            if (fact != null) {
                                                paramEditorDialogComponent.createObject(mainWindow).open()
                                            }
                                        } else {
                                            Qt.openUrlExternally(link);
                                        }
                                    }
                                }

                                Component {
                                    id: paramEditorDialogComponent

                                    ParameterEditorDialog {
                                        title:          qsTr("Edit Parameter")
                                        fact:           description.fact
                                        destroyOnClose: true
                                    }
                                }
                            }
                        }
                    }

                }
            }
        }
    }

    Component {
        id: vtolTransitionComponent

        Rectangle {
            width:          mainLayout.width   + (_margins * 2)
            height:         mainLayout.height  + (_margins * 2)
            radius:         ScreenTools.defaultFontPixelHeight * 0.5
            color:          qgcPal.window
            border.color:   qgcPal.text

            QGCButton {
                id:                 mainLayout
                anchors.margins:    _margins
                anchors.top:        parent.top
                anchors.left:       parent.left
                text:               _vtolInFWDFlight ? qsTr("Transition to Multi-Rotor") : qsTr("Transition to Fixed Wing")

                onClicked: {
                    if (_vtolInFWDFlight) {
                        mainWindow.vtolTransitionToMRFlightRequest()
                    } else {
                        mainWindow.vtolTransitionToFwdFlightRequest()
                    }
                    mainWindow.hideIndicatorPopup()
                }
            }
        }
    }
}

