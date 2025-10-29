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
import QGroundControl.FactSystem 1.0
import QGroundControl.Controllers 1.0

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

    // 0 = horizontal (E-W stripes, vertical progression)
    // 1 = vertical   (N-S stripes, horizontal progression)
    property int orientationMode: 0

    function fmtCoord(c) {
        if (!c || !c.isValid) return "--"
        const lat = Number(c.latitude).toFixed(7)
        const lon = Number(c.longitude).toFixed(7)
        const agl = Number(c.altitude || 0).toFixed(2)
        return `${lat}, ${lon}  alt=${agl} m`
    }

    Component.onCompleted: {
        // restore saved settings/coords if available
        if (Constants.savedPitch !== undefined) {
            abluoToolStrip.pitchValue = Number(Constants.savedPitch) || 0
            serpentine.pitch_m = abluoToolStrip.pitchValue
            if (pitchField) pitchField.text = abluoToolStrip.pitchValue > 0 ? String(abluoToolStrip.pitchValue) : ""
        }
        if (Constants.savedOrientation !== undefined) {
            orientationMode = Number(Constants.savedOrientation) === 1 ? 1 : 0
            serpentine.orientation = orientationMode
        }
        if (Constants.savedStart && Constants.savedStart.isValid) serpentine.s_coord = Constants.savedStart
        if (Constants.savedStop  && Constants.savedStop.isValid)  serpentine.t_coord = Constants.savedStop

        leftPanel.sideLeft = (Constants.savedSideLeft !== undefined) ? !!Constants.savedSideLeft : true
        serpentine.build()

        // if the plugin already exposes how many waypoints the current mission on the vehicle has,
        // initialize serpentine.path accordingly so serpCanvas already knows its length
        if (CustomPlugin && CustomPlugin.abluoMissionCount !== undefined) {
            serpentine.setPathLengthFromCount(CustomPlugin.abluoMissionCount)
            serpCanvas.schedulePaint()

            // align passedWpIndex with the new length
            const len = Math.max(serpentine.path.length - 1, -1)
            if (CustomPlugin.abluoCurrentWp !== undefined) {
                serpCanvas.passedWpIndex = Math.min(Math.max(CustomPlugin.abluoCurrentWp, -1), len)
            }
        }
    }

    Connections {
        target: QGroundControl.multiVehicleManager ? QGroundControl.multiVehicleManager.activeVehicle : null
        onFlightModeChanged: {
            const v = QGroundControl.multiVehicleManager.activeVehicle
            const autoMode = (v && v.flightMode === "Auto")
            abluoToolStrip.missionInProgress = !!autoMode
            // keep canvas active even in AUTO so colors can update live
            serpCanvas.drawingEnabled = true
            serpCanvas.schedulePaint()
        }
    }

    function get_current_coord_agl() {
        const vm = QGroundControl.multiVehicleManager
        if (!vm || !vm.activeVehicle) return null
        const v = vm.activeVehicle
        if (!v.coordinate || !v.coordinate.isValid) return null

        // prefer relative altitude if available
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
            property real  fontPx: ScreenTools.defaultFontPixelHeight
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

            // area actually painted by the image
            readonly property real imgLeft:   bigImage.x + (bigImage.width  - bigImage.paintedWidth)  / 2
            readonly property real imgTop:    bigImage.y + (bigImage.height - bigImage.paintedHeight) / 2
            readonly property real imgRight:  imgLeft + bigImage.paintedWidth
            readonly property real imgBottom: imgTop + bigImage.paintedHeight
            readonly property real imgW:      bigImage.paintedWidth
            readonly property real imgH:      bigImage.paintedHeight

            // S/T markers style
            property real markerSize:  Math.round(ScreenTools.defaultFontPixelHeight * 1.2)
            property real markerInset: Math.round(ScreenTools.defaultFontPixelHeight * 0.4)
            property color markerFill: "white"
            property color markerText: "black"
            property color markerBorder: "black"

            // S marker
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

            // T marker
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

            // WIDTH label (text only)
            QGCLabel {
                id: widthText
                text: leftPanel.dimValueWidthMeters
                color: "white"
                x: leftPanel.imgLeft + leftPanel.imgW/2 - width/2
                y: leftPanel.imgTop - height * 0.8
                z: 10
                visible: bigImage.status === Image.Ready && leftPanel.width_m > 0
            }

            // HEIGHT label (text only, rotated)
            QGCLabel {
                id: heightText
                text: leftPanel.dimValueHeightMeters
                color: "white"
                x: leftPanel.imgLeft - height * 1.5
                y: leftPanel.imgTop + leftPanel.imgH/2 - width/2
                rotation: -90
                transformOrigin: Item.Center
                z: 10
                visible: bigImage.status === Image.Ready && leftPanel.height_m > 0
            }

            // SERPENTINE canvas
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

                // current mission "target" waypoint index; -1 = none yet
                property int passedWpIndex: -1

                // repaint triggers
                Connections {
                    target: serpentine
                    onWidth_mChanged:  serpCanvas.schedulePaint()
                    onHeight_mChanged: serpCanvas.schedulePaint()
                    onPathChanged:     serpCanvas.schedulePaint()
                    onOrientationChanged: serpCanvas.schedulePaint()
                }
                Connections {
                    target: leftPanel
                    onSideLeftChanged: serpCanvas.schedulePaint()
                    onSAboveChanged:   serpCanvas.schedulePaint()
                }
                Connections {
                    target: abluoToolStrip
                    onPitchValueChanged: serpCanvas.schedulePaint()
                    onOrientationModeChanged: serpCanvas.schedulePaint()
                }

                // keep passedWpIndex in sync with CustomPlugin
                Connections {
                    target: CustomPlugin
                    onAbluoCurrentWpChanged: {
                        const len = Math.max(serpentine.path.length - 1, -1)
                        serpCanvas.passedWpIndex = Math.min(Math.max(CustomPlugin.abluoCurrentWp, -1), len)
                        serpCanvas.schedulePaint()
                    }
                }

                // if the number of waypoints in the loaded mission changes on the vehicle,
                // update the local "length" accordingly and realign passedWpIndex
                Connections {
                    target: CustomPlugin
                    onAbluoMissionCountChanged: {
                        if (CustomPlugin.abluoMissionCount !== undefined) {
                            serpentine.setPathLengthFromCount(CustomPlugin.abluoMissionCount)

                            const len = Math.max(serpentine.path.length - 1, -1)
                            serpCanvas.passedWpIndex = Math.min(
                                Math.max(CustomPlugin.abluoCurrentWp, -1),
                                len
                            )
                            serpCanvas.schedulePaint()
                        }
                    }
                }

                Component.onCompleted: {
                    if (CustomPlugin && CustomPlugin.abluoCurrentWp !== undefined) {
                        const len = Math.max(serpentine.path.length - 1, -1)
                        serpCanvas.passedWpIndex = Math.min(Math.max(CustomPlugin.abluoCurrentWp, -1), len)
                    }
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

                    const rectWpx = Math.max(0, rightX - leftX)
                    const rectHpx = Math.max(0, bottomY - topY)
                    if (rectWpx <= 0 || rectHpx <= 0) return

                    // orientation: 0=horizontal (vertical progression), 1=vertical (horizontal progression)
                    const isHorizontal = (abluoToolStrip.orientationMode === 0)

                    // step unit (meters -> pixels) along the progression direction
                    const pxPerMeterAdvance = isHorizontal
                        ? (rectHpx / Math.max(0.001, leftPanel.height_m))
                        : (rectWpx / Math.max(0.001, leftPanel.width_m))
                    const stepPxNom = Math.max(1, abluoToolStrip.pitchValue * pxPerMeterAdvance)

                    // S/T points in pixels
                    const sPixX = leftPanel.sideLeft ? leftX : rightX
                    const tPixX = leftPanel.sideLeft ? rightX : leftX
                    const sPixY = leftPanel.sAbove ? topY : bottomY
                    const tPixY = leftPanel.sAbove ? bottomY : topY

                    const pts = []

                    if (isHorizontal) {
                        // move along Y; horizontal stripes going L<->R
                        const dirY = (tPixY > sPixY) ? +1 : -1
                        let y = sPixY
                        let goRight = leftPanel.sideLeft
                        pts.push({x: sPixX, y: y})

                        let stripes = 0
                        while (((dirY > 0 && y < tPixY) || (dirY < 0 && y > tPixY)) && stripes < serpCanvas.maxStripes) {
                            // horizontal segment
                            pts.push({ x: (goRight ? rightX : leftX), y: y })
                            // vertical step (shorten last step)
                            const remaining = Math.abs(tPixY - y)
                            const step = Math.min(stepPxNom, remaining)
                            y += dirY * step
                            pts.push({ x: (goRight ? rightX : leftX), y: y })

                            goRight = !goRight
                            stripes++
                        }
                        const last = pts[pts.length - 1]
                        if (last.x !== tPixX || last.y !== tPixY) pts.push({ x: tPixX, y: tPixY })
                    } else {
                        // vertical: move along X; vertical stripes going T<->B
                        const dirX = (tPixX > sPixX) ? +1 : -1
                        let x = sPixX
                        let goDown = leftPanel.sAbove  // if S is above, first stripe goes down
                        pts.push({x: x, y: sPixY})

                        let stripes = 0
                        while (((dirX > 0 && x < tPixX) || (dirX < 0 && x > tPixX)) && stripes < serpCanvas.maxStripes) {
                            // vertical segment
                            pts.push({ x: x, y: (goDown ? bottomY : topY) })
                            // horizontal step (shorten last step)
                            const remaining = Math.abs(tPixX - x)
                            const step = Math.min(stepPxNom, remaining)
                            x += dirX * step
                            pts.push({ x: x, y: (goDown ? bottomY : topY) })

                            goDown = !goDown
                            stripes++
                        }
                        const last = pts[pts.length - 1]
                        if (last.x !== tPixX || last.y !== tPixY) pts.push({ x: tPixX, y: tPixY })
                    }

                    function strokeFromTo(iStart, iEnd, color, lineWidth) {
                        if (iEnd <= iStart || iStart < 0 || iEnd >= pts.length) return
                        ctx.beginPath()
                        ctx.moveTo(pts[iStart].x, pts[iStart].y)
                        for (let i = iStart + 1; i <= iEnd; i++) {
                            ctx.lineTo(pts[i].x, pts[i].y)
                        }
                        ctx.strokeStyle = color
                        ctx.lineWidth   = lineWidth
                        ctx.stroke()
                    }

                    const cut = Math.min(
                        Math.max(serpCanvas.passedWpIndex-1, -1),
                        pts.length - 1
                    )
                    const lineW = 4
                    const COL_RED    = "#FF3B30"  // already flown
                    const COL_ORANGE = "#FFA500"  // current segment
                    const COL_YELLOW = "#FFD600"  // remaining
                    const idx = Math.min(
                        Math.max(serpCanvas.passedWpIndex-1, -1),
                        pts.length - 1
                    )

                    if (cut < 0) {
                        strokeFromTo(0, pts.length - 1, COL_YELLOW, lineW)
                    } else if (cut >= pts.length - 1) {
                        strokeFromTo(0, pts.length - 1, COL_RED, lineW)
                    } else {
                        const passedEnd = idx - 1
                        if (passedEnd >= 1) {
                            strokeFromTo(0, passedEnd, COL_RED, lineW)
                        } else if (passedEnd === 0) {
                            strokeFromTo(0, 0, COL_RED, lineW)
                        }
                        const curStart = Math.max(0, passedEnd)
                        const curEnd   = Math.max(1, idx)
                        strokeFromTo(curStart, curEnd, COL_ORANGE, lineW)
                        strokeFromTo(idx, pts.length - 1, COL_YELLOW, lineW)
                    }
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
                        enabled: !abluoToolStrip.missionInProgress
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

                // optional: flip side (hidden)
                QGCButton {
                    text: "Flip side"
                    Layout.fillWidth: true
                    visible: false
                    onClicked: {
                        leftPanel.sideLeft = !leftPanel.sideLeft
                        Constants.savedSideLeft = leftPanel.sideLeft
                    }
                }

                // row: Pitch (m) | [text field] | Orientation [combo]
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
                        placeholderText: "Enter pitch"
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
                    QGCComboBox {
                        id: orientationCombo
                        //Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 16
                        model: [qsTr("Horizontal"), qsTr("Vertical")]
                        Layout.fillWidth:   true
                        currentIndex: abluoToolStrip.orientationMode
                        onActivated: {
                            abluoToolStrip.orientationMode = currentIndex
                            serpentine.orientation = currentIndex
                            Constants.savedOrientation = currentIndex
                            serpentine.build()
                            serpCanvas.schedulePaint()
                        }
                    }
                }

                Rectangle { height: 1; Layout.fillWidth: true; color: Qt.rgba(255,255,255,0.15) }

                QGCLabel {
                    text: "Total path length: " + total_length_m.toFixed(2) + " m"
                    color: "white"
                    Layout.fillWidth: true
                }

                QGCLabel {
                    text: {
                        const L = total_length_m
                        const s = CustomPlugin.wpnavSpeedMps
                        const sp = isFinite(s) ? s.toFixed(1) + " m/s" : "--"
                        if (!(L > 0) || !(s > 0)) return "Required time: -- @ " + sp
                        const minutes = (L / s) / 60.0
                        return "Required time: " + minutes.toFixed(1) + " min @ " + s.toFixed(1) + " m/s"
                    }
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

                // --- row: Upload | Clear ---
                RowLayout {
                    Layout.fillWidth: true
                    spacing: ScreenTools.defaultFontPixelWidth

                    QGCButton {
                        id: uploadBtn
                        text: "Upload"
                        Layout.fillWidth: true
                        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 3

                        enabled: !abluoToolStrip.missionInProgress
                                && serpentine.s_coord && serpentine.s_coord.isValid
                                && serpentine.t_coord && serpentine.t_coord.isValid
                                && abluoToolStrip.pitchValue > 0

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
                                abluoToolStrip.orientationMode
                            )
                            if (!pts || pts.length < 2) {
                                if (CustomPlugin && CustomPlugin.sendLogMessage)
                                    CustomPlugin.sendLogMessage("Empty path: check pitch and altitudes", "", "Warning")
                                return
                            }

                            serpentine.path = pts

                            if (CustomPlugin && CustomPlugin.uploadAbluoMission) {
                                CustomPlugin.uploadAbluoMission(pts, abluoToolStrip.orientationMode)
                                if (CustomPlugin.sendLogMessage)
                                    CustomPlugin.sendLogMessage("Mission uploaded: " + pts.length + " waypoints")
                                CustomPlugin.isAbluoMapPlanEnabled = !CustomPlugin.isAbluoMapPlanEnabled
                            }

                            // reset the “passed” index
                            serpCanvas.passedWpIndex = -1

                            // also update the local path length using the actual waypoint count,
                            // if the plugin already updated it
                            if (CustomPlugin && CustomPlugin.abluoMissionCount !== undefined) {
                                serpentine.setPathLengthFromCount(CustomPlugin.abluoMissionCount)
                            }

                            serpCanvas.schedulePaint()
                        }
                    }

                    QGCButton {
                        id: clearBtn
                        text: "clear mission"
                        Layout.fillWidth: true
                        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 3

                        enabled: !abluoToolStrip.missionInProgress
                        ToolTip.visible: hovered && !enabled
                        ToolTip.text: "Mission is running: clear disabled"

                        onClicked: {
                            // 1) clear local path
                            serpentine.path = []
                            serpentine.total_len_m = 0

                            // 2) clear start/stop in UI and persistent vars
                            serpentine.s_coord = QtPositioning.coordinate()
                            serpentine.t_coord = QtPositioning.coordinate()
                            Constants.savedStart = QtPositioning.coordinate()
                            Constants.savedStop  = QtPositioning.coordinate()
                            serpentine.build()

                            // 3) ask plugin to clear mission on vehicle if available
                            if (CustomPlugin && CustomPlugin.clearAbluoMission) {
                                CustomPlugin.clearAbluoMission()
                            }

                            // 4) reset passed index and repaint
                            serpCanvas.passedWpIndex = -1
                            serpCanvas.schedulePaint()
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
        // 0=horizontal (E-W sweeps), 1=vertical (N-S sweeps)
        property int  orientation: 0

        // Set serpentine.path to a certain (dummy) length, to reflect
        // how many waypoints are currently loaded on the vehicle mission,
        // even if we don't actually have those coordinates here in QML.
        function setPathLengthFromCount(cnt) {
            if (!isFinite(cnt) || cnt <= 0) {
                path = []
                return
            }
            const tmp = []
            for (var i = 0; i < cnt; i++) {
                tmp.push(i)    // placeholder value
            }
            path = tmp
        }

        function _asCoord(p) {
            if (!p) return null
            if (p.isValid !== undefined && p.isValid) return p
            if (p.latitude !== undefined && p.longitude !== undefined) {
                return QtPositioning.coordinate(Number(p.latitude), Number(p.longitude), Number(p.altitude || 0))
            }
            return null
        }

        function _sumPathLen(list) {
            let sum = 0
            for (let i = 1; i < list.length; i++) {
                const a = _asCoord(list[i-1])
                const b = _asCoord(list[i])
                if (a && b && a.isValid && b.isValid) sum += a.distanceTo(b)
            }
            return sum
        }

        onPathChanged: {
            total_len_m = (Array.isArray(path) && path.length >= 2) ? _sumPathLen(path) : 0
        }

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
            width_m = s_coord.distanceTo(t_coord)
            console.log(`AbluoPathWidgetUP: rectangle size: width=${width_m.toFixed(2)} m, height=${height_m.toFixed(2)} m`)
        }

        function build() {
            if (!s_coord || !t_coord || !s_coord.isValid || !t_coord.isValid) { path = []; total_len_m = 0; return }

            _recalc_rect_metrics()

            const z0 = Number(s_coord.altitude) || 0
            const z1 = Number(t_coord.altitude) || 0
            const dz = Math.max(0.001, Number(pitch_m) || 0.001)
            const dirZ = (z1 >= z0) ? +1 : -1

            const south = Math.min(s_coord.latitude,  t_coord.latitude)
            const north = Math.max(s_coord.latitude,  t_coord.latitude)
            const west  = Math.min(s_coord.longitude, t_coord.longitude)
            const east  = Math.max(s_coord.longitude, t_coord.longitude)

            function metersBetween(lat1, lon1, lat2, lon2) {
                return QtPositioning.coordinate(lat1, lon1).distanceTo(QtPositioning.coordinate(lat2, lon2))
            }

            // Explicit sweep axis selection
            // orientation==0 -> sweeps by longitude (E<->W) at constant lat
            // orientation==1 -> sweeps by latitude  (S<->N) at constant lon
            const sweepByLon = (orientation === 0)

            let pts = []

            // 1) First point = exact S at altitude z0
            let p = QtPositioning.coordinate(s_coord.latitude, s_coord.longitude, z0)
            pts.push(p)

            // 2) Second point = go to the opposite side along the "sweep" axis
            let p2 = QtPositioning.coordinate(p.latitude, p.longitude, z0)
            if (sweepByLon) {
                const dW = Math.abs(p.longitude - west)
                const dE = Math.abs(p.longitude - east)
                p2.longitude = (dW < dE) ? east : west
            } else {
                const dS = Math.abs(p.latitude - south)
                const dN = Math.abs(p.latitude - north)
                p2.latitude = (dS < dN) ? north : south
            }
            if (p2.distanceTo(pts[pts.length-1]) > 0.01) pts.push(p2)

            // 3) Vertical steps in altitude + horizontal sweeps at each altitude
            let y = z0
            let cur = p2
            while ((dirZ > 0 && y < z1) || (dirZ < 0 && y > z1)) {
                const remaining = Math.abs(z1 - y)
                const step = Math.min(dz, remaining)
                y += dirZ * step

                // vertical (altitude step)
                let v = QtPositioning.coordinate(cur.latitude, cur.longitude, y)
                if (v.distanceTo(pts[pts.length-1]) > 0.01) pts.push(v)

                // horizontal sweep to the opposite side for the chosen axis
                let h = QtPositioning.coordinate(v.latitude, v.longitude, y)
                if (sweepByLon) {
                    const dW = Math.abs(v.longitude - west)
                    const dE = Math.abs(v.longitude - east)
                    h.longitude = (dW < dE) ? east : west
                } else {
                    const dS = Math.abs(v.latitude - south)
                    const dN = Math.abs(v.latitude - north)
                    h.latitude = (dS < dN) ? north : south
                }
                if (h.distanceTo(pts[pts.length-1]) > 0.01) pts.push(h)
                cur = h
            }

            // 4) Snap exactly to T
            let last = pts[pts.length-1]
            if (Math.abs(last.altitude - z1) > 0.01) {
                let v = QtPositioning.coordinate(last.latitude, last.longitude, z1)
                if (v.distanceTo(last) > 0.01) pts.push(v)
                last = v
            }
            const tExact = QtPositioning.coordinate(t_coord.latitude, t_coord.longitude, z1)
            if (tExact.distanceTo(last) > 0.01) pts.push(tExact)

            // 5) total path length
            let sum = 0
            for (let i = 1; i < pts.length; i++) sum += pts[i-1].distanceTo(pts[i])
            total_len_m = sum
            path = pts
        }

        onS_coordChanged: build()
        onT_coordChanged: build()
        onPitch_mChanged: build()
        onOrientationChanged: build()
        Component.onCompleted: build()
    }
}
