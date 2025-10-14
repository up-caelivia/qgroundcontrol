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

ToolStrip {
    id: abluoToolStrip
    width: Screen.width * 0.6
    height: Screen.height * 0.4
    color: Qt.rgba(0, 0, 0, 0.7)

    // esponi il percorso GPS (per invio piano di volo o per una mappa)
    property alias serpentine_path: serpentine.path

    // pitch verticale in metri da UI
    property real pitchValue: 0

    // Ritorna la coordinata GPS corrente del drone con ALT = AGL (se disponibile)
    function get_current_coord_agl() {
        const vm = QGroundControl.multiVehicleManager
        if (!vm || !vm.activeVehicle) return null
        const v = vm.activeVehicle

        // coordinate lat/lon
        if (!v.coordinate || !v.coordinate.isValid) return null

        // AGL se disponibile, altrimenti AMSL, altrimenti 0
        let altAgl = 0
        try {
            // In QGC, altitudeRelative è un Fact: usa .rawValue
            if (v.altitudeRelative && v.altitudeRelative.rawValue !== undefined) {
                altAgl = Number(v.altitudeRelative.rawValue)
            } else if (v.altitudeAMSL && v.altitudeAMSL.rawValue !== undefined) {
                // fallback ad AMSL se AGL non disponibile
                altAgl = Number(v.altitudeAMSL.rawValue)
            } else if (v.coordinate.altitude !== undefined) {
                altAgl = Number(v.coordinate.altitude)
            }
        } catch (e) {
            altAgl = 0
        }

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

        // --- Sinistra: immagine + quote + marker S/T + serpentina disegnata ---
        Rectangle {
            id: leftPanel
            Layout.preferredWidth: parent.width * 0.5
            Layout.fillHeight: true
            color: "transparent"

            // quote: posizione/stile
            property real dimGapHorizontal: ScreenTools.defaultFontPixelHeight * 1.6
            property real dimGapVertical:   ScreenTools.defaultFontPixelHeight * 1.8
            property real arrowLen:  ScreenTools.defaultFontPixelHeight * 0.9
            property real arrowHalf: arrowLen * 0.45
            property real lineWidth: 2
            property real fontPx:    Math.round(ScreenTools.defaultFontPixelHeight * 1.0)

            // quote: testo (derivato dai metri calcolati dal motore)
            property real width_m:  serpentine.width_m
            property real height_m: serpentine.height_m
            property string dimValueWidthMeters:  width_m > 0 ? width_m.toFixed(2)  + " m" : "--"
            property string dimValueHeightMeters: height_m > 0 ? height_m.toFixed(2) + " m" : "--"

            // spazio riservato a quote
            property real canvasPad: ScreenTools.defaultFontPixelHeight * 0.5
            readonly property real baseMargin:   ScreenTools.defaultFontPixelWidth
            readonly property real topReserve:  (fontPx + arrowLen + 12) + dimGapHorizontal + canvasPad
            readonly property real leftReserve: (fontPx + arrowLen + 14) + dimGapVertical   + canvasPad

            // immagine
            Image {
                id: bigImage
                anchors.fill: parent
                anchors.leftMargin:   leftPanel.baseMargin + leftPanel.leftReserve
                anchors.topMargin:    leftPanel.baseMargin + leftPanel.topReserve
                anchors.rightMargin:  leftPanel.baseMargin
                anchors.bottomMargin: leftPanel.baseMargin
                fillMode: Image.PreserveAspectFit
                source: "/res/facade"
                mipmap: true
            }

            // area effettiva
            readonly property real imgLeft:   bigImage.x + (bigImage.width  - bigImage.paintedWidth)  / 2
            readonly property real imgTop:    bigImage.y + (bigImage.height - bigImage.paintedHeight) / 2
            readonly property real imgRight:  imgLeft + bigImage.paintedWidth
            readonly property real imgBottom: imgTop + bigImage.paintedHeight
            readonly property real imgW:      bigImage.paintedWidth
            readonly property real imgH:      bigImage.paintedHeight

            // marker S/T
            property real markerSize:  Math.round(ScreenTools.defaultFontPixelHeight * 1.2)
            property real markerInset: Math.round(ScreenTools.defaultFontPixelHeight * 0.4)
            property color markerFill: "white"
            property color markerText: "black"
            property color markerBorder: "black"

            // S = in basso a sinistra
            Item {
                width: leftPanel.markerSize; height: leftPanel.markerSize
                x: leftPanel.imgLeft + leftPanel.markerInset
                y: leftPanel.imgBottom - height - leftPanel.markerInset
                z: 20; visible: bigImage.status === Image.Ready
                Rectangle { anchors.fill: parent; radius: width/2; color: leftPanel.markerFill; border.color: leftPanel.markerBorder; border.width: 1 }
                QGCLabel { anchors.centerIn: parent; text: "S"; color: leftPanel.markerText; font.bold: true; font.pointSize: ScreenTools.defaultFontPointSize * 1.1 }
            }
            // T = in alto a destra
            Item {
                width: leftPanel.markerSize; height: leftPanel.markerSize
                x: leftPanel.imgRight - width - leftPanel.markerInset
                y: leftPanel.imgTop + leftPanel.markerInset
                z: 20; visible: bigImage.status === Image.Ready
                Rectangle { anchors.fill: parent; radius: width/2; color: leftPanel.markerFill; border.color: leftPanel.markerBorder; border.width: 1 }
                QGCLabel { anchors.centerIn: parent; text: "T"; color: leftPanel.markerText; font.bold: true; font.pointSize: ScreenTools.defaultFontPointSize * 1.1 }
            }

            // ===== QUOTA ORIZZONTALE =====
            Canvas {
                width:  bigImage.paintedWidth + leftPanel.canvasPad * 2
                height: leftPanel.fontPx + leftPanel.arrowLen + 12 + leftPanel.canvasPad * 2
                x: leftPanel.imgLeft - leftPanel.canvasPad
                y: leftPanel.imgTop - leftPanel.dimGapHorizontal - height + leftPanel.canvasPad
                z: 10; visible: bigImage.status === Image.Ready
                onXChanged: requestPaint(); onYChanged: requestPaint(); onWidthChanged: requestPaint(); onHeightChanged: requestPaint()
                Component.onCompleted: requestPaint()
                onPaint: {
                    const ctx = getContext("2d"); ctx.clearRect(0,0,width,height)
                    const color = "white", lw = leftPanel.lineWidth, aL = leftPanel.arrowLen, aH = leftPanel.arrowHalf, pad = leftPanel.canvasPad
                    const y = height - pad - 4, leftTip = pad, rightTip = width - pad
                    ctx.strokeStyle = color; ctx.fillStyle = color; ctx.lineWidth = lw
                    ctx.beginPath(); ctx.moveTo(leftTip + aL, y); ctx.lineTo(rightTip - aL, y); ctx.stroke()
                    ctx.beginPath(); ctx.moveTo(leftTip, y);  ctx.lineTo(leftTip + aL, y - aH);  ctx.lineTo(leftTip + aL, y + aH);  ctx.closePath(); ctx.fill()
                    ctx.beginPath(); ctx.moveTo(rightTip, y); ctx.lineTo(rightTip - aL, y - aH); ctx.lineTo(rightTip - aL, y + aH); ctx.closePath(); ctx.fill()
                    ctx.font = `${leftPanel.fontPx}px sans-serif`; ctx.textAlign = "center"; ctx.textBaseline = "bottom"
                    ctx.fillText(leftPanel.dimValueWidthMeters, width/2, y - 3)
                }
            }

            // ===== QUOTA VERTICALE =====
            Canvas {
                width:  leftPanel.fontPx + leftPanel.arrowLen + 14 + leftPanel.canvasPad * 2
                height: bigImage.paintedHeight + leftPanel.canvasPad * 2
                x: leftPanel.imgLeft - leftPanel.dimGapVertical - width + leftPanel.canvasPad
                y: leftPanel.imgTop  - leftPanel.canvasPad
                z: 10; visible: bigImage.status === Image.Ready
                onXChanged: requestPaint(); onYChanged: requestPaint(); onWidthChanged: requestPaint(); onHeightChanged: requestPaint()
                Component.onCompleted: requestPaint()
                onPaint: {
                    const ctx = getContext("2d"); ctx.clearRect(0,0,width,height)
                    const color = "white", lw = leftPanel.lineWidth, aL = leftPanel.arrowLen, aH = leftPanel.arrowHalf, pad = leftPanel.canvasPad
                    const x = width - pad - 4, topTip = pad, bottomTip = height - pad
                    ctx.strokeStyle = color; ctx.fillStyle = color; ctx.lineWidth = lw
                    ctx.beginPath(); ctx.moveTo(x, topTip + aL); ctx.lineTo(x, bottomTip - aL); ctx.stroke()
                    ctx.beginPath(); ctx.moveTo(x, topTip);      ctx.lineTo(x - aH, topTip + aL);    ctx.lineTo(x + aH, topTip + aL);    ctx.closePath(); ctx.fill()
                    ctx.beginPath(); ctx.moveTo(x, bottomTip);   ctx.lineTo(x - aH, bottomTip - aL); ctx.lineTo(x + aH, bottomTip - aL); ctx.closePath(); ctx.fill()
                    ctx.save(); ctx.translate(x - 6, (topTip + bottomTip)/2); ctx.rotate(-Math.PI/2)
                    ctx.font = `${leftPanel.fontPx}px sans-serif`; ctx.textAlign = "center"; ctx.textBaseline = "bottom"
                    ctx.fillText(leftPanel.dimValueHeightMeters, 0, -3); ctx.restore()
                }
            }

            // ===== SERPENTINA sopra l'immagine (gialla) =====
            Canvas {
                id: serpCanvas
                x: leftPanel.imgLeft
                y: leftPanel.imgTop
                width:  leftPanel.imgW
                height: leftPanel.imgH
                z: 15
                visible: bigImage.status === Image.Ready

                // proxy per triggerare repaint senza Connections
                property real pitch_proxy: abluoToolStrip.pitchValue
                onPitch_proxyChanged: requestPaint()
                property real height_m_proxy: leftPanel.height_m
                onHeight_m_proxyChanged: requestPaint()
                onXChanged: requestPaint(); onYChanged: requestPaint()
                onWidthChanged: requestPaint(); onHeightChanged: requestPaint()
                Component.onCompleted: requestPaint()

                onPaint: {
                    const ctx = getContext("2d")
                    ctx.clearRect(0,0,width,height)

                    // rettangolo interno fra i centri dei marker S/T
                    const inset = leftPanel.markerInset + leftPanel.markerSize/2
                    const leftX   = inset
                    const rightX  = width  - inset
                    const topY    = inset
                    const bottomY = height - inset
                    const rectHpx = bottomY - topY

                    if (leftPanel.height_m <= 0 || rectHpx <= 0) return

                    // metri -> pixel (quota verticale)
                    const pxPerMeterV = rectHpx / leftPanel.height_m
                    const stepPxNom   = Math.max(1, abluoToolStrip.pitchValue * pxPerMeterV)

                    // stile
                    ctx.strokeStyle = "#FFD600"   // giallo
                    ctx.lineWidth   = 2

                    // parte sempre da S (bottom-left) e arriva a T (top-right)
                    let y = bottomY
                    let goRight = true
                    ctx.beginPath()
                    ctx.moveTo(leftX, y)

                    while (y > topY) {
                        ctx.lineTo(goRight ? rightX : leftX, y)          // orizzontale
                        const step = Math.min(stepPxNom, y - topY)       // ultimo step accorciato
                        y -= step
                        ctx.lineTo(goRight ? rightX : leftX, y)          // verticale
                        goRight = !goRight
                    }

                    // chiudi su T orizzontalmente se necessario
                    const currentX = goRight ? leftX : rightX
                    if (currentX !== rightX) ctx.lineTo(rightX, topY)

                    ctx.stroke()
                }
            }
        }

        // --- Destra: controlli ---
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Qt.rgba(1,1,1,0.06)
            radius: ScreenTools.defaultFontPixelHeight
            border.color: Qt.rgba(1,1,1,0.15)
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: ScreenTools.defaultFontPixelWidth * 2
                spacing: ScreenTools.defaultFontPixelHeight

                QGCLabel {
                    text: "ABLUO planning"
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillWidth: true
                    font.pointSize: ScreenTools.isMobile ? ScreenTools.defaultFontPointSize * 1.5
                                                         : ScreenTools.defaultFontPointSize * 3
                }

                QGCButton {
                    text: "Start Point"
                    Layout.fillWidth: true
                    onClicked: {
                        if (CustomPlugin && CustomPlugin.setStart) CustomPlugin.setStart()
                        const cur = get_current_coord_agl()
                        if (cur) {
                            serpentine.s_coord = cur      // salva per il piano di volo (GPS)
                            serpentine.build()            // ricalcola metri/serpentina
                            serpCanvas.requestPaint()     // ridisegna sopra la facciata
                        }
                    }
                }

                QGCButton {
                    text: "Stop Point"
                    Layout.fillWidth: true
                    onClicked: {
                        if (CustomPlugin && CustomPlugin.setStop) CustomPlugin.setStop()
                        const cur = get_current_coord_agl()
                        if (cur) {
                            serpentine.t_coord = cur      // salva per il piano di volo (GPS)
                            serpentine.build()
                            serpCanvas.requestPaint()
                        }
                    }
                }


                QGCLabel { text: "Pitch (m)"; color: "white"; Layout.fillWidth: true }

                QGCTextField {
                    id: pitchField
                    Layout.fillWidth: true
                    placeholderText: "insert Pitch"
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    validator: DoubleValidator { bottom: 0; top: 1e6; decimals: 3 }
                    onEditingFinished: {
                        abluoToolStrip.pitchValue = Number(text)
                        serpentine.pitch_m = abluoToolStrip.pitchValue
                        if (CustomPlugin && !isNaN(abluoToolStrip.pitchValue) && CustomPlugin.setPitch)
                            CustomPlugin.setPitch(abluoToolStrip.pitchValue)
                        serpentine.build()
                        serpCanvas.requestPaint()
                    }
                }

                Item { Layout.fillHeight: true }

                QGCButton {
                    text: "uppload and start cleaning"
                    Layout.fillWidth: true
                    Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 3
                    onClicked: { if (CustomPlugin && CustomPlugin.start) CustomPlugin.start() }
                }
            }
        }
    }

    // ===== Motore SERPENTINA (GPS) =====
    Item {
        id: serpentine

        // input (tutto in minuscolo)
        property var  s_coord: QtPositioning.coordinate(45.000000, 9.000000, 10)   // S: basso-sinistra
        property var  t_coord: QtPositioning.coordinate(45.005000, 9.010000, 40)   // T: alto-destra
        property real pitch_m: 10

        // dimensioni rettangolo in metri (calcolate)
        property real width_m:  0
        property real height_m: 0

        // output percorso GPS
        property var  path: []    // polilinea a zig-zag S→…→T

        // interpola lungo la geodetica
        function _interp(c1, c2, tt) {
            const az = c1.azimuthTo(c2)
            const d  = c1.distanceTo(c2)
            return c1.atDistanceAndAzimuth(d * Math.max(0, Math.min(1, tt)), az)
        }

        function _recalc_rect_metrics() {
            const south = Math.min(s_coord.latitude,  t_coord.latitude)
            const north = Math.max(s_coord.latitude,  t_coord.latitude)
            const west  = Math.min(s_coord.longitude, t_coord.longitude)
            const east  = Math.max(s_coord.longitude, t_coord.longitude)
            const bl = QtPositioning.coordinate(south, west)
            const br = QtPositioning.coordinate(south, east)
            const tl = QtPositioning.coordinate(north, west)
            width_m  = bl.distanceTo(br)         // E-W
            height_m = bl.distanceTo(tl)         // S-N
        }

        function build() {
            if (!s_coord || !t_coord) return
            _recalc_rect_metrics()

            const z0 = s_coord.altitude || 0
            const z1 = t_coord.altitude || z0
            const dz = Math.max(0.001, pitch_m)
            const totalH = Math.max(0.001, z1 - z0)

            // angoli rettangolo (assiale)
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
            let leftToRight = true // parte da S (sinistra) verso destra

            // passi interi
            while (y < z1) {
                const next = Math.min(z1, y + dz) // ultimo step ridotto per arrivare a T
                const t1 = (y    - z0) / totalH
                const t2 = (next - z0) / totalH

                const pL1 = _interp(bl, tl, t1); pL1.altitude = y
                const pR1 = _interp(br, tr, t1); pR1.altitude = y
                const pL2 = _interp(bl, tl, t2); pL2.altitude = next
                const pR2 = _interp(br, tr, t2); pR2.altitude = next

                // orizzontale alla quota y
                if (leftToRight) { pts.push(pL1); pts.push(pR1) } else { pts.push(pR1); pts.push(pL1) }
                // verticale sul bordo corrente fino alla quota next
                if (leftToRight) { pts.push(pR2) } else { pts.push(pL2) }

                leftToRight = !leftToRight
                y = next
            }

            // garantisci arrivo esatto a T (top-right)
            const tTop = 1.0
            const pRtop = _interp(br, tr, tTop); pRtop.altitude = z1
            const last = pts.length ? pts[pts.length-1] : null
            if (!last || last.latitude !== pRtop.latitude || last.longitude !== pRtop.longitude || last.altitude !== pRtop.altitude) {
                pts.push(pRtop)
            }

            path = pts
        }

        onS_coordChanged: build()
        onT_coordChanged: build()
        onPitch_mChanged: build()
        Component.onCompleted: build()
    }
}
