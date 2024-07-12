/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtQuick.Layouts          1.2

import QGroundControl                       1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.FactControls          1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Palette               1.0
import QGroundControl.Controllers           1.0
import QGroundControl.SettingsManager       1.0

Rectangle {
    id:                 _root
    color:              qgcPal.window
    anchors.fill:       parent
    anchors.margins:    ScreenTools.defaultFontPixelWidth

    property Fact _savePath:                            QGroundControl.settingsManager.appSettings.savePath
    property Fact _appFontPointSize:                    QGroundControl.settingsManager.appSettings.appFontPointSize
    property Fact _userBrandImageIndoor:                QGroundControl.settingsManager.brandImageSettings.userBrandImageIndoor
    property Fact _userBrandImageOutdoor:               QGroundControl.settingsManager.brandImageSettings.userBrandImageOutdoor
    property Fact _virtualJoystick:                     QGroundControl.settingsManager.appSettings.virtualJoystick
    property Fact _virtualJoystickAutoCenterThrottle:   QGroundControl.settingsManager.appSettings.virtualJoystickAutoCenterThrottle

    property real   _labelWidth:                ScreenTools.defaultFontPixelWidth * 20
    property real   _comboFieldWidth:           ScreenTools.defaultFontPixelWidth * 30
    property real   _valueFieldWidth:           ScreenTools.defaultFontPixelWidth * 10 * 2
    property string _mapProvider:               QGroundControl.settingsManager.flightMapSettings.mapProvider.value
    property string _mapType:                   QGroundControl.settingsManager.flightMapSettings.mapType.value
    property Fact   _followTarget:              QGroundControl.settingsManager.appSettings.followTarget
    property real   _panelWidth:                _root.width * _internalWidthRatio
    property real   _margins:                   ScreenTools.defaultFontPixelWidth
    property var    _planViewSettings:          QGroundControl.settingsManager.planViewSettings
    property var    _flyViewSettings:           QGroundControl.settingsManager.flyViewSettings
    property var    _videoSettings:             QGroundControl.settingsManager.videoSettings
    property string _videoSource:               _videoSettings.videoSource.rawValue
    property bool   _isGst:                     QGroundControl.videoManager.isGStreamer
    property bool   _isUDP264:                  _isGst && _videoSource === _videoSettings.udp264VideoSource
    property bool   _isUDP265:                  _isGst && _videoSource === _videoSettings.udp265VideoSource
    property bool   _isRTSP:                    _isGst && _videoSource === _videoSettings.rtspVideoSource
    property bool   _isTCP:                     _isGst && _videoSource === _videoSettings.tcpVideoSource
    property bool   _isMPEGTS:                  _isGst && _videoSource === _videoSettings.mpegtsVideoSource
    property bool   _videoAutoStreamConfig:     QGroundControl.videoManager.autoStreamConfigured
    property bool   _showSaveVideoSettings:     _isGst || _videoAutoStreamConfig
    property bool   _disableAllDataPersistence: QGroundControl.settingsManager.appSettings.disableAllPersistence.rawValue

    property string gpsDisabled: "Disabled"
    property string gpsUdpPort:  "UDP Port"

    readonly property real _internalWidthRatio: 0.8

        QGCFlickable {
            clip:               true
            anchors.fill:       parent
            contentHeight:      outerItem.height
            contentWidth:       outerItem.width

            Item {
                id:     outerItem
                width:  Math.max(_root.width, settingsColumn.width)
                height: settingsColumn.height

                ColumnLayout {
                    id:                         settingsColumn
                    anchors.horizontalCenter:   parent.horizontalCenter


                    Item { width: 1; height: _margins; visible: ntripSectionLabel.visible }
                    QGCLabel {
                        id:         ntripSectionLabel
                        text:       qsTr("NTRIP / RTCM")
                        visible:    QGroundControl.settingsManager.ntripSettings.visible
                    }
                    Rectangle {
                        Layout.preferredHeight: ntripGrid.y + ntripGrid.height + _margins
                        Layout.preferredWidth:  ntripGrid.width*2 + (_margins * 2)
                        color:                  qgcPal.windowShade
                        visible:                ntripSectionLabel.visible
                        Layout.fillWidth:       true

                        GridLayout {
                            id:                         ntripGrid
                            anchors.topMargin:          _margins
                            anchors.top:                parent.top
                            Layout.fillWidth:           true
                            anchors.horizontalCenter:   parent.horizontalCenter
                            columns:                    2

                            property var  ntripSettings:    QGroundControl.settingsManager.ntripSettings

                            FactCheckBox {
                                text:                   ntripGrid.ntripSettings.ntripServerConnectEnabled.shortDescription
                                fact:                   ntripGrid.ntripSettings.ntripServerConnectEnabled
                                visible:                ntripGrid.ntripSettings.ntripServerConnectEnabled.visible
                                Layout.columnSpan:      2
                            }

                            FactCheckBox {
                                text:                   ntripGrid.ntripSettings.ntripEnableVRS.shortDescription
                                fact:                   ntripGrid.ntripSettings.ntripEnableVRS
                                visible:                ntripGrid.ntripSettings.ntripEnableVRS.visible
                                Layout.columnSpan:      2
                            }

                            QGCLabel {
                                text:               ntripGrid.ntripSettings.ntripServerHostAddress.shortDescription
                                visible:            ntripGrid.ntripSettings.ntripServerHostAddress.visible
                            }
                            FactTextField {
                                fact:                   ntripGrid.ntripSettings.ntripServerHostAddress
                                visible:                ntripGrid.ntripSettings.ntripServerHostAddress.visible
                                Layout.preferredWidth:  _valueFieldWidth
                            }

                            QGCLabel {
                                text:               ntripGrid.ntripSettings.ntripServerPort.shortDescription
                                visible:            ntripGrid.ntripSettings.ntripServerPort.visible
                            }
                            FactTextField {
                                fact:                   ntripGrid.ntripSettings.ntripServerPort
                                visible:                ntripGrid.ntripSettings.ntripServerPort.visible
                                Layout.preferredWidth:  _valueFieldWidth
                            }

                            QGCLabel {
                                text:               ntripGrid.ntripSettings.ntripUsername.shortDescription
                                visible:            ntripGrid.ntripSettings.ntripUsername.visible
                            }
                            FactTextField {
                                fact:                   ntripGrid.ntripSettings.ntripUsername
                                visible:                ntripGrid.ntripSettings.ntripUsername.visible
                                Layout.preferredWidth:  _valueFieldWidth
                            }

                            QGCLabel {
                                text:               ntripGrid.ntripSettings.ntripPassword.shortDescription
                                visible:            ntripGrid.ntripSettings.ntripPassword.visible
                            }
                            FactTextField {
                                fact:                   ntripGrid.ntripSettings.ntripPassword
                                visible:                ntripGrid.ntripSettings.ntripPassword.visible
                                Layout.preferredWidth:  _valueFieldWidth
                            }

                            QGCLabel {
                                text:               ntripGrid.ntripSettings.ntripMountpoint.shortDescription
                                visible:            ntripGrid.ntripSettings.ntripMountpoint.visible
                            }
                            FactTextField {
                                fact:                   ntripGrid.ntripSettings.ntripMountpoint
                                visible:                ntripGrid.ntripSettings.ntripMountpoint.visible
                                Layout.preferredWidth:  _valueFieldWidth
                            }

                            QGCLabel {
                                text:               ntripGrid.ntripSettings.ntripWhitelist.shortDescription
                                visible:            ntripGrid.ntripSettings.ntripWhitelist.visible
                            }
                            FactTextField {
                                fact:                   ntripGrid.ntripSettings.ntripWhitelist
                                visible:                ntripGrid.ntripSettings.ntripWhitelist.visible
                                Layout.preferredWidth:  _valueFieldWidth
                            }
                        }
                    }

                } // settingsColumn
            }
    }
}
