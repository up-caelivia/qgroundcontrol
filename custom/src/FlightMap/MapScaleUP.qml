/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3

import QGroundControl                   1.0
import QGroundControl.Controls          1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.SettingsManager   1.0

import "qrc:/js/MapScaleMath.js" as MapScale

Item {
    id:     scale
    width:  buttonsOnLeft || !_zoomButtonsVisible ? rightEnd.x + rightEnd.width : zoomDownButton.x + zoomDownButton.width
    height: rightEnd.y + rightEnd.height

    property var    mapControl
    property bool   terrainButtonVisible:   false
    property alias  terrainButtonChecked:   terrainButton.checked
    property bool   zoomButtonsVisible:     true
    property bool   buttonsOnLeft:          true

    signal terrainButtonClicked

    property bool   _zoomButtonsVisible:    zoomButtonsVisible && !ScreenTools.isMobile
    property var    _color:                 mapControl.isSatelliteMap ? "white" : "black"

    Connections {
        target:             mapControl
        function onWidthChanged() {     scaleTimer.restart() }
        function onHeightChanged() {    scaleTimer.restart() }
        function onZoomLevelChanged() { scaleTimer.restart() }
    }

    Timer {
        id:                 scaleTimer
        interval:           100
        running:            false
        repeat:             false
        onTriggered:        MapScale.calculateScale()
    }

    QGCMapLabel {
        id:                 scaleText
        map:                mapControl
        font.family:        ScreenTools.demiboldFontFamily
        anchors.left:       parent.left
        anchors.right:      rightEnd.right
        horizontalAlignment:Text.AlignRight
        text:               "0 m"
    }

    Rectangle {
        id:                 leftEnd
        anchors.top:        scaleText.bottom
        anchors.leftMargin: buttonsOnLeft && (_zoomButtonsVisible || terrainButtonVisible) ? ScreenTools.defaultFontPixelWidth / 2 : 0
        anchors.left:       buttonsOnLeft ?
                                (_zoomButtonsVisible ? zoomDownButton.right : (terrainButtonVisible ? terrainButton.right : parent.left)) :
                                parent.left
        width:              2
        height:             ScreenTools.defaultFontPixelHeight
        color:              _color
    }

    Rectangle {
        id:                 centerLine
        anchors.bottomMargin:   2
        anchors.bottom:     leftEnd.bottom
        anchors.left:       leftEnd.right
        height:             2
        color:              _color
    }

    Rectangle {
        id:                 rightEnd
        anchors.top:        leftEnd.top
        anchors.left:       centerLine.right
        width:              2
        height:             ScreenTools.defaultFontPixelHeight
        color:              _color
    }

    QGCButton {
        id:                 terrainButton
        anchors.top:        scaleText.top
        anchors.bottom:     rightEnd.bottom
        anchors.leftMargin: buttonsOnLeft ? 0 : ScreenTools.defaultFontPixelWidth / 2
        anchors.left:       buttonsOnLeft ? parent.left : rightEnd.right
        text:               qsTr("T")
        width:              height
        opacity:            0.75
        visible:            terrainButtonVisible
        onClicked:          terrainButtonClicked()
    }

    QGCButton {
        id:                 zoomUpButton
        anchors.top:        scaleText.top
        anchors.bottom:     rightEnd.bottom
        anchors.leftMargin: terrainButton.visible ? ScreenTools.defaultFontPixelWidth / 2 : 0
        anchors.left:       terrainButton.visible ? terrainButton.right : terrainButton.left
        text:               qsTr("+")
        width:              height
        opacity:            0.75
        visible:            _zoomButtonsVisible
        onClicked:          mapControl.zoomLevel += 0.5
    }

    QGCButton {
        id:                 zoomDownButton
        anchors.top:        scaleText.top
        anchors.bottom:     rightEnd.bottom
        anchors.leftMargin: ScreenTools.defaultFontPixelWidth / 2
        anchors.left:       zoomUpButton.right
        text:               qsTr("-")
        width:              height
        opacity:            0.75
        visible:            _zoomButtonsVisible
        onClicked:          mapControl.zoomLevel -= 0.5
    }

    Component.onCompleted: {
        if (scale.visible) {
            MapScale.calculateScale();
        }
    }
}
