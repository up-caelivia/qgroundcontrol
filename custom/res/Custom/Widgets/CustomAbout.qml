/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Layouts  1.11

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

import Constants 1.0

Rectangle {
    color:          qgcPal.window
    anchors.fill:   parent

    readonly property real _margins: ScreenTools.defaultFontPixelHeight
    property int clickCount: 0

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    QGCFlickable {
        anchors.margins:    _margins
        anchors.fill:       parent
        contentWidth:       grid.width
        contentHeight:      grid.height
        clip:               true
        
		GridLayout {
            id:         grid
            columns: 2
            rowSpacing: 6
            columnSpacing: 20

            QGCLabel { text: "R&D:"; font.bold: true }
            QGCLabel { text: "Via Negrelli, 8 - 39100 - Bolzano (BZ) - Italy" }

            QGCLabel { text: "Legal address:"; font.bold: true }
            QGCLabel { text: "Via della Fossa, 1 - 38051 - Borgo Valsugana (TN) - Italy" }

            QGCLabel { text: "Flying test address:"; font.bold: true }
            QGCLabel { text: "Via Rossano, 44 - 36056 - Belvedere di Tezze sul Brenta (VI) - Italy" }

            QGCLabel { text: "P. IVA (VAT):"; font.bold: true }
            QGCLabel { text: "IT02165150224" }

            QGCLabel { text: "Phone Number:"; font.bold: true }
            QGCLabel { text: "+39-0424-861021" }

            QGCLabel { text: "E-mail:"; font.bold: true }
            QGCLabel { text: "info@up-caelivia.it" }

            QGCLabel { text: "Developer Mode ON"; 
                font.bold: true
                Layout.columnSpan: 2 
                visible: Constants.developer}

            Image {
                source: "qrc:/res/QGCLogoFull"  
                fillMode: Image.PreserveAspectFit
                Layout.columnSpan: 2
                //Layout.alignment: Qt.AlignHLeft
                Layout.preferredHeight: 100

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        clickCount++
                        if (!clickTimer.running) {
                            clickTimer.start()
                        }

                        if (clickCount >= 6) {
                            clickTimer.stop()
                            clickCount = 0
                            console.log("Comando iniizato!")
                            Constants.developer = !Constants.developer 
                            console.log("Comando eseguito!")
                        }
                    }
                }

                Timer {
                    id: clickTimer
                    interval: 5000 // 5 seconds
                    running: false
                    repeat: false
                    onTriggered: {
                        // ⏳ Timeout: reset conteggio
                        clickCount = 0
                    }
                }
            }
        }
    }
}

