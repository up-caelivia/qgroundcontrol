/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick 2.12

import QGroundControl.Controls    1.0
import QGroundControl.Palette     1.0
import QGroundControl.ScreenTools 1.0

import Custom.Controllers 1.0

Rectangle {
    id: root

    property var controller: null

    readonly property var vehicle: controller ? controller.vehicle : null
    readonly property string modality: vehicle ? vehicle.flightMode : ""

    property int _selectedMode: SpeedSelectorController.NormalSpeed

    readonly property real _padding: ScreenTools.defaultFontPixelHeight * 0.15
    readonly property int _selectedIndex:
        _selectedMode === SpeedSelectorController.HighSpeed ? 0
        : _selectedMode === SpeedSelectorController.NormalSpeed ? 1 : 2
    readonly property real buttonSize: (height - _padding * 2) / 3
    readonly property int _previewIndex: dragArea.dragging ?
        Math.round((dragArea.thumbY - dragArea.minimumY) / buttonSize) : _selectedIndex

    readonly property var _modes: [
        { label: qsTr("H"), mode: SpeedSelectorController.HighSpeed },
        { label: qsTr("N"), mode: SpeedSelectorController.NormalSpeed },
        { label: qsTr("L"), mode: SpeedSelectorController.LowSpeed },
    ]

    readonly property bool _lowUnavailable: _previewIndex === 2 && modality !== "Loiter"


    implicitWidth: buttonSize
    width: implicitWidth

    enabled: controller ? controller.ready : false
    radius: width / 2
    antialiasing: true
    color: Qt.rgba(
        qgcPal.window.r,
        qgcPal.window.g,
        qgcPal.window.b,
        0.30)
    border.width: 1
    border.color: Qt.rgba(
        qgcPal.text.r,
        qgcPal.text.g,
        qgcPal.text.b,
        0.18)

    QGCPalette {
        id: qgcPal
        colorGroupEnabled: root.enabled
    }

    Timer {
        id: messageTimer
        interval: 2000
        onTriggered: CustomPlugin.speedMessage = ""
    }

    function showMessage(message, warning, persistent) {
        CustomPlugin.sendLogMessage(
            message, "", warning ? "Warning" : "Info")
        CustomPlugin.speedMessage = message
        messageTimer.stop()
        if (!persistent) {
            messageTimer.start()
        }
    }

    function resetSelection() {
        _selectedMode = SpeedSelectorController.NormalSpeed
        messageTimer.stop()
        CustomPlugin.speedMessage = ""
    }

    function selectMode(mode, label) {
        if (!controller || !controller.ready) {
            return
        }

        // Keep Low pending outside Loiter, as in the previous selector.
        if (mode === SpeedSelectorController.LowSpeed && modality !== "Loiter") {
            _selectedMode = mode
            showMessage(qsTr("Low speed mode not available"), true, true)
            return
        }

        if (!controller.requestSpeedMode(mode)) {
            showMessage(
                qsTr("Could not apply %1 speed settings").arg(label),
                true, false)
            return
        }

        _selectedMode = mode
        showMessage(
            qsTr("%1 speed mode enabled").arg(label),
            false, mode === SpeedSelectorController.LowSpeed)
    }

    onVehicleChanged: resetSelection()

    onVisibleChanged: {
        if (!visible) {
            dragArea.dragging = false
            dragArea.pressedVehicle = null
            messageTimer.stop()
            CustomPlugin.speedMessage=""
        }
    }

    onModalityChanged: {
        if (_selectedMode === SpeedSelectorController.LowSpeed) {
            if (modality === "Loiter") {
                selectMode(SpeedSelectorController.LowSpeed, qsTr("Low"))
            } else {
                showMessage(qsTr("Low speed mode not available"), true, true)
            }
        }
    }

    Rectangle {
        id: thumb

        x: root._padding
        y: dragArea.dragging ?
            dragArea.thumbY : dragArea.minimumY + root._selectedIndex * root.buttonSize

        width: root.width - root._padding * 2
        height: width
        radius: width / 2
        antialiasing: true

        color: Qt.rgba(
            qgcPal.window.r,
            qgcPal.window.g,
            qgcPal.window.b,
            0.85
        )

        border.width: 1
        border.color: root._lowUnavailable ? qgcPal.colorOrange : qgcPal.colorGreen

        Behavior on y {
            enabled: !dragArea.dragging

            NumberAnimation {
                duration: 150
                easing.type: Easing.InOutQuad
            }
        }
    }

    Column {
        y: root._padding
        width: root.width

        Repeater {
            model: root._modes

            Item {
                width: root.width
                height: root.buttonSize

                QGCLabel {
                    anchors.fill: parent
                    anchors.margins: root._padding
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text: modelData.label

                    fontSizeMode: Text.Fit
                    minimumPointSize: Math.min(6, ScreenTools.smallFontPointSize)
                    font.pointSize: ScreenTools.defaultFontPointSize
                    font.weight: index === root._previewIndex ? Font.DemiBold : Font.Normal

                    color: index === root._previewIndex ?
                        (root._lowUnavailable ? qgcPal.colorOrange : qgcPal.colorGreen)
                        : Qt.rgba(
                            qgcPal.text.r,
                            qgcPal.text.g,
                            qgcPal.text.b,
                            0.65
                        )
                }
            }
        }
    }

    MouseArea {
        id: dragArea

        anchors.fill: parent
        preventStealing: true

        property bool dragging: false
        property real thumbY: 0
        property real grabOffset: 0
        property var pressedVehicle: null

        readonly property real minimumY: root._padding + (root.buttonSize - thumb.height) / 2

        function boundedY(position) {
            return Math.max(
                minimumY,
                Math.min(minimumY + root.buttonSize * 2, position)
            )
        }

        onPressed: {
            pressedVehicle = root.vehicle
            var onThumb = mouse.y >= thumb.y && mouse.y <= thumb.y + thumb.height
            grabOffset = onThumb ? mouse.y - thumb.y : thumb.height / 2
            thumbY = boundedY(mouse.y - grabOffset)
            dragging = true
        }

        onPositionChanged: {
            if (dragging) {
                thumbY = boundedY(mouse.y - grabOffset)
            }
        }

        onReleased: {
            if (dragging && root.buttonSize > 0 && root.vehicle === pressedVehicle) {
                var index = Math.round(
                    (boundedY(thumbY) - minimumY) / root.buttonSize
                )
                var selection = root._modes[index]

                if (selection.mode !== root._selectedMode) {
                    root.selectMode(selection.mode, selection.label)
                }
                dragging = false
                pressedVehicle = null
            }
        }

        onCanceled: {
            dragging = false
            pressedVehicle = null
        }
    }
}
