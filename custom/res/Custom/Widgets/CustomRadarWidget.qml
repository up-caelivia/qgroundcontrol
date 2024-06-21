import QtQuick                  2.4
import QtPositioning            5.2
import QtQuick.Layouts          1.2
import QtQuick.Controls         1.4
import QtQuick.Dialogs          1.2
import QtGraphicalEffects       1.0

import QGroundControl                   1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Controls          1.0
import QGroundControl.Palette           1.0
import QGroundControl.Vehicle           1.0
import QGroundControl.Controllers       1.0
import QGroundControl.FactSystem        1.0
import QGroundControl.FactControls      1.0
import QGroundControl.FlightDisplay     1.0

import Constants  1.0

Item {
    id: root
    height: size * 1.2
    width: size

    property real   _margins:           ScreenTools.defaultFontPixelHeight / 2
    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property bool   _isHighSpeed:       false
    property bool   _isLowSpeed:        false
    property var    modality:           _activeVehicle ? _activeVehicle.flightMode : ""

    property real size:                 ScreenTools.isAndroid ? 300 : 100
    property real _reticleHeight:       1
    property real _reticleSpacing:      size * 0.15
    property real _reticleSlot:         _reticleSpacing + _reticleHeight
    property real _longDash:            size * 0.35
    property real _shortDash:           size * 0.25
    property real _fontSize:            ScreenTools.defaultFontPointSize * 0.75

    property real distance:             proximityValues.rotationNoneValue

    ProximityRadarValues {
        id:                     proximityValues
        vehicle:                root.vehicle
    }

    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.5)
        radius: _margins
        visible: proximityValues.telemetryAvailable

        Rectangle {
            id: scale
            color: "transparent"
            width: size
            height: size
            anchors.centerIn: parent

            Item {
                anchors.fill: parent

                Column {
                    anchors.centerIn: parent
                    spacing: _reticleSpacing

                    Repeater {
                        model: proximityValues.maxDistance / 5 + 1
                        Rectangle {
                            property int _pitch: proximityValues.maxDistance - modelData * 5
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.horizontalCenterOffset: +(_longDash / 2 * 1.2)

                            width: (_pitch % 10) === 0 ? _longDash : _shortDash
                            height: _reticleHeight
                            color: "white"
                            antialiasing: true
                            smooth: true

                            QGCLabel {
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.horizontalCenterOffset: -(_longDash * 1.2)
                                anchors.verticalCenter: parent.verticalCenter
                                smooth: true
                                font.family: ScreenTools.demiboldFontFamily
                                font.pointSize: _fontSize
                                text: _pitch + " m"
                                color: "white"
                            }
                        }
                    }
                }

                Rectangle {
                    id: distanceBar
                    width: _longDash * 1.2
                    height: 3
                    color: get_color()
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.horizontalCenterOffset: +(_longDash / 2 * 1.2)

                    y: parent.height * (1 - distance / proximityValues.maxDistance) - height / 2
                    visible: distance > 0 && distance < proximityValues.maxDistance

                    Text {
                        id: distanceText
                        text: distance.toFixed(1) + " m"
                        color: distanceBar.get_color()
                        font.bold: true
                        font.pointSize: _fontSize * 1.2
                        style: Text.Outline
                        styleColor: "black"
                        anchors.bottom: distanceBar.top
                        anchors.bottomMargin: 5
                        anchors.horizontalCenter: distanceBar.horizontalCenter
                    }

                    function get_color() {
                        if (distance < 10)
                            return "red"
                        if (distance < 20)
                            return "orange"
                        return "green"
                    }
                }
            }
        }
    }
}
