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
    height: size * 1.15
    width: size
    visible: proximityValues.telemetryAvailable && !isNaN(proximityValues.rotationPitch270Value)

    property real   _margins:           ScreenTools.defaultFontPixelHeight / 6
    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property bool   _isHighSpeed:       false
    property bool   _isLowSpeed:        false
    property var    modality:           _activeVehicle ? _activeVehicle.flightMode : ""

    property real size:                 ScreenTools.isAndroid ? 300 : 100
    property real _reticleHeight:       1
    property real _reticleSpacing:      size * 0.08
    property real _reticleSlot:         _reticleSpacing + _reticleHeight
    property real _longDash:            size * 0.35
    property real _shortDash:           size * 0.25
    property real _fontSize:            ScreenTools.defaultFontPointSize * 0.75

    property real distance:             proximityValues.rotationPitch270Value //proximityValues.rotationNoneValue
    property real maxDistance:          50  //proximityValues.maxDistance
    //property real labelHeight:          50  // Height for the "Front radar" label


    ProximityRadarValues {
        id:                     proximityValues
        vehicle:                root.vehicle
    }

    Rectangle {
        id: radarLabelBackground
        color: "black"
        opacity: 0.6
        radius: 5
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        width: radarLabel.width + 10
        height: radarLabel.height + 6

        Text {
            id: radarLabel
            text: "RADAR ALT"
            anchors.centerIn: parent
            font.pointSize: ScreenTools.defaultFontPointSize
            font.bold: true
            color: "white"
            opacity: 100
            style: Text.Outline
            styleColor: "black"
        }
    }

    Rectangle {
        anchors.top: radarLabelBackground.bottom
        anchors.topMargin: _margins
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        color: Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.5)
        radius: ScreenTools.defaultFontPixelWidth * 2

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
                        model: maxDistance / 5 + 1
                        Rectangle {
                            property int _pitch: maxDistance - modelData * 5
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
                    y: parent.height * (1 - distance / maxDistance) - height / 2
                    visible: distance > 0 && distance < maxDistance

                    Rectangle {
                        id: textBackground
                        color: "black"
                        opacity: 0.6
                        radius: 4
                        anchors.bottom: distanceBar.top
                        anchors.bottomMargin: 2
                        anchors.horizontalCenter: distanceBar.horizontalCenter
                        width: distanceText.width + 10
                        height: distanceText.height + 6

                        Text {
                            id: distanceText
                            text: distance.toFixed(1) + " m"
                            color: distanceBar.get_color()
                            font.bold: true
                            font.pointSize: _fontSize * 1.2
                            anchors.centerIn: parent
                            opacity: 100
                            style: Text.Outline
                            styleColor: "black"
                        }
                    }

                    function get_color() {
                        if (distance < 10)
                            return "red"
                        if (distance < 20)
                            return "orange"
                        return Qt.rgba(0.224, 1.0, 0.078, 1.0) // Verde molto acceso
                    }

                }
            }
        }
    }
}
