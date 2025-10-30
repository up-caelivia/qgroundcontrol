import QtQuick 2.15
import QtLocation 5.15
import QGroundControl.KML 1.0

MapItemView {
    id: polygonOverlay

    // Il modello da usare (può essere esterno o di default)
    property var polygonModel: []
    property var showBorder: true
    property var map
    property Component popupMenuComponent
    property var root
    property var selectedPolygon: []

    model: polygonModel

    delegate: MapPolygon {
        path: modelData.coordinates
        color: modelData.color
        border.color: modelData.color
        border.width: showBorder ? (selectedPolygon==modelData ? 4: 2) : 0
        opacity: showBorder ? (selectedPolygon==modelData ? 0.6: 0.4) : 0.2
        z: 100

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton

            property var lastClickTime: 0
            property var doubleClickThreshold: 250 // ms

            onClicked: {
                var currentTime = Date.now()
                if (currentTime - lastClickTime < doubleClickThreshold) {
                    if (map){
                        var globalPoint = mapToItem(map, Qt.point(mouse.x, mouse.y))
                        var clickedCoord = map.toCoordinate(globalPoint, false)
                        var listObj = KmlPolygonLoader.checkPolygonsInPoint(clickedCoord)
                        root.showPolygonMenuAtMouse(globalPoint.x, globalPoint.y, listObj)
                    }
                } else {
                    KmlPolygonLoader.selectPolygon(modelData)
                }
                lastClickTime = currentTime
            }
        }
    }
}   