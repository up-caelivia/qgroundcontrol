import QtQuick 2.15
import QtLocation 5.15
import QGroundControl.KML 1.0

MapItemView {
    id: polygonOverlay

    // Il modello da usare (può essere esterno o di default)
    property var polygonModel: []
    property var showBorder: true

    model: polygonModel

    delegate: MapPolygon {
        path: modelData.coordinates
        color: modelData.color
        border.color: modelData.color
        border.width: showBorder ? 2 : 0
        opacity: showBorder ? 0.4 : 0.2
        z: 100

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                KmlPolygonLoader.selectPolygon(modelData)
            }
        }
    }
}   