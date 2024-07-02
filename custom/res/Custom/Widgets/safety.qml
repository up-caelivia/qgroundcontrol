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

SetupPage {
    id:             safetyPage
    pageComponent:  safetyPageComponent

    Component {
        id: safetyPageComponent

        Flow {
            id:         flowLayout
            width:      availableWidth
            spacing:    _margins

            FactPanelController { id: controller; }

            QGCPalette { id: ggcPal; colorGroupEnabled: true }

            property Fact _batt1Monitor:                    controller.getParameterFact(-1, "BATT_MONITOR")
            property Fact _batt2Monitor:                    controller.getParameterFact(-1, "BATT2_MONITOR", false /* reportMissing */)
            property bool _batt2MonitorAvailable:           controller.parameterExists(-1, "BATT2_MONITOR")
            property bool _batt1MonitorEnabled:             _batt1Monitor.rawValue !== 0
            property bool _batt2MonitorEnabled:             _batt2MonitorAvailable ? _batt2Monitor.rawValue !== 0 : false
            property bool _batt1ParamsAvailable:            controller.parameterExists(-1, "BATT_CAPACITY")
            property bool _batt2ParamsAvailable:            controller.parameterExists(-1, "BATT2_CAPACITY")

            property Fact _failsafeBatt1LowAct:             controller.getParameterFact(-1, "BATT_FS_LOW_ACT", false /* reportMissing */)
            property Fact _failsafeBatt2LowAct:             controller.getParameterFact(-1, "BATT2_FS_LOW_ACT", false /* reportMissing */)
            property Fact _failsafeBatt1CritAct:            controller.getParameterFact(-1, "BATT_FS_CRT_ACT", false /* reportMissing */)
            property Fact _failsafeBatt2CritAct:            controller.getParameterFact(-1, "BATT2_FS_CRT_ACT", false /* reportMissing */)
            property Fact _failsafeBatt1LowMah:             controller.getParameterFact(-1, "BATT_LOW_MAH", false /* reportMissing */)
            property Fact _failsafeBatt2LowMah:             controller.getParameterFact(-1, "BATT2_LOW_MAH", false /* reportMissing */)
            property Fact _failsafeBatt1CritMah:            controller.getParameterFact(-1, "BATT_CRT_MAH", false /* reportMissing */)
            property Fact _failsafeBatt2CritMah:            controller.getParameterFact(-1, "BATT2_CRT_MAH", false /* reportMissing */)
            property Fact _failsafeBatt1LowVoltage:         controller.getParameterFact(-1, "BATT_LOW_VOLT", false /* reportMissing */)
            property Fact _failsafeBatt2LowVoltage:         controller.getParameterFact(-1, "BATT2_LOW_VOLT", false /* reportMissing */)
            property Fact _failsafeBatt1CritVoltage:        controller.getParameterFact(-1, "BATT_CRT_VOLT", false /* reportMissing */)
            property Fact _failsafeBatt2CritVoltage:        controller.getParameterFact(-1, "BATT2_CRT_VOLT", false /* reportMissing */)

            property Fact _armingCheck: controller.getParameterFact(-1, "ARMING_CHECK")

            property real _margins:         ScreenTools.defaultFontPixelHeight
            property real _innerMargin:     _margins / 2
            property bool _showIcon:        !ScreenTools.isTinyScreen
            property bool _roverFirmware:   controller.parameterExists(-1, "MODE1") // This catches all usage of ArduRover firmware vehicle types: Rover, Boat...


            property string _restartRequired: qsTr("Requires vehicle reboot")


            Component {
                id: restartRequiredComponent

                ColumnLayout {
                    spacing: ScreenTools.defaultFontPixelWidth

                    QGCLabel {
                        text: _restartRequired
                    }

                    QGCButton {
                        text:       qsTr("Reboot vehicle")
                        onClicked:  controller.vehicle.rebootVehicle()
                    }
                }
            }


            Component {
                id: copterGeoFence

                Column {
                    spacing: _margins / 2

                    property Fact _fenceAction: controller.getParameterFact(-1, "FENCE_ACTION")
                    property Fact _fenceAltMax: controller.getParameterFact(-1, "FENCE_ALT_MAX")
                    property Fact _fenceEnable: controller.getParameterFact(-1, "FENCE_ENABLE")
                    property Fact _fenceMargin: controller.getParameterFact(-1, "FENCE_MARGIN")
                    property Fact _fenceRadius: controller.getParameterFact(-1, "FENCE_RADIUS")
                    property Fact _fenceType:   controller.getParameterFact(-1, "FENCE_TYPE")

                    readonly property int _maxAltitudeFenceBitMask: 1
                    readonly property int _circleFenceBitMask:      2
                    readonly property int _polygonFenceBitMask:     4

                    QGCLabel {
                        text:           qsTr("GeoFence")
                        font.family:    ScreenTools.demiboldFontFamily
                    }

                    Rectangle {
                        width:  mainLayout.width + (_margins * 2)
                        height: mainLayout.height + (_margins * 2)
                        color:  ggcPal.windowShade

                        ColumnLayout {
                            id:         mainLayout
                            x:          _margins
                            y:          _margins
                            spacing:    ScreenTools.defaultFontPixellHeight / 2

                            FactCheckBox {
                                id:     enabledCheckBox
                                text:   qsTr("Enabled")
                                fact:   _fenceEnable
                            }

                            GridLayout {
                                columns:    2
                                enabled:    enabledCheckBox.checked

                                QGCCheckBox {
                                    text:       qsTr("Maximum Altitude")
                                    checked:    _fenceType.rawValue & _maxAltitudeFenceBitMask

                                    onClicked: {
                                        if (checked) {
                                            _fenceType.rawValue |= _maxAltitudeFenceBitMask
                                        } else {
                                            _fenceType.value &= ~_maxAltitudeFenceBitMask
                                        }
                                    }
                                }

                                FactTextField {
                                    fact: _fenceAltMax
                                }

                                QGCCheckBox {
                                    text:       qsTr("Circle centered on Home")
                                    checked:    _fenceType.rawValue & _circleFenceBitMask

                                    onClicked: {
                                        if (checked) {
                                            _fenceType.rawValue |= _circleFenceBitMask
                                        } else {
                                            _fenceType.value &= ~_circleFenceBitMask
                                        }
                                    }
                                }

                                FactTextField {
                                    fact:       _fenceRadius
                                    showUnits:  true
                                }

                                QGCCheckBox {
                                    text:       qsTr("Inclusion/Exclusion Circles+Polygons")
                                    checked:    _fenceType.rawValue & _polygonFenceBitMask

                                    onClicked: {
                                        if (checked) {
                                            _fenceType.rawValue |= _polygonFenceBitMask
                                        } else {
                                            _fenceType.value &= ~_polygonFenceBitMask
                                        }
                                    }
                                }

                                Item {
                                    height: 1
                                    width:  1
                                }
                            } // GridLayout

                            Item {
                                height: 1
                                width:  1
                            }

                            GridLayout {
                                columns: 2
                                enabled: enabledCheckBox.checked

                                QGCLabel {
                                    text: qsTr("Breach action")
                                }

                                FactComboBox {
                                    sizeToContents: true
                                    fact:           _fenceAction
                                }

                                QGCLabel {
                                    text: qsTr("Fence margin")
                                }

                                FactTextField {
                                    fact: _fenceMargin
                                }
                            }
                        }
                    } // Rectangle - GeoFence Settings
                } // Column - GeoFence Settings
            }

            Loader {
                sourceComponent: controller.vehicle.multiRotor ? copterGeoFence : undefined
            }

            Component {
                id: copterRTL

                Column {
                    spacing: _margins / 2

                    property Fact _landSpeedFact:   controller.getParameterFact(-1, "LAND_SPEED")
                    property Fact _rtlAltFact:      controller.getParameterFact(-1, "RTL_ALT")
                    property Fact _rtlLoitTimeFact: controller.getParameterFact(-1, "RTL_LOIT_TIME")
                    property Fact _rtlAltFinalFact: controller.getParameterFact(-1, "RTL_ALT_FINAL")

                    QGCLabel {
                        id:             rtlLabel
                        text:           qsTr("Return to Launch")
                        font.family:    ScreenTools.demiboldFontFamily
                    }

                    Rectangle {
                        id:     rtlSettings
                        width:  landSpeedField.x + landSpeedField.width + _margins
                        height: landSpeedField.y + landSpeedField.height + _margins
                        color:  ggcPal.windowShade

                        Image {
                            id:                 icon
                            anchors.margins:    _margins
                            anchors.left:       parent.left
                            anchors.top:        parent.top
                            height:             ScreenTools.defaultFontPixelWidth * 20
                            width:              ScreenTools.defaultFontPixelWidth * 20
                            sourceSize.width:   width
                            mipmap:             true
                            fillMode:           Image.PreserveAspectFit
                            visible:            false
                            source:             "/qmlimages/ReturnToHomeAltitude.svg"
                        }

                        ColorOverlay {
                            anchors.fill:   icon
                            source:         icon
                            color:          ggcPal.text
                            visible:        _showIcon
                        }

                        QGCRadioButton {
                            id:                 returnAtCurrentRadio
                            anchors.margins:    _innerMargin
                            anchors.left:       _showIcon ? icon.right : parent.left
                            anchors.top:        parent.top
                            text:               qsTr("Return at current altitude")
                            checked:            _rtlAltFact.value == 0

                            onClicked: _rtlAltFact.value = 0
                        }

                        QGCRadioButton {
                            id:                 returnAltRadio
                            anchors.topMargin:  _innerMargin
                            anchors.top:        returnAtCurrentRadio.bottom
                            anchors.left:       returnAtCurrentRadio.left
                            text:               qsTr("Return at specified altitude:")
                            checked:            _rtlAltFact.value != 0

                            onClicked: _rtlAltFact.value = 1500
                        }

                        FactTextField {
                            id:                 rltAltField
                            anchors.leftMargin: _margins
                            anchors.left:       returnAltRadio.right
                            anchors.baseline:   returnAltRadio.baseline
                            fact:               _rtlAltFact
                            showUnits:          true
                            enabled:            returnAltRadio.checked
                        }

                        QGCCheckBox {
                            id:                 homeLoiterCheckbox
                            anchors.left:       returnAtCurrentRadio.left
                            anchors.baseline:   landDelayField.baseline
                            checked:            _rtlLoitTimeFact.value > 0
                            text:               qsTr("Loiter above Home for:")

                            onClicked: _rtlLoitTimeFact.value = (checked ? 60 : 0)
                        }

                        FactTextField {
                            id:                 landDelayField
                            anchors.topMargin:  _innerMargin
                            anchors.left:       rltAltField.left
                            anchors.top:        rltAltField.bottom
                            fact:               _rtlLoitTimeFact
                            showUnits:          true
                            enabled:            homeLoiterCheckbox.checked === true
                        }

                        QGCLabel {
                            anchors.left:       returnAtCurrentRadio.left
                            anchors.baseline:   rltAltFinalField.baseline
                            text:               qsTr("Final land stage altitude:")
                        }

                        FactTextField {
                            id:                 rltAltFinalField
                            anchors.topMargin:  _innerMargin
                            anchors.left:       rltAltField.left
                            anchors.top:        landDelayField.bottom
                            fact:               _rtlAltFinalFact
                            showUnits:          true
                        }

                        QGCLabel {
                            anchors.left:       returnAtCurrentRadio.left
                            anchors.baseline:   landSpeedField.baseline
                            text:               qsTr("Final land stage descent speed:")
                        }

                        FactTextField {
                            id:                 landSpeedField
                            anchors.topMargin: _innerMargin
                            anchors.left:       rltAltField.left
                            anchors.top:        rltAltFinalField.bottom
                            fact:               _landSpeedFact
                            showUnits:          true
                        }
                    } // Rectangle - RTL Settings
                } // Column - RTL Settings
            }

            Loader {
                sourceComponent: controller.vehicle.multiRotor ? copterRTL : undefined
            }


        } // Flow
    } // Component - safetyPageComponent
} // SetupView
