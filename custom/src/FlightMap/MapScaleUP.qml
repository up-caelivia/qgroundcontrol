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
    width:  buttonsOnLeft ? rightEnd.x + rightEnd.width
                         : centerButton.visible ? centerButton.x + centerButton.width
                         : _zoomButtonsVisible ? zoomDownButton.x + zoomDownButton.width
                         : terrainButtonVisible ? terrainButton.x + terrainButton.width
                         : rightEnd.x + rightEnd.width
    height: rightEnd.y + rightEnd.height

    property var    mapControl
    property var    navigationController: null
    property bool   terrainButtonVisible:   false
    property alias  terrainButtonChecked:   terrainButton.checked
    property bool   zoomButtonsVisible:     true
    property bool   buttonsOnLeft:          true

    property bool   _zoomButtonsVisible:    zoomButtonsVisible && !ScreenTools.isMobile
    property var    _color:                 mapControl.isSatelliteMap ? "white" : "black"

    signal terrainButtonClicked()

    Connections {
        target: scale.mapControl
        function onWidthChanged() {     scaleTimer.restart() }
        function onHeightChanged() {    scaleTimer.restart() }
        function onZoomLevelChanged() { scaleTimer.restart() }
        function onCenterChanged() {    scaleTimer.restart() }
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
        map:                scale.mapControl
        font.family:        ScreenTools.demiboldFontFamily
        anchors.left:       parent.left
        anchors.right:      rightEnd.right
        horizontalAlignment:Text.AlignRight
        text:               "0 m"
    }

    Rectangle {
        id:                 leftEnd
        anchors.top:        scaleText.bottom
        anchors.leftMargin: scale.buttonsOnLeft && (centerButton.visible || scale._zoomButtonsVisible || scale.terrainButtonVisible) ? ScreenTools.defaultFontPixelWidth / 2 : 0
        anchors.left:       !scale.buttonsOnLeft ? parent.left
                                : centerButton.visible ? centerButton.right
                                : scale._zoomButtonsVisible ? zoomDownButton.right
                                : scale.terrainButtonVisible ? terrainButton.right : parent.left
        width:              2
        height:             ScreenTools.defaultFontPixelHeight
        color:              scale._color
    }

    Rectangle {
        id:                 centerLine
        anchors.bottomMargin:   2
        anchors.bottom:     leftEnd.bottom
        anchors.left:       leftEnd.right
        height:             2
        color:              scale._color
    }

    Rectangle {
        id:                 rightEnd
        anchors.top:        leftEnd.top
        anchors.left:       centerLine.right
        width:              2
        height:             ScreenTools.defaultFontPixelHeight
        color:              scale._color
    }

    QGCButton {
        id:                 terrainButton
        anchors.top:        scaleText.top
        anchors.bottom:     rightEnd.bottom
        anchors.leftMargin: scale.buttonsOnLeft ? 0 : ScreenTools.defaultFontPixelWidth / 2
        anchors.left:       scale.buttonsOnLeft ? parent.left : rightEnd.right
        text:               qsTr("T")
        width:              height
        opacity:            0.75
        visible:            scale.terrainButtonVisible
        onClicked:          scale.terrainButtonClicked()
        backRadius:         ScreenTools.defaultFontPointSize * 1.2
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
        visible:            scale._zoomButtonsVisible
        onClicked:          scale.mapControl.zoomLevel += 0.5
        backRadius:         ScreenTools.defaultFontPointSize * 1.2
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
        visible:            scale._zoomButtonsVisible
        onClicked:          scale.mapControl.zoomLevel -= 0.5
        backRadius:         ScreenTools.defaultFontPointSize * 1.2
    }

    QGCButton {
        id:                 centerButton
        anchors.top:        scaleText.top
        anchors.bottom:     rightEnd.bottom
        anchors.leftMargin: scale.buttonsOnLeft && !scale._zoomButtonsVisible && !terrainButton.visible ? 0 : ScreenTools.defaultFontPixelWidth / 2
        anchors.left:       scale._zoomButtonsVisible ? zoomDownButton.right
                                : terrainButton.visible ? terrainButton.right : terrainButton.left
        width:              height
        opacity:            0.75
        visible:            !!scale.navigationController && scale.navigationController.canCenterOnVehicle
        iconSource:         "/qmlimages/MapCenter.svg"
        iconCentered:       true
        onClicked:          scale.navigationController.centerOnVehicle()
        backRadius:         ScreenTools.defaultFontPointSize * 1.2
    }

    Component.onCompleted: {
        if (scale.visible) {
            MapScale.calculateScale();
        }
    }
}
