import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQml 2.15
import QGroundControl 1.0
import QGroundControl.Controls 1.0
import QGroundControl.ScreenTools 1.0
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import QtLocation 5.15
import QtPositioning 5.15
import Constants 1.0

ToolStrip {
    id: abluoToolStrip
    width: Screen.width * 0.6
    height: Screen.height * 0.5
    color: Qt.rgba(0, 0, 0, 0.7)

    anchors.centerIn: parent
    anchors.verticalCenterOffset: -Screen.height * 0.10

    // UI / telemetry state
    property bool missionInProgress: false

    // Public API
    property alias serpentine_path: serpentine.path
    property alias total_length_m:  serpentine.total_len_m
    property real  pitchValue: 0

    function fmtCoord(c) {
        if (!c || !c.isValid) return "--"
        const lat = Number(c.latitude).toFixed(7)
        const lon = Number(c.longitude).toFixed(7)
        const agl = Number(c.altitude || 0).toFixed(2)
        return `${lat}, ${lon}  AGL=${agl} m`
    }

    Component.onCompleted: {
        if (Constants.savedPitch !== undefined) {
            abluoToolStrip.pitchValue = Number(Constants.savedPitch) || 0
            serpentine.pitch_m = abluoToolStrip.pitchValue
            if (pitchField) pitchField.text = abluoToolStrip.pitchValue > 0 ? String(abluoToolStrip.pitchValue) : ""
        }
        if (Constants.savedStart && Constants.savedStart.isValid) serpentine.s_coord = Constants.savedStart
        if (Constants.savedStop  && Constants.savedStop.isValid)  serpentine.t_coord = Constants.savedStop

        leftPanel.sideLeft = (Constants.savedSideLeft !== undefined) ? !!Constants.savedSideLeft : true
        serpentine.build()
    }

    // Mission state watcher – when in Auto, reduce drawing load
    Connections {
        target: QGroundControl.multiVehicleManager ? QGroundControl.multiVehicleManager.activeVehicle : null
        onFlightModeChanged: {
            const v = QGroundControl.multiVehicleManager.activeVehicle
            const autoMode = (v && v.flightMode === "Auto")
            abluoToolStrip.missionInProgress = !!autoMode
            serpCanvas.drawingEnabled = !autoMode
            serpCanvas.schedulePaint()
        }
    }

    function get_current_coord_agl() {
        const vm = QGroundControl.multiVehicleManager
        if (!vm || !vm.activeVehicle) return null
        const v = vm.activeVehicle
        if (!v.coordinate || !v.coordinate.isValid) return null

        let altAgl = 0
        try {
            if (v.altitudeRelative && v.altitudeRelative.rawValue !== undefined) {
                altAgl = Number(v.altitudeRelative.rawValue)
            } else if (v.altitudeAMSL && v.altitudeAMSL.rawValue !== undefined) {
                altAgl = Number(v.altitudeAMSL.rawValue)
            } else if (v.coordinate.altitude !== undefined) {
                altAgl = Number(v.coordinate.altitude)
            }
        } catch (e) { altAgl = 0 }

        return QtPositioning.coordinate(
            Number(v.coordinate.latitude),
            Number(v.coordinate.longitude),
            altAgl
        )
    }

    RowLayout {
        id: mainRow
        anchors.fill: parent
        anchors.margins: ScreenTools.defaultFontPixelWidth
        spacing: ScreenTools.defaultFontPixelHeight

        // ------------ LEFT: image + markers + dimensions + serpentine ------------
        Rectangle {
            id: leftPanel
            Layout.preferredWidth: parent.width * 0.35
            Layout.fillHeight: true
            color: "transparent"

            // inner padding
            property real pad: ScreenTools.defaultFontPixelHeight * 0.5

            // dimension labels
            property real  fontPx: Math.round(ScreenTools.defaultFontPixelHeight * 1.0)
            property real  width_m:  serpentine.width_m
            property real  height_m: serpentine.height_m
            property string dimValueWidthMeters:  width_m  > 0 ? width_m.toFixed(2)  + " m" : "--"
            property string dimValueHeightMeters: height_m > 0 ? height_m.toFixed(2) + " m" : "--"

            // S/T placement: S on left/right and above/below
            property bool sideLeft: true
            property bool sAbove: {
                if (!serpentine.s_coord || !serpentine.t_coord ||
                    !serpentine.s_coord.isValid || !serpentine.t_coord.isValid) return false
                const zS = serpentine.s_coord.altitude || 0
                const zT = serpentine.t_coord.altitude || 0
                return zS > zT
            }

            // image fills available area with padding
            Image {
                id: bigImage
                anchors.fill: parent
                anchors.margins: leftPanel.pad
                fillMode: Image.PreserveAspectFit
                source: "/res/facade"
                mipmap: true
            }

            // real painted area
            readonly property real imgLeft:   bigImage.x + (bigImage.width  - bigImage.paintedWidth)  / 2
            readonly property real imgTop:    bigImage.y + (bigImage.height - bigImage.paintedHeight) / 2
            readonly property real imgRight:  imgLeft + bigImage.paintedWidth
            readonly property real imgBottom: imgTop + bigImage.paintedHeight
            readonly property real imgW:      bigImage.paintedWidth
            readonly property real imgH:      bigImage.paintedHeight

            // S/T markers
            property real markerSize:  Math.round(ScreenTools.defaultFontPixelHeight * 1.2)
            property real markerInset: Math.round(ScreenTools.defaultFontPixelHeight * 0.4)
            property color markerFill: "white"
            property color markerText: "black"
            property color markerBorder: "black"

            // S
            Item {
                width: leftPanel.markerSize; height: leftPanel.markerSize
                x: leftPanel.sideLeft
                   ? leftPanel.imgLeft + leftPanel.markerInset
                   : leftPanel.imgRight - width - leftPanel.markerInset
                y: leftPanel.sAbove
                   ? leftPanel.imgTop + leftPanel.markerInset
                   : leftPanel.imgBottom - height - leftPanel.markerInset
                z: 20; visible: bigImage.status === Image.Ready
                Rectangle { anchors.fill: parent; radius: width/2; color: leftPanel.markerFill; border.color: leftPanel.markerBorder; border.width: 1 }
                QGCLabel { anchors.centerIn: parent; text: "S"; color: leftPanel.markerText; font.bold: true; font.pointSize: ScreenTools.defaultFontPointSize * 1.1 }
            }

            // T
            Item {
                width: leftPanel.markerSize; height: leftPanel.markerSize
                x: leftPanel.sideLeft
                   ? leftPanel.imgRight - width - leftPanel.markerInset
                   : leftPanel.imgLeft  + leftPanel.markerInset
                y: leftPanel.sAbove
                   ? leftPanel.imgBottom - height - leftPanel.markerInset
                   : leftPanel.imgTop    + leftPanel.markerInset
                z: 20; visible: bigImage.status === Image.Ready
                Rectangle { anchors.fill: parent; radius: width/2; color: leftPanel.markerFill; border.color: leftPanel.markerBorder; border.width: 1 }
                QGCLabel { anchors.centerIn: parent; text: "T"; color: leftPanel.markerText; font.bold: true; font.pointSize: ScreenTools.defaultFontPointSize * 1.1 }
            }

            // WIDTH (text only)
            QGCLabel {
                id: widthText
                text: leftPanel.dimValueWidthMeters
                color: "white"
                font.pixelSize: leftPanel.fontPx
                x: leftPanel.imgLeft + leftPanel.imgW/2 - width/2
                y: leftPanel.imgTop - 8 - height
                z: 10
                visible: bigImage.status === Image.Ready && leftPanel.width_m > 0
            }

            // HEIGHT (text only, rotated)
            QGCLabel {
                id: heightText
                text: leftPanel.dimValueHeightMeters
                color: "white"
                font.pixelSize: leftPanel.fontPx
                x: leftPanel.imgLeft - 10 - height
                y: leftPanel.imgTop + leftPanel.imgH/2 - width/2
                rotation: -90
                transformOrigin: Item.Center
                z: 10
                visible: bigImage.status === Image.Ready && leftPanel.height_m > 0
            }

            // SERPENTINE
            Canvas {
                id: serpCanvas
                x: leftPanel.imgLeft
                y: leftPanel.imgTop
                width:  leftPanel.imgW
                height: leftPanel.imgH
                z: 15
                visible: bigImage.status === Image.Ready

                // performance knobs
                property int  maxStripes: 160
                property bool drawingEnabled: true
                property bool _paintScheduled: false
                function schedulePaint() {
                    if (_paintScheduled) return
                    _paintScheduled = true
                    Qt.callLater(function() {
                        _paintScheduled = false
                        serpCanvas.requestPaint()
                    })
                }

                // repaint on value changes only
                Connections {
                    target: serpentine
                    onWidth_mChanged:  serpCanvas.schedulePaint()
                    onHeight_mChanged: serpCanvas.schedulePaint()
                    onPathChanged:     serpCanvas.schedulePaint()
                }
                Connections {
                    target: leftPanel
                    onSideLeftChanged: serpCanvas.schedulePaint()
                    onSAboveChanged:   serpCanvas.schedulePaint()
                }
                Connections {
                    target: abluoToolStrip
                    onPitchValueChanged: serpCanvas.schedulePaint()
                }
                onWidthChanged:  schedulePaint()
                onHeightChanged: schedulePaint()

                onPaint: {
                    if (!drawingEnabled) return
                    const ctx = getContext("2d")
                    ctx.clearRect(0,0,width,height)

                    if (leftPanel.height_m <= 0 || width <= 0 || height <= 0) return

                    // usable rectangle (between image edges and marker inset)
                    const inset = leftPanel.markerInset + leftPanel.markerSize/2
                    const leftX   = inset
                    const rightX  = width  - inset
                    const topY    = inset
                    const bottomY = height - inset

                    // meters->pixels (vertical)
                    const rectHpx = Math.max(0, bottomY - topY)
                    if (rectHpx <= 0) return
                    const pxPerMeterV = rectHpx / Math.max(0.001, leftPanel.height_m)
                    const stepPxNom   = Math.max(1, abluoToolStrip.pitchValue * pxPerMeterV)

                    // S/T in pixels
                    const sPixX = leftPanel.sideLeft ? leftX : rightX
                    const tPixX = leftPanel.sideLeft ? rightX : leftX
                    const sPixY = leftPanel.sAbove ? topY : bottomY
                    const tPixY = leftPanel.sAbove ? bottomY : topY

                    // vertical direction
                    const dir = (tPixY > sPixY) ? +1 : -1

                    // style
                    ctx.strokeStyle = "#FFD600"
                    ctx.lineWidth   = 2

                    // draw
                    let y = sPixY
                    let goRight = leftPanel.sideLeft
                    let stripes = 0
                    ctx.beginPath()
                    ctx.moveTo(sPixX, y)

                    while (((dir > 0 && y < tPixY) || (dir < 0 && y > tPixY)) && stripes < maxStripes) {
                        ctx.lineTo(goRight ? rightX : leftX, y)      // horizontal
                        const remaining = Math.abs(tPixY - y)
                        const step = Math.min(stepPxNom, remaining)  // shorten last step
                        y += dir * step
                        ctx.lineTo(goRight ? rightX : leftX, y)      // vertical
                        goRight = !goRight
                        stripes++
                    }

                    // close exactly on T
                    const curX = goRight ? leftX : rightX
                    if (curX !== tPixX || y !== tPixY) ctx.lineTo(tPixX, tPixY)

                    ctx.stroke()
                }
            }
        }

        // ----------------- RIGHT: controls -----------------
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Qt.rgba(1,1,1,0.06)
            radius: ScreenTools.defaultFontPixelHeight
            border.color: Qt.rgba(1,1,1,0.15)
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: ScreenTools.defaultFontPixelWidth
                spacing: ScreenTools.defaultFontPixelHeight * 0.1

                // row: Set Start | Set Stop
                RowLayout {
                    Layout.fillWidth: true
                    spacing: ScreenTools.defaultFontPixelWidth

                    QGCButton {
                        text: "Set Start"
                        enabled: !abluoToolStrip.missionInProgress
                        Layout.fillWidth: true
                        onClicked: {
                            if (CustomPlugin && CustomPlugin.setStart) CustomPlugin.setStart()
                            const cur = get_current_coord_agl()
                            if (cur) {
                                serpentine.s_coord = cur
                                Constants.savedStart = serpentine.s_coord
                                serpentine.build()
                            }
                        }
                    }
                    QGCButton {
                        text: "Set Stop"
                        Layout.fillWidth: true
                        onClicked: {
                            if (CustomPlugin && CustomPlugin.setStop) CustomPlugin.setStop()
                            const cur = get_current_coord_agl()
                            if (cur) {
                                serpentine.t_coord = cur
                                Constants.savedStop = serpentine.t_coord
                                serpentine.build()
                            }
                        }
                    }
                }

                // optional: flip (hidden)
                QGCButton {
                    text: "Flip side"
                    Layout.fillWidth: true
                    visible: false
                    onClicked: {
                        leftPanel.sideLeft = !leftPanel.sideLeft
                        Constants.savedSideLeft = leftPanel.sideLeft
                    }
                }

                // row: Pitch (m) | [text field]
                RowLayout {
                    Layout.fillWidth: true
                    spacing: ScreenTools.defaultFontPixelWidth

                    QGCLabel {
                        text: "Pitch (m)"
                        color: "white"
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 10
                    }
                    QGCTextField {
                        id: pitchField
                        Layout.fillWidth: true
                        placeholderText: "Insert pitch"
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        validator: DoubleValidator { bottom: 0; top: 1e6; decimals: 3 }
                        onEditingFinished: {
                            abluoToolStrip.pitchValue = Number(text)
                            serpentine.pitch_m = abluoToolStrip.pitchValue
                            if (CustomPlugin && !isNaN(abluoToolStrip.pitchValue) && CustomPlugin.setPitch)
                                CustomPlugin.setPitch(abluoToolStrip.pitchValue)
                            Constants.savedPitch = abluoToolStrip.pitchValue
                            serpentine.build()
                        }
                    }
                }

                Rectangle { height: 1; Layout.fillWidth: true; color: Qt.rgba(255,255,255,0.15) }

                QGCLabel {
                    text: "Total path length: " + (
                           total_length_m >= 1000
                           ? (total_length_m/1000).toFixed(2) + " km"
                           : total_length_m.toFixed(2) + " m"
                    )
                    color: "white"
                    Layout.fillWidth: true
                }

                QGCLabel {
                    text: "Start (S): " + fmtCoord(serpentine.s_coord) + "\nStop (T): " + fmtCoord(serpentine.t_coord)
                    color: "white"
                    font.family: "monospace"
                    Layout.fillWidth: true
                }

                Item { Layout.fillHeight: true }

                QGCButton {
                    id: uploadBtn
                    text: "upload and start cleaning"
                    Layout.fillWidth: true
                    Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 3

                    // Disabilita quando la missione è in corso, oppure se mancano dati minimi
                    enabled: !abluoToolStrip.missionInProgress
                            && serpentine.s_coord && serpentine.s_coord.isValid
                            && serpentine.t_coord && serpentine.t_coord.isValid
                            && abluoToolStrip.pitchValue > 0

                    // (opzionale) tooltip per feedback all’utente
                    ToolTip.visible: hovered && !enabled
                    ToolTip.text: abluoToolStrip.missionInProgress
                                ? "Mission is running: upload disabled"
                                : "Set Start/Stop and a positive Pitch to enable"

                    onClicked: {
                        if (!serpentine.s_coord || !serpentine.t_coord ||
                            !serpentine.s_coord.isValid || !serpentine.t_coord.isValid) {
                            if (CustomPlugin && CustomPlugin.sendLogMessage)
                                CustomPlugin.sendLogMessage("Set Start and Stop before uploading the mission", "", "Warning")
                            return
                        }

                        const pts = CustomPlugin.buildAbluoPath(
                            serpentine.s_coord,
                            serpentine.t_coord,
                            abluoToolStrip.pitchValue,
                            leftPanel.sideLeft
                        )
                        if (!pts || pts.length < 2) {
                            if (CustomPlugin && CustomPlugin.sendLogMessage)
                                CustomPlugin.sendLogMessage("Empty path: check pitch and altitudes", "", "Warning")
                            return
                        }

                        serpentine.path = pts

                        if (CustomPlugin && CustomPlugin.uploadAbluoMission) {
                            CustomPlugin.uploadAbluoMission(pts)
                            if (CustomPlugin.sendLogMessage)
                                CustomPlugin.sendLogMessage("Mission uploaded: " + pts.length + " waypoints")
                        } else if (CustomPlugin && CustomPlugin.sendLogMessage) {
                            CustomPlugin.sendLogMessage("uploadAbluoMission not implemented in plugin", "", "Warning")
                        }
                    }
                }

            }
        }
    }

    // ---------------- SERPENTINE ENGINE (GPS) ----------------
    Item {
        id: serpentine
        property var  s_coord: QtPositioning.coordinate()
        property var  t_coord: QtPositioning.coordinate()
        property real pitch_m: 10
        property real width_m:  0
        property real height_m: 0
        property var  path: []
        property real total_len_m: 0

        function _interp(c1, c2, tt) {
            const az = c1.azimuthTo(c2)
            const d  = c1.distanceTo(c2)
            return c1.atDistanceAndAzimuth(d * Math.max(0, Math.min(1, tt)), az)
        }

        function _recalc_rect_metrics() {
            if (!s_coord || !t_coord || !s_coord.isValid || !t_coord.isValid) { width_m = 0; height_m = 0; return }
            const z0 = s_coord.altitude || 0
            const z1 = t_coord.altitude || 0
            height_m = Math.abs(z1 - z0)
            const west  = Math.min(s_coord.longitude, t_coord.longitude)
            const east  = Math.max(s_coord.longitude, t_coord.longitude)
            const midLa = 0.5 * (Number(s_coord.latitude) + Number(t_coord.latitude))
            const pW = QtPositioning.coordinate(midLa, west)
            const pE = QtPositioning.coordinate(midLa, east)
            width_m  = pW.distanceTo(pE)
            console.log(`AbluoPathWidgetUP: rectangle size: width=${width_m.toFixed(2)} m, height=${height_m.toFixed(2)} m`)
        }

        function build() {
            if (!s_coord || !t_coord || !s_coord.isValid || !t_coord.isValid) { path = []; total_len_m = 0; return }
            _recalc_rect_metrics()

            const z0 = s_coord.altitude || 0
            const z1 = t_coord.altitude || z0
            const dz = Math.max(0.001, pitch_m)
            const totalH = Math.max(0.001, z1 - z0)

            const south = Math.min(s_coord.latitude,  t_coord.latitude)
            const north = Math.max(s_coord.latitude,  t_coord.latitude)
            const west  = Math.min(s_coord.longitude, t_coord.longitude)
            const east  = Math.max(s_coord.longitude, t_coord.longitude)
            const bl = QtPositioning.coordinate(south, west)
            const br = QtPositioning.coordinate(south, east)
            const tl = QtPositioning.coordinate(north, west)
            const tr = QtPositioning.coordinate(north, east)

            let pts = []
            let y = z0
            let leftToRight = true

            while (y < z1) {
                const next = Math.min(z1, y + dz)
                const t1 = (y    - z0) / totalH
                const t2 = (next - z0) / totalH

                const pL1 = _interp(bl, tl, t1); pL1.altitude = y
                const pR1 = _interp(br, tr, t1); pR1.altitude = y
                const pL2 = _interp(bl, tl, t2); pL2.altitude = next
                const pR2 = _interp(br, tr, t2); pR2.altitude = next

                if (leftToRight) { pts.push(pL1); pts.push(pR1) } else { pts.push(pR1); pts.push(pL1) }
                if (leftToRight) { pts.push(pR2) } else { pts.push(pL2) }

                leftToRight = !leftToRight
                y = next
            }

            const pRtop = _interp(br, tr, 1.0); pRtop.altitude = z1
            const last = pts.length ? pts[pts.length-1] : null
            if (!last || last.latitude !== pRtop.latitude || last.longitude !== pRtop.longitude || last.altitude !== pRtop.altitude) {
                pts.push(pRtop)
            }

            let sum = 0
            for (let i = 1; i < pts.length; i++) sum += pts[i-1].distanceTo(pts[i])
            total_len_m = sum
            path = pts
        }

        onS_coordChanged: build()
        onT_coordChanged: build()
        onPitch_mChanged: build()
        Component.onCompleted: build()
    }
}
