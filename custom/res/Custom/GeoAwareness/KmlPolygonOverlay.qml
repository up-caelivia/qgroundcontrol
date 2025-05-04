import QtQuick 2.15
import QtLocation 5.15
import QGroundControl.KML 1.0

MapItemView {
    id: polygonOverlay

    // Il modello da usare (può essere esterno o di default)
    property var polygonModel: []

    model: polygonModel

    delegate: MapPolygon {
        path: modelData.coordinates
        color: "lightblue"
        border.color: "red"
        border.width: 2
        opacity: 1
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