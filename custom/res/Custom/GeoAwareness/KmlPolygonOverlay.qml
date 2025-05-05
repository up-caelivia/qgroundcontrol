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
        border.color: showBorder ? modelData.color : modelData.transparent
        border.width: 2
        opacity: 0.4
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